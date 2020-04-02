
#include <iostream>
#include <cryptopp/sha.h>
#include <cryptopp/files.h>
#include "crack_server.h"
#include "args.h"

// -n Number of machines
// cracks computing while waiting for clients to connect
// Waiting for 4 machines...
// Waiting for 1 machine...
// All machines connected.

int main(int argc, char **argv) {
    
    Args args = get_args(argc, argv);
    
    /* If missing required args, exit */
    if (!args.hash_to_crack.size() || !args.hash_algo.size() || args.total_clients < 0) {
        puts("\nError: Missing required arguments.\n");
        print_usage();
        exit(EXIT_FAILURE);
    }
    std::cout
        << "Hash to crack       : \"" << args.hash_to_crack << "\"\n"
        << "Hash Algorithm      : " << args.hash_algo << "\n"
        << "Search space        : \"" << args.search_space << "\"\n"
        << "Max string length   : " << (!args.max_string_len ? "None (Test all string lengths)" : std::to_string(args.max_string_len)) << "\n"
        << "Fixed string length : " << (args.use_fixed_str_len ? "True" : "False") << "\n"
        << "Total clients       : " << args.total_clients << "\n\n";
    
    HashCrackServer crack(
        // "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", // abc
        // "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb", // a
        // "fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603", // ab
        "fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603", // ab
        args.total_clients
    );
    
    return 0;
}
