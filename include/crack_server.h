
#ifndef _CRACK_SERVER_H_
#define _CRACK_SERVER_H_

#include <cryptopp/sha.h>
#include <thread>
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
        std::string hash_to_crack,
                    search_space,
                    hash_algo;
        int total_clients,
            max_string_length;
        bool use_fixed_string_length,
             started;
        TCPComm socket;
        std::vector<int> client_sockfds;
        std::vector<std::thread> client_connections;
        
        /* Methods */
        void init_server(int);
        
    public:
        /* Constructors */
        HashCrackServer(std::string, std::string, std::string, int, int, int, bool);
        /* Destructor */
        ~HashCrackServer();
        
        /* Methods */
        void accept_clients();
        void start();
        void wait_for_clients();
};

#endif
