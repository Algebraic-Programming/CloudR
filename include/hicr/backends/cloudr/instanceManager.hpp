
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

  typedef std::function<int(int argc, char** argv)> mainFc_t;

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
    for (int i = 0; i < *pargc; i++) _args.push_back((*pargv)[i]);

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
    auto launchMainExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { runMainFunction(); });
    _rpcEngine->addRPCTarget(__CLOUDR_LAUNCH_MAIN_RPC_NAME, launchMainExecutionUnit);

    // Registering finalize function
    auto finalizeWorkerExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { finalizeWorker(); });
    _rpcEngine->addRPCTarget(__CLOUDR_FINALIZE_WORKER_RPC_NAME, finalizeWorkerExecutionUnit);

    // Creating instance objects from the initially found instances now
    HiCR::Instance::instanceId_t instanceIdCounter = 0;
    for (auto& instance : _instanceManager->getInstances())
    {
     // Only the current instance is the root one
     const bool isRoot = _instanceManager->getRootInstanceId() == _instanceManager->getCurrentInstance()->getId();

     // Creating new cloudr instance object (contains all the emulated information)
     auto newInstance = std::make_shared<HiCR::backend::cloudr::Instance>(instanceIdCounter, instance.get(), isRoot);

     // Adding instance to the collection
     _instances.push_back(newInstance);

     // Increasing instance Id
     instanceIdCounter++;
    }

    // Print root status
    if (_instanceManager->getRootInstanceId() != _instanceManager->getCurrentInstance()->getId()) 
    {
      printf("I am not root instance -- Listening\n");
      _continueListening = true;
      while(_continueListening) _rpcEngine->listen();
    } 

  }

  __INLINE__ void setInstanceTopologies(const nlohmann::json& instanceTopologiesJs)
  {
    // This function will only be ran by the root rank
    if (_instanceManager->getRootInstanceId() != _instanceManager->getCurrentInstance()->getId()) return;

    // Storing instance topologies
    setInstanceTopologiesImpl(instanceTopologiesJs);
  }

  __INLINE__ void startService()
  {
    // This function will only be ran by the root rank
    if (_instanceManager->getRootInstanceId() != _instanceManager->getCurrentInstance()->getId()) return;
  }

  __INLINE__ void stopService()
  {
    // This function will only be ran by the root rank
    if (_instanceManager->getRootInstanceId() != _instanceManager->getCurrentInstance()->getId()) return;

    // Requesting others to shut down
    for (auto& instance : _instances) _rpcEngine->requestRPC(*instance, __CLOUDR_FINALIZE_WORKER_RPC_NAME);
  }

  __INLINE__ void finalize() override
  {
    // Finalizing underlying instance manager
    _instanceManager->finalize();
  } 

  __INLINE__ void abort(int errorCode) override { _instanceManager->abort(errorCode); }

  [[nodiscard]] __INLINE__ HiCR::Instance::instanceId_t getRootInstanceId() const override { return 0; }

  protected:

  __INLINE__ std::shared_ptr<HiCR::Instance> createInstanceImpl [[noreturn]] (const std::shared_ptr<HiCR::InstanceTemplate> &instanceTemplate) override
  {
    HICR_THROW_LOGIC("The MPI backend does not currently support the launching of new instances during runtime");
  }

  __INLINE__ std::shared_ptr<HiCR::Instance> addInstanceImpl(HiCR::Instance::instanceId_t instanceId) override
  {
    HICR_THROW_LOGIC("The Host backend does not currently support the detection of new instances during runtime");
  }

  private:

  __INLINE__ void setInstanceTopologiesImpl(const nlohmann::json& instanceTopologiesJs)
  {
    // Getting array of topologies
    auto instanceTopologies = hicr::json::getArray<nlohmann::json>(instanceTopologiesJs, "Instance Topologies");

    // Check whether the number of topologies passed coincides with the number of instances
    if (instanceTopologies.size() != _instances.size()) HICR_THROW_LOGIC("Trying to configure the topology of %lu instances, when %lu were actually created\n", instanceTopologies.size(), _instances.size());

    // Assigning topologies to each of the detected instances
    for (size_t i = 0; i < _instances.size(); i++)
    {
      // Grabbing the instance topology to emulate
      const auto& instanceTopologyJs = instanceTopologies[i];

      // Setting the instance's topology
      _instances[i]->setTopology(instanceTopologyJs);
    }
  }

  __INLINE__ void runMainFunction()
  {
    // Getting argc and argv values
    int argc = _args.size();
    const char* argv[_args.size()];
    for (size_t i = 0; i < _args.size(); i++) argv[i] = _args[i].c_str();
    int retVal = _mainFunction(argc, (char**)argv);
    printf("Retval: %d\n", retVal);
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

  /// Storage for launch arguments
  std::vector<std::string> _args;

  /// Storage for the main function to run when a new instance runs
  const mainFc_t _mainFunction;

  // Storage for the instance collection
  std::vector<std::shared_ptr<HiCR::backend::cloudr::Instance>> _instances;

  // Flag to signal non-root instances to finish listening
  bool _continueListening;

}; // class CloudR

} // namespace cloudr