
#include <iostream>
#include <cryptopp/sha.h>
#include <cryptopp/files.h>
#include "crack_client.h"
#include "args.h"

int main(int argc, char **argv) {
    
    ArgsClient args = ArgParser::client_get_args(argc, argv);
    
    /* If missing required args, exit */
    if (args.tot_threads <= 0 || args.port < 0) {
        puts("Error: Missing/invalid required arguments.");
        ArgParser::client_print_usage();
        exit(EXIT_FAILURE);
    }
    #ifdef CLIENT_VERBOSE
    std::cout
        << "\nServer IP Address: " << args.server_ip_addr << "\n"
        << "Server port        : " << args.port << "\n"
        << "Total Threads      : " << args.tot_threads << "\n\n";
    #endif
    
    HashCrackClient client_comm(args.tot_threads);
    
    /* if (args.hash_algo == "SHA1") {
        
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
        
    } */
    
    return 0;
}
