
#ifndef _CRACK_CLIENT_H_
#define _CRACK_CLIENT_H_

#include <cryptopp/sha.h>
#include <cryptopp/sha3.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include "encoder.h"
#include "tcp.h"
#include "settings.h"

/* Handle connecting to and receving from server */
class HashCrackClient {
    private:
        TCPComm socket;
        Settings settings;
        int tot_threads;
        bool is_connected;
        
    public:
        /* Constructor */
        HashCrackClient(int);
        
        /* Methods */
        bool connect_to_server(const char *, int);
        void send_results_to_server(bool b, std::string);
        bool recv_message(void *, int);
        void close_socket();
        Settings get_settings();
        bool get_is_connected();
};

/* Hash_Algo is expected to be a CryptoPP hash class */
/*
   HashCrack inherits:
   Encoder for encoding/decoding hex string to byte string
*/
template <typename HashAlgo> class HashCrack : public Encoder {
    private:
        /* Properties */
        std::string cracked_hash,
                    search_space,
                    prefixes;
        byte *hash_to_crack_byte_arr;
        int tot_threads,
            max_string_len;
        std::atomic<bool> is_cracked,
                          is_done;
        bool use_fixed_str_len;
        #if defined DEBUG || defined VERBOSE
        std::mutex lock_stdout;
        #endif
        #ifdef VERBOSE
        int curr_length;
        #endif
        
        /* Methods */
        bool crack(std::string="", int=0);
        bool hash_all_strings(std::string, int, std::string);
        // void crack(std::string="");

    public:
        /* Constructors */
        HashCrack(Settings, int);
        HashCrack(std::string, std::string, std::string, int, int, bool);

        /* Methods */
        void multithreaded_crack(HashCrackClient);
        bool check_hash_match(std::string);
        
        /* Setters and getters */
        void set_hash_to_crack(std::string);
        void set_search_space(std::string);
        void set_max_string_len(int);
        void set_use_fixed_string_len(bool);
        void set_tot_threads(int);
        void set_is_cracked(bool);
        void set_is_done(bool);
        std::string get_cracked_hash();
        bool get_is_cracked();
        bool get_is_done();
};

#endif
