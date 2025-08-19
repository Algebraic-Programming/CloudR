#include <fstream>
#include <nlohmann_json/json.hpp>
#include <hicr/backends/cloudr/instanceManager.hpp>
#include <hicr/backends/cloudr/communicationManager.hpp>
#include <hicr/backends/cloudr/topologyManager.hpp>
#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

int main(int argc, char *argv[])
{
    // Instantiating base managers
  auto instanceManager         = HiCR::backend::mpi::InstanceManager::createDefault(&argc, &argv);
  auto communicationManager    = HiCR::backend::mpi::CommunicationManager(MPI_COMM_WORLD);
  auto memoryManager           = HiCR::backend::mpi::MemoryManager();
  auto computeManager          = HiCR::backend::pthreads::ComputeManager();

  // Checking if I'm root
  bool isRoot = instanceManager->getCurrentInstance()->isRootInstance();

  // Checking arguments
  if (argc != 2)
  {
    if (isRoot == true) fprintf(stderr, "Error: Must provide a CloudR JSON configuration file.\n");
    instanceManager->abort(-1);
    return -1;
  }

  // Getting CloudR configuration file name from arguments
  std::string cloudrConfigurationFilePath = std::string(argv[1]);

  // Reserving memory for hwloc
  hwloc_topology_t hwlocTopology;
  hwloc_topology_init(&hwlocTopology);

  // Initializing HWLoc-based host (CPU) topology manager
  auto hwlocTopologyManager = HiCR::backend::hwloc::TopologyManager(&hwlocTopology);

  // Finding the first memory space and compute resource to create our RPC engine
  const auto& topology           = hwlocTopologyManager.queryTopology();
  const auto& firstDevice        = topology.getDevices().begin().operator*();
  const auto& RPCMemorySpace     = firstDevice->getMemorySpaceList().begin().operator*();
  const auto& RPCComputeResource = firstDevice->getComputeResourceList().begin().operator*();
  
  // Instantiating RPC engine
  HiCR::frontend::RPCEngine rpcEngine(communicationManager, *instanceManager, memoryManager, computeManager, RPCMemorySpace, RPCComputeResource);

  // Initializing RPC engine
  rpcEngine.initialize();

  // Instanciating CloudR
  auto cloudrInstanceManager = HiCR::backend::cloudr::InstanceManager(&rpcEngine);
  auto cloudrTopologyManager = HiCR::backend::cloudr::TopologyManager(&cloudrInstanceManager);

  // Lambda representing cloudr's entry point
  auto entryPoint = [&]()
  {
    printf("Hello\n");
  };

  // Initializing CloudR
  cloudrInstanceManager.initialize();

  // Setting CloudR's entry point
  cloudrInstanceManager.setEntryPoint(entryPoint);

  // If I am the root, I need to configure the CloudR environment
  if (isRoot)
  {
    // Parsing CloudR configuration file contents to a JSON object
    std::ifstream ifs(cloudrConfigurationFilePath);
    auto          cloudrConfigurationJs = nlohmann::json::parse(ifs);

    // Configuring emulated instance topologies
    cloudrInstanceManager.setConfiguration(cloudrConfigurationJs);
  }

  // Deploying CloudR
  cloudrInstanceManager.deploy();
  
  // Finalizing cloudR
  cloudrInstanceManager.finalize();

  // Finalizing base instance manager
  instanceManager->finalize();
}
