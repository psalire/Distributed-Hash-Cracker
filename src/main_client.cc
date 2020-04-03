
#include <iostream>
#include <cryptopp/sha.h>
#include <cryptopp/files.h>
#include "crack_server.h"
#include "args.h"

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
    
    return 0;
}
