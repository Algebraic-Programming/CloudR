#include <stdio.h>
#include <fstream>
#include <hicr/backends/cloudr/instanceManager.hpp>

int cloudRMain(int argc, char* argv[])
{
    printf("I am on main\n");
    return 0;
}

int main(int argc, char *argv[])
{
    HiCR::backend::cloudr::InstanceManager cloudr(cloudRMain);
    cloudr.initialize(&argc, &argv);

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

    // Starting cloudr service
    cloudr.startService();

    // Stopping cloudr service();
    cloudr.stopService();

    // Finalizing cloudr
    cloudr.finalize();
}