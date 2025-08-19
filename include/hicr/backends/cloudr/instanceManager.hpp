
#pragma once

#include <functional>
#include <nlohmann_json/json.hpp>
#include <nlohmann_json/parser.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/instanceManager.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/frontends/RPCEngine/RPCEngine.hpp>
#include "instance.hpp"

namespace HiCR::backend::cloudr
{

class CommunicationManager;
class TopologyManager;

class InstanceManager final : public HiCR::InstanceManager
{
  friend TopologyManager;
  friend CommunicationManager;

  public:

#define __CLOUDR_LAUNCH_ENTRY_POINT_RPC_NAME "[CloudR] Launch Main"
#define __CLOUDR_FINALIZE_WORKER_RPC_NAME "[CloudR] Finalize Worker"
#define __CLOUDR_REQUEST_TOPOLOGY_RPC_NAME "[CloudR] Request Topology"

// Communication Manager RPCs
#define __CLOUDR_EXCHANGE_GLOBAL_MEMORY_SLOTS_RPC_NAME "[CloudR] Exchange Global Memory Slots"
#define __CLOUDR_FENCE_RPC_NAME "[CloudR] Fence"

  typedef std::function<void(void)> entryPoint_t;

  InstanceManager(HiCR::frontend::RPCEngine*rpcEngine): HiCR::InstanceManager(), _rpcEngine(rpcEngine) {}
  ~InstanceManager() = default;

  __INLINE__ void initialize()
  {
    // Registering launch function
    auto launchEntryPointExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { runEntryPointRPC(); });
    _rpcEngine->addRPCTarget(__CLOUDR_LAUNCH_ENTRY_POINT_RPC_NAME, launchEntryPointExecutionUnit);

    // Registering finalize function
    auto finalizeWorkerExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { finalizeWorker(); });
    _rpcEngine->addRPCTarget(__CLOUDR_FINALIZE_WORKER_RPC_NAME, finalizeWorkerExecutionUnit);

