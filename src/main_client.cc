
#include <iostream>
#include <cryptopp/sha.h>
#include <cryptopp/files.h>
#include <chrono>
#include "crack_client.h"
#include "args.h"

template <typename HashAlgo> static void do_crack(HashCrackClient &client_comm, Settings &settings, int tot_threads) {
    /* Instantiate HashCrack and do multithreaded_crack() */
    HashCrack<HashAlgo> hash_crack(settings, tot_threads);
    std::cout << "[CLIENT] Cracking hash...\n";
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    hash_crack.multithreaded_crack(client_comm);
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    std::cout << "[CLIENT] Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0 << "s\n";
    /* Send results to server */
    client_comm.send_results_to_server(hash_crack.get_is_cracked(), hash_crack.get_cracked_hash());
}

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
        << "\nServer IP Address : " << args.server_ip_addr << "\n"
        << "Server port       : " << args.port << "\n"
        << "Total Threads     : " << args.tot_threads << "\n\n";
    #endif
    
    /* Connect to server */
    std::cout << "[CLIENT] Connecting to server at " << args.server_ip_addr << ":" << args.port << "...\n";
    HashCrackClient client_comm(args.tot_threads);
    if (!client_comm.connect_to_server(args.server_ip_addr.c_str(), args.port)) {
        exit(EXIT_FAILURE);
    }
    assert(client_comm.get_is_connected());
    
    /* Crack the hash */
    Settings settings = client_comm.get_settings();
    #ifdef CLIENT_VERBOSE
    std::cout << "[INFO] Hash Algo: " << settings.hash_algo << "\n";
    #endif
    if (strcmp(settings.hash_algo, "SHA1") == 0) {
        do_crack<CryptoPP::SHA1>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA224") == 0) {
        do_crack<CryptoPP::SHA224>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA256") == 0) {
        do_crack<CryptoPP::SHA256>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA384") == 0) {
        do_crack<CryptoPP::SHA384>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA512") == 0) {
        do_crack<CryptoPP::SHA512>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA3_224") == 0) {
        do_crack<CryptoPP::SHA3_224>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA3_256") == 0) {
        do_crack<CryptoPP::SHA3_256>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA3_384") == 0) {
        do_crack<CryptoPP::SHA3_384>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA3_512") == 0) {
        do_crack<CryptoPP::SHA3_512>(client_comm, settings, args.tot_threads);
    }
    else if (strcmp(settings.hash_algo, "SHA3_224") == 0) {
        do_crack<CryptoPP::SHA3_224>(client_comm, settings, args.tot_threads);
    }
    else {
        printf("[ERROR] Hash algo \"%s\" not found. Exiting...", settings.hash_algo);
        client_comm.close_socket();
        exit(EXIT_FAILURE);
    }
    
    client_comm.close_socket();
    return 0;
}
