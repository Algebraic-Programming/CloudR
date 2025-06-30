#include <stdio.h>
#include <fstream>
#include <hicr/backends/cloudr/instanceManager.hpp>
#include <hicr/backends/cloudr/communicationManager.hpp>
#include <hicr/backends/hwloc/device.hpp>

int cloudRMain(HiCR::backend::cloudr::InstanceManager *cloudr, int argc, char *argv[])
{
  bool isRoot = cloudr->getCurrentInstance()->getId() == cloudr->getRootInstanceId();
  if (isRoot)
    printf("Root: I am on main\n");
  else
    printf("Non-Root: I am on main\n");

  auto memoryManager = cloudr->getMemoryManager();
  auto communicationManager = cloudr->getCommunicationManager();
  auto topologyManager = cloudr->getTopologyManager();

  // Getting current process id
  auto myInstanceId = cloudr->getCurrentInstance()->getId();

  // Asking backend to check the available devices
  const auto topology = topologyManager->queryTopology();

  // Getting first device found
  auto device = *topology.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = device->getMemorySpaceList();

  // Getting the first memory space we found
  auto firstMemSpace = *memSpaces.begin();

  // Creating memory slot and exchanging it before we create the other instance
  constexpr size_t bufferTag = 0;
  constexpr size_t bufferSize = 4096;

  // Allocating send/receive buffer
  auto bufferSlot = memoryManager->allocateLocalMemorySlot(firstMemSpace, bufferSize);

  // Exchanging communicator tag
  communicationManager->exchangeGlobalMemorySlots(bufferTag, {{myInstanceId, bufferSlot}});

  // Synchronizing so that all actors have finished registering their global memory slots
  communicationManager->fence(bufferTag);

  // If I'm root, create new instance
  if (isRoot)
  {
    // Requesting the creation of the new topology
    auto newInstance = cloudr->createInstance();
    if (newInstance == nullptr)
    {
      fprintf(stderr, "Error: Failed to create new instance.\n");
      return -1;
    }
  }

  return 0;
}

int main(int argc, char *argv[])
{
  HiCR::backend::cloudr::InstanceManager cloudr(cloudRMain);
  cloudr.initialize(&argc, &argv);

  // Checking if I'm root
  bool isRoot = cloudr.getCurrentInstance()->getId() == cloudr.getRootInstanceId();

  // Only do the following if I'm root
  if (isRoot)
  {
    // Checking arguments
    if (argc != 2)
    {
      fprintf(stderr, "Error: Must provide a JSON file with a list of instance topologies as argument.\n");
      cloudr.abort(-1);
      return -1;
    }

    // Getting request file name from arguments
    std::string instanceTopologiesFilePath = std::string(argv[1]);

    // Parsing request file contents to a JSON object
    std::ifstream ifs(instanceTopologiesFilePath);
    auto          instanceTopologiesJs = nlohmann::json::parse(ifs);

    // Configuring emulated instance topologies
    cloudr.setInstanceTopologies(instanceTopologiesJs);

    // Calling what's supposed to be cloudR main
    cloudRMain(&cloudr, argc, argv);
  }

  // Finalizing cloudr
  cloudr.finalize();
}