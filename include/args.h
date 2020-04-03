
#ifndef _ARGS_H_
#define _ARGS_H_

#include <iostream>
#include <algorithm>
#include <unistd.h>

typedef struct Args {
    std::string hash_to_crack,
                search_space,
                hash_algo,
                output_file;
    unsigned int max_string_len;
    bool use_fixed_str_len;
    int total_clients,
        port;
} Args;

class ArgParser {
    public:
        static Args server_get_args(int argc, char **argv);
        static Args client_get_args(int argc, char **argv);
        static void server_print_usage();
        static void client_print_usage();
};

#endif
