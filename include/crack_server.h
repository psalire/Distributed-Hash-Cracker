
#ifndef _CRACK_SERVER_H_
#define _CRACK_SERVER_H_

#include <cryptopp/sha.h>
#include <thread>
#include <mutex>
#include <iostream>
#include "tcp.h"
#include "encoder.h"
#include "settings.h"

/* Hash_Algo is expected to be a CryptoPP hash class */
/*
   HashCrackServer inherits:
   Encoder for encoding/decoding hex string to byte string
   ThreadPool for cracking the hash
*/
class HashCrackServer {
    private:
        /* Properties */
        std::string hash_to_crack_,
                    search_space_,
                    hash_algo_,
                    cracked_hash_;
        int total_clients_,
            max_string_length_;
        bool use_fixed_string_length_,
             is_started_;
        std::atomic<bool> is_cracked_hash_;
        TCPComm socket_;
        std::vector<int> client_sockfds_;
        std::vector<std::thread> client_connections;
        std::mutex lock_cracked_hash_;
        
        /* Methods */
        void init_server(int);
        
    public:
        /* Constructors */
        HashCrackServer(std::string, std::string, std::string, int, int, int, bool);
        
        /* Methods */
        void accept_clients();
        void start();
        void wait_for_clients();
        /* Setters and getters */
        void set_hash_to_crack(std::string);
        void set_search_space(std::string);
        void set_hash_algo(std::string);
        void set_total_clients(int);
        void set_max_string_length(int);
        void set_use_fixed_string_length(bool);
        void set_is_started(bool);
        void set_is_cracked_hash(bool);
        void set_cracked_hash(std::string);
        bool get_is_cracked_hash();
        std::string get_cracked_hash();
};

#endif
