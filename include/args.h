
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

Args get_args(int argc, char **argv);
void print_usage();

#endif
