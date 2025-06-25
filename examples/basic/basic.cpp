#include <stdio.h>
#include <fstream>
#include <hicr/backends/cloudr/instanceManager.hpp>
#include <hicr/backends/hwloc/device.hpp>

int cloudRMain(HiCR::backend::cloudr::InstanceManager *cloudr, int argc, char *argv[])
{
  bool isRoot = cloudr->getCurrentInstance()->getId() == cloudr->getRootInstanceId();
  if (isRoot)
    printf("Root: I am on main\n");
  else
    printf("Non-Root: I am on main\n");

  // If I'm root, create new instance
  if (isRoot)
  {
    // Creating HWloc topology object
    hwloc_topology_t topology;

    // Reserving memory for hwloc
    hwloc_topology_init(&topology);

    // Initializing HWLoc-based host (CPU) topology manager
    HiCR::backend::hwloc::TopologyManager tm(&topology);

    // Detecting my own local topology
    const auto localTopology = tm.queryTopology();

    // Creating a new (reduced) topology
    HiCR::Topology newTopology;

    // Adding a single device (NUMA Domain) to it
    auto newDevice = std::make_shared<HiCR::backend::hwloc::Device>();
    newDevice->addComputeResource(localTopology.getDevices().begin().operator*()->getComputeResourceList().begin().operator*());
    newDevice->addMemorySpace(localTopology.getDevices().begin().operator*()->getMemorySpaceList().begin().operator*());
    newTopology.addDevice(newDevice);

    // Requesting the creation of the new topology
    HiCR::InstanceTemplate it(newTopology);
    auto                   newInstance = cloudr->createInstance(it);
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