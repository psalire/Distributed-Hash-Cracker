
#include <iostream>
#include <fstream>
#include <chrono>
#include "crack_server.h"
#include "args.h"

int main(int argc, char **argv) {

    ArgsServer args = ArgParser::server_get_args(argc, argv);

    /* If missing required args, exit */
    if (!args.hash_to_crack.size() || !args.hash_algo.size() || args.total_clients <= 0
        || args.port < 0 || args.output_file == "" || (args.hash_algo != "SHA1" &&
         args.hash_algo != "SHA224" && args.hash_algo != "SHA256" && args.hash_algo != "SHA384" &&
         args.hash_algo != "SHA512" && args.hash_algo != "SHA3_224" && args.hash_algo != "SHA3_256" &&
         args.hash_algo != "SHA3_384" && args.hash_algo != "SHA3_512" && args.hash_algo != "SHA3_224")) {
        puts("Error: Missing/invalid required arguments.");
        ArgParser::server_print_usage();
        exit(EXIT_FAILURE);
    }

    #ifdef SERVER_VERBOSE
    std::cout
        << "\nHash to crack       : " << args.hash_to_crack << "\n"
        << "Hash Algorithm      : "     << args.hash_algo << "\n"
        << "Search space        : \""   << args.search_space << "\"\n"
        << "Max string length   : "     << (!args.max_string_len ? "None (Test all string lengths)" : std::to_string(args.max_string_len)) << "\n"
        << "Fixed string length : "     << (args.use_fixed_str_len ? "True" : "False") << "\n"
        << "Total clients       : "     << args.total_clients << "\n"
        << "Port number         : "     << args.port << "\n"
        << "Output File         : "     << args.output_file << "\n\n";
    #endif

    HashCrackServer crack_server(
        args.hash_to_crack,
        args.search_space,
        args.hash_algo,
        args.total_clients,
        args.port,
        args.max_string_len,
        args.use_fixed_str_len
    );

    std::cout << "[SERVER] Accepting " << args.total_clients << " clients...\n";
    crack_server.accept_clients();
    
    std::cout << "[SERVER] Initializing all clients and client comms...\n";
    crack_server.start();
    
    std::cout << "[SERVER] Clients are computing, waiting for results...\n";
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    crack_server.wait_for_clients();
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    std::cout << "[SERVER] Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0 << "s\n";
    
    if (crack_server.get_is_cracked_hash()) {
        std::cout << "[SERVER] Client successfully cracked " << args.hash_to_crack << "\n";
        try {
            std::ofstream out(args.output_file);
            out << crack_server.get_cracked_hash();
            out.close();
            std::cout << "[SERVER] Saved crack to " << args.output_file << ".\n";
        }
        catch (...) {
            std::cout << "[SERVER] ERROR: Could not write to " << args.output_file << "\n";
            std::cout << "[SERVER] Outputting crack to stdout:\n";
            std::cout << crack_server.get_cracked_hash();
        }
    }
    else {
        std::cout << "[SERVER] Clients failed to crack hash\n";
    }
    return 0;
}
