
#ifndef _ARGS_H_
#define _ARGS_H_

#include <iostream>
#include <algorithm>
#include <unistd.h>

typedef struct ArgsServer {
    std::string hash_to_crack,
                search_space,
                hash_algo,
                output_file;
    unsigned int max_string_len;
    bool use_fixed_str_len;
    int total_clients,
        port;
} ArgsServer;

typedef struct ArgsClient {
    int tot_threads,
        port;
    std::string server_ip_addr;
} ArgsClient;

class ArgParser {
    public:
        static ArgsServer server_get_args(int argc, char **argv);
        static ArgsClient client_get_args(int argc, char **argv);
        static void server_print_usage();
        static void client_print_usage();
};

#endif
