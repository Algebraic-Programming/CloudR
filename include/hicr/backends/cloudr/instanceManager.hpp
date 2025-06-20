
#pragma once

#include <hicr/core/exceptions.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/instanceManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/frontends/RPCEngine/RPCEngine.hpp>
#include <nlohmann_json/json.hpp>
#include <nlohmann_json/parser.hpp>
#include "instance.hpp"

namespace HiCR::backend::cloudr
{

class InstanceManager final : public HiCR::InstanceManager
{
  public:

  #define __CLOUDR_LAUNCH_MAIN_RPC_NAME "[CloudR] Launch Main"
  #define __CLOUDR_FINALIZE_WORKER_RPC_NAME "[CloudR] Finalize Worker"
  #define __CLOUDR_REQUEST_TOPOLOGY_RPC_NAME "[CloudR] Request Topology"


  typedef std::function<int(HiCR::backend::cloudr::InstanceManager* cloudr, int argc, char** argv)> mainFc_t;

  InstanceManager(mainFc_t mainFunction) : HiCR::InstanceManager(), _mainFunction(mainFunction)
  {
  }

  ~InstanceManager() {}

  __INLINE__ void initialize(int *pargc, char ***pargv)
  {
    // Initializing instance
    _instanceManager      = HiCR::backend::mpi::InstanceManager::createDefault(pargc, pargv);
    _communicationManager = std::make_unique<HiCR::backend::mpi::CommunicationManager>(MPI_COMM_WORLD);
    _memoryManager        = std::make_unique<HiCR::backend::mpi::MemoryManager>();
    _computeManager       = std::make_unique<HiCR::backend::pthreads::ComputeManager>();

    // Storing arguments
    std::vector<std::string> args;
    for (int i = 0; i < *pargc; i++) args.push_back((*pargv)[i]);

    // Getting argc and argv values
    _argc = args.size();
    _argv = (char**) malloc (args.size() * sizeof(char*));
    for (size_t i = 0; i < args.size(); i++) _argv[i] = (char*)args[i].c_str();

    // Reserving memory for hwloc
    hwloc_topology_init(&_hwlocTopology);

    // Initializing HWLoc-based host (CPU) topology manager
    _topologyManager = std::make_unique<HiCR::backend::hwloc::TopologyManager>(&_hwlocTopology);

    // Finding the first memory space and compute resource to create our RPC engine
    auto firstDevice        = _topologyManager->queryTopology().getDevices().begin().operator*();
    auto RPCMemorySpace     = firstDevice->getMemorySpaceList().begin().operator*();
    auto RPCComputeResource = firstDevice->getComputeResourceList().begin().operator*();

    // Instantiating RPC engine
    _rpcEngine = std::make_unique<HiCR::frontend::RPCEngine>(*_communicationManager, *_instanceManager, *_memoryManager, *_computeManager, RPCMemorySpace, RPCComputeResource);

    // Initializing RPC engine
    _rpcEngine->initialize();

    // Registering launch function
    auto launchMainExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { runMainFunctionRPC(); });
    _rpcEngine->addRPCTarget(__CLOUDR_LAUNCH_MAIN_RPC_NAME, launchMainExecutionUnit);

    // Registering finalize function
    auto finalizeWorkerExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { finalizeWorker(); });
    _rpcEngine->addRPCTarget(__CLOUDR_FINALIZE_WORKER_RPC_NAME, finalizeWorkerExecutionUnit);