    // Registering request topology function
    auto requestTopologyExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { requestTopology(); });
    _rpcEngine->addRPCTarget(__CLOUDR_REQUEST_TOPOLOGY_RPC_NAME, requestTopologyExecutionUnit);

    // Registering exchange global memory slots RPC
    auto exchangeGlobalMemorySlotsExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { exchangeGlobalMemorySlotsRPC(); });
    _rpcEngine->addRPCTarget(__CLOUDR_EXCHANGE_GLOBAL_MEMORY_SLOTS_RPC_NAME, exchangeGlobalMemorySlotsExecutionUnit);

    // Registering global fence
    auto fenceExecutionUnit = HiCR::backend::pthreads::ComputeManager::createExecutionUnit([this](void *) { fenceRPC(); });
    _rpcEngine->addRPCTarget(__CLOUDR_FENCE_RPC_NAME, fenceExecutionUnit);

    // Creating instance objects from the initially found instances now
    HiCR::Instance::instanceId_t instanceIdCounter = 0;
    for (auto &instance : _rpcEngine->getInstanceManager()->getInstances())
    {
      // Only the current instance is the root one
      const bool isRoot = _rpcEngine->getInstanceManager()->getRootInstanceId() == instance->getId();

      // Creating new cloudr instance object (contains all the emulated information)
      auto newInstance = std::make_shared<HiCR::backend::cloudr::Instance>(instanceIdCounter, instance.get(), isRoot);

      // Adding new instance to the internal storage
      _cloudrInstances.push_back(newInstance);

      // If this is the current instance, set it now
      if (instance->getId() == _rpcEngine->getInstanceManager()->getCurrentInstance()->getId()) setCurrentInstance(newInstance);

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
    // printf("[CloudR] Worker %lu finished.\n", _rpcEngine->getInstanceManager()->getCurrentInstance()->getId());
  }

  __INLINE__ void deploy()
  {
    // Main loop for running instances
    if (_rpcEngine->getInstanceManager()->getCurrentInstance()->isRootInstance() == false)
    {
      while (_continueListening)
      {
        // printf("[CloudR] Worker %lu listening...\n", _rpcEngine->getInstanceManager()->getCurrentInstance()->getId());
        _rpcEngine->listen();
        // printf("[CloudR] Worker %lu back from listening...\n", _rpcEngine->getInstanceManager()->getCurrentInstance()->getId());
      }
    }
    else // If I am root, go straight to the entry point
    {
      _entryPoint();
    }
  }

  __INLINE__ void setConfiguration(const nlohmann::json &configurationJs)
  {
    // This function will only be ran by the root rank
    if (_rpcEngine->getInstanceManager()->getRootInstanceId() != _rpcEngine->getInstanceManager()->getCurrentInstance()->getId()) return;

    // Getting array of topologies
    auto instanceTopologies = hicr::json::getArray<nlohmann::json>(configurationJs, "Instance Topologies");

    // Check whether the number of topologies passed coincides with the number of instances
    if (instanceTopologies.size() != _cloudrInstances.size())
      HICR_THROW_LOGIC("Trying to configure the topology of %lu instances, when %lu were actually created\n", instanceTopologies.size(), _cloudrInstances.size());

    // Assigning topologies to each of the detected instances
    for (size_t i = 0; i < _cloudrInstances.size(); i++)
    {
      // Grabbing the instance topology to emulate
      const auto &instanceTopologyJs = instanceTopologies[i];

      // Setting the instance's topology
      const auto &cloudrInstance = _cloudrInstances[i].get();
      cloudrInstance->setTopology(instanceTopologyJs);

      // Store our local topology for later reference
      if (cloudrInstance->getBaseInstance()->getId() == _rpcEngine->getInstanceManager()->getCurrentInstance()->getId()) 
        _localTopologyJs = instanceTopologyJs;
    }
  }

  __INLINE__ void finalize() override
  {
    // The following only be ran by the root rank, send an RPC to all others to finalize them
    if (_rpcEngine->getInstanceManager()->getRootInstanceId() == _rpcEngine->getInstanceManager()->getCurrentInstance()->getId())
    {
      // printf("[Root] Finalizing CloudR...\n");
      for (auto &instance : _cloudrInstances)
        if (instance->isRootInstance() == false) _rpcEngine->requestRPC(*instance, __CLOUDR_FINALIZE_WORKER_RPC_NAME);
    }
  }

  __INLINE__ void abort(int errorCode) override { _rpcEngine->getInstanceManager()->abort(errorCode); }

  [[nodiscard]] __INLINE__ HiCR::Instance::instanceId_t getRootInstanceId() const override { return _rootInstance->getId(); }
  [[nodiscard]] __INLINE__ auto        getRPCEngine() const { return _rpcEngine; }
  [[nodiscard]] __INLINE__ const auto &getFreeInstances() const { return _freeInstances; }
  __INLINE__ void setEntryPoint(const entryPoint_t entryPoint) { _entryPoint = entryPoint; }

  private:
  
  [[nodiscard]] __INLINE__ auto getLocalInstanceTopologyJs() const { return _localTopologyJs; }

  __INLINE__ std::shared_ptr<HiCR::Instance> createInstanceImpl(const HiCR::InstanceTemplate instanceTemplate) override
  {
    // If no more free instances available, fail now
    // Commented out because we don't want to fail, simply return a nullptr
    // if (_freeInstances.empty()) HICR_THROW_LOGIC("Requested the creation of a new instances, but CloudR has ran out of free instances");

    // Creating instance object to return
    std::shared_ptr<HiCR::backend::cloudr::Instance> newInstance = nullptr;

    // Getting requested topology from the instance template
    const auto &topology = instanceTemplate.getTopology();

    // Iterating over free instances to get the first one that satisfies the request
    for (const auto &instance : _freeInstances)
      if (instance->isCompatible(topology))
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
      _rpcEngine->requestRPC(*newInstance->getBaseInstance(), __CLOUDR_LAUNCH_ENTRY_POINT_RPC_NAME);

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

  __INLINE__ void requestExchangeGlobalMemorySlots(HiCR::GlobalMemorySlot::tag_t tag)
  {
    // Asking free instances to run the exchange RPC
    for (const auto &instance : _freeInstances) _rpcEngine->requestRPC(*instance, __CLOUDR_EXCHANGE_GLOBAL_MEMORY_SLOTS_RPC_NAME, tag);
  }

  __INLINE__ void requestFence(HiCR::GlobalMemorySlot::tag_t tag)
  {
    // Asking free instances to run the exchange RPC
    for (const auto &instance : _freeInstances) _rpcEngine->requestRPC(*instance, __CLOUDR_FENCE_RPC_NAME, tag);
  }

  __INLINE__ void runEntryPointRPC()
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
    _rpcEngine->getMemoryManager()->freeLocalMemorySlot(returnValue);

    // Updating current instance's topology
    static_cast<HiCR::backend::cloudr::Instance *>(getCurrentInstance().get())->setTopology(topologyJson);

    // Running main function
    _entryPoint();
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
    // printf("[CloudR] Worker %lu running finalizeWorker() RPC.\n", _rpcEngine->getInstanceManager()->getCurrentInstance()->getId());
    _continueListening = false;
  }

  __INLINE__ void exchangeGlobalMemorySlotsRPC()
  {
    const auto exchangeTag = _rpcEngine->getRPCArgument();
    _rpcEngine->getCommunicationManager()->exchangeGlobalMemorySlots(exchangeTag, {});
  }

  __INLINE__ void fenceRPC()
  {
    const auto exchangeTag = _rpcEngine->getRPCArgument();
    _rpcEngine->getCommunicationManager()->fence(exchangeTag);
  }

  /// RPC engine
  HiCR::frontend::RPCEngine* const _rpcEngine;

  /// Hwloc topology object
  hwloc_topology_t _hwlocTopology;

  /// Storage for the main function to run when a new instance runs
  entryPoint_t _entryPoint = [&]()
  {
    fprintf(stderr, "[CloudR] Error: Trying to run CloudR without having defined an entry point.");
    _rpcEngine->getInstanceManager()->abort(-8);
  };

  // Pointer to the root instance
  HiCR::backend::cloudr::Instance *_rootInstance;

  // Flag to signal non-root instances to finish listening
  bool _continueListening = true;

  // Map that links the underlying instance ids with the cloudr instances
  std::map<HiCR::Instance::instanceId_t, HiCR::backend::cloudr::Instance *> _baseIdsToCloudrInstanceMap;

  /// Internal collection of cloudr instances
  std::vector<std::shared_ptr<HiCR::backend::cloudr::Instance>> _cloudrInstances;

  /// A collection of ready-to-use instances currently on standby
  std::set<std::shared_ptr<HiCR::backend::cloudr::Instance>> _freeInstances;

  /// Storage for the local instance's topology
  nlohmann::json _localTopologyJs;

}; // class CloudR

} // namespace HiCR::backend::cloudr