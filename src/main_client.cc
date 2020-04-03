
#include <iostream>
#include <cryptopp/sha.h>
#include <cryptopp/files.h>
#include "crack_client.h"
#include "args.h"

int main(int argc, char **argv) {
    
    Args args = ArgParser::client_get_args(argc, argv);
    
    /* If missing required args, exit */
    if (!args.hash_to_crack.size() || !args.hash_algo.size() || args.total_clients < 0) {
        puts("Error: Missing required arguments.");
        ArgParser::client_print_usage();
        exit(EXIT_FAILURE);
    }
    else if (args.total_clients == 0) {
        puts("Error: Total clients must be >0.");
        exit(EXIT_FAILURE);
    }
    #ifdef SERVER_VERBOSE
    std::cout
        << "Hash to crack       : \"" << args.hash_to_crack << "\"\n"
        << "Hash Algorithm      : "   << args.hash_algo << "\n"
        << "Search space        : \"" << args.search_space << "\"\n"
        << "Max string length   : "   << (!args.max_string_len ? "None (Test all string lengths)" : std::to_string(args.max_string_len)) << "\n"
        << "Fixed string length : "   << (args.use_fixed_str_len ? "True" : "False") << "\n"
        << "Total clients       : "   << args.total_clients << "\n\n";
    #endif
    if (args.hash_algo == "SHA1") {
        
    }
    else if (args.hash_algo == "SHA224") {
        
    }
    else if (args.hash_algo == "SHA256") {
        
    }
    else if (args.hash_algo == "SHA384") {
        
    }
    else if (args.hash_algo == "SHA512") {
        
    }
    else if (args.hash_algo == "SHA3_224") {
        
    }
    else if (args.hash_algo == "SHA3_256") {
        
    }
    else if (args.hash_algo == "SHA3_384") {
        
    }
    else if (args.hash_algo == "SHA3_512") {
        
    }
    else if (args.hash_algo == "SHA3_224") {
        
    }
    
    return 0;
}