    // Registering request topology function
    auto requestTopologyExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { requestTopology(); });
    _rpcEngine->addRPCTarget(__CLOUDR_REQUEST_TOPOLOGY_RPC_NAME, requestTopologyExecutionUnit);

    // Creating instance objects from the initially found instances now
    HiCR::Instance::instanceId_t instanceIdCounter = 0;
    for (auto& instance : _instanceManager->getInstances())
    {
     // Only the current instance is the root one
     const bool isRoot = _instanceManager->getRootInstanceId() == instance->getId();

     // Creating new cloudr instance object (contains all the emulated information)
     auto newInstance = std::make_shared<HiCR::backend::cloudr::Instance>(instanceIdCounter, instance.get(), isRoot);

     // Adding new instance to the internal storage
     _cloudrInstances.push_back(newInstance);

     // If this is the current instance, set it now
     if (instance->getId() == _instanceManager->getCurrentInstance()->getId()) setCurrentInstance(newInstance);

     // If it's root, store its pointer
     if (isRoot)
     {
      // Store root instance pointer for later referencing
      _rootInstance = newInstance.get();

      // Adding instance to the collection of currently active instances
      addInstance(newInstance);
     } 

     // If not root, add to the list of free instances (can be activated later)
     if (isRoot == false) _freeInstances.insert(newInstance);

     // Linking base instance id to the respective cloudr instance
     _baseIdsToCloudrInstanceMap[instance->getId()] = newInstance.get();

     // Increasing cloudr instance Id
     instanceIdCounter++;
    }

    // Main loop for running instances
    if (_instanceManager->getRootInstanceId() != _instanceManager->getCurrentInstance()->getId()) 
    {
      _continueListening = true;
      while(_continueListening) _rpcEngine->listen();
    } 
  }

  __INLINE__ void setInstanceTopologies(const nlohmann::json& instanceTopologiesJs)
  {
    // This function will only be ran by the root rank
    if (_instanceManager->getRootInstanceId() != _instanceManager->getCurrentInstance()->getId()) return;

    // Getting array of topologies
    auto instanceTopologies = hicr::json::getArray<nlohmann::json>(instanceTopologiesJs, "Instance Topologies");

    // Check whether the number of topologies passed coincides with the number of instances
    if (instanceTopologies.size() != _cloudrInstances.size()) HICR_THROW_LOGIC("Trying to configure the topology of %lu instances, when %lu were actually created\n", instanceTopologies.size(),_cloudrInstances.size());

    // Assigning topologies to each of the detected instances
    for (size_t i = 0; i < _cloudrInstances.size(); i++)
    {
      // Grabbing the instance topology to emulate
      const auto& instanceTopologyJs = instanceTopologies[i];

      // Setting the instance's topology
      const auto& cloudrInstance = _cloudrInstances[i].get();
      cloudrInstance->setTopology(instanceTopologyJs);
    }
  }

  __INLINE__ void startService()
  {
    // This function will only be ran by the root rank
    if (_instanceManager->getRootInstanceId() != _instanceManager->getCurrentInstance()->getId()) return;

    // Running main driver function ourselves
    runMainFunctionImpl();
  }

  __INLINE__ void stopService()
  {
    // This function will only be ran by the root rank
    if (_instanceManager->getRootInstanceId() != _instanceManager->getCurrentInstance()->getId()) return;

    // Requesting others to shut down
    for (auto& instance : _cloudrInstances) if (instance->isRootInstance() == false) _rpcEngine->requestRPC(*instance, __CLOUDR_FINALIZE_WORKER_RPC_NAME);
  }

  __INLINE__ void finalize() override
  {
    // Finalizing underlying instance manager
    _instanceManager->finalize();
  } 

  __INLINE__ void abort(int errorCode) override { _instanceManager->abort(errorCode); }

  [[nodiscard]] __INLINE__ HiCR::Instance::instanceId_t getRootInstanceId() const override { return 0; }

  protected:

  __INLINE__ std::shared_ptr<HiCR::Instance> createInstanceImpl (const HiCR::InstanceTemplate instanceTemplate) override
  {
    // If no more free instances available, fail now
    // Commented out because we don't want to fail, simply return a nullptr
    // if (_freeInstances.empty()) HICR_THROW_LOGIC("Requested the creation of a new instances, but CloudR has ran out of free instances");

    // Creating instance object to return
    std::shared_ptr<HiCR::backend::cloudr::Instance> newInstance = nullptr; 

    // Getting requested topology from the instance template
    const auto& topology = instanceTemplate.getTopology();

    // Iterating over free instances to get the first one that satisfies the request
    for (const auto& instance : _freeInstances) if (instance->isCompatible(topology))
    {
      // Assigning it as compatible instance
      newInstance = instance;

      // Erasing it from the list of free instances
      _freeInstances.erase(instance);

      // Stop looking into the others
      break;
    }

    // Commented out because we don't want to fail, simply return a nullptr
    // if (newInstance == nullptr)  HICR_THROW_LOGIC("Tried to create new instance but did not find any free instances that meet the required topology");

    // If successful, initialize the new instance
    if (newInstance != nullptr)
    {
      // (1) Request the execution of the main driver function
      _rpcEngine->requestRPC(*newInstance->getBaseInstance(), __CLOUDR_LAUNCH_MAIN_RPC_NAME);

      // (2) Listen for incoming topology information requests
      _rpcEngine->listen();
    }

    // Returning result. Nullptr, if no instance was created
    return newInstance;
  }

  __INLINE__ std::shared_ptr<HiCR::Instance> addInstanceImpl(HiCR::Instance::instanceId_t instanceId) override
  {
    HICR_THROW_LOGIC("The Host backend does not currently support the detection of new instances during runtime");
  }

  private:

  __INLINE__ void runMainFunctionRPC()
  {
    // Requesting the root 
    _rpcEngine->requestRPC(*_rootInstance, __CLOUDR_REQUEST_TOPOLOGY_RPC_NAME);

    // Getting return value (topology)
    auto returnValue = _rpcEngine->getReturnValue(*_rootInstance);

    // Receiving raw serialized topology information from the worker
    std::string serializedTopology = (char *)returnValue->getPointer();

    // Parsing serialized raw topology into a json object
    auto topologyJson = nlohmann::json::parse(serializedTopology);

    // Freeing return value
    _memoryManager->freeLocalMemorySlot(returnValue);

    // Updating current instance's topology
    static_cast<HiCR::backend::cloudr::Instance*>(getCurrentInstance().get())->setTopology(topologyJson);

    // Running main function
    runMainFunctionImpl();
  }

  __INLINE__ void runMainFunctionImpl()
  {
    // Running main function
    int retVal = _mainFunction(this, _argc, _argv);

    // Checking return value
    if (retVal != 0) HICR_THROW_RUNTIME("Instance %lu returned main with a non-zero (%d) error code", getCurrentInstance()->getId(), retVal);
  }

  __INLINE__ void requestTopology()
  {
    // Getting a pointer to the base instance who made the request
    auto baseRequesterInstance = _rpcEngine->getRPCRequester();

    // Getting base instance id
    const auto baseInstanceId = baseRequesterInstance->getId();

    // Getting cloudr instance from the base instance id
    const auto cloudrInstance = _baseIdsToCloudrInstanceMap.at(baseInstanceId);

    // Serializing topology information
    const auto serializedTopology = cloudrInstance->getTopologyJs().dump();

    // Returning serialized topology
    _rpcEngine->submitReturnValue((void *)serializedTopology.c_str(), serializedTopology.size() + 1);
  }

  __INLINE__ void finalizeWorker()
  {
    // Do not continue listening
    _continueListening = false;
  }

  /// Storage for the distributed engine's communication manager
  std::unique_ptr<HiCR::CommunicationManager> _communicationManager;

  /// Storage for the distributed engine's instance manager
  std::unique_ptr<HiCR::InstanceManager> _instanceManager;

  /// Storage for the distributed engine's memory manager
  std::unique_ptr<HiCR::MemoryManager> _memoryManager;

  /// Storage for compute manager
  std::unique_ptr<HiCR::backend::pthreads::ComputeManager> _computeManager;

  /// Storage for topology manager
  std::unique_ptr<HiCR::backend::hwloc::TopologyManager> _topologyManager;

  /// RPC engine
  std::unique_ptr<HiCR::frontend::RPCEngine> _rpcEngine;

  /// Hwloc topology object
  hwloc_topology_t _hwlocTopology;

  /// Storage for launch arguments count
  int _argc;

  /// Storage for launch arguments values
  char** _argv;

  /// Storage for the main function to run when a new instance runs
  const mainFc_t _mainFunction;

  // Pointer to the root instance
  HiCR::backend::cloudr::Instance* _rootInstance;

  // Flag to signal non-root instances to finish listening
  bool _continueListening;

  // Map that links the underlying instance ids with the cloudr instances
  std::map<HiCR::Instance::instanceId_t, HiCR::backend::cloudr::Instance*> _baseIdsToCloudrInstanceMap;
  
  /// Internal collection of cloudr instances
  std::vector<std::shared_ptr<HiCR::backend::cloudr::Instance>> _cloudrInstances;

  /// A collection of ready-to-use instances currently on standby
  std::set<std::shared_ptr<HiCR::backend::cloudr::Instance>> _freeInstances;

}; // class CloudR

} // namespace cloudr