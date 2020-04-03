
#include "args.h"

Args get_args(int argc, char **argv) {
    Args args;
    /* Initialize values */
    args.search_space = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`!@#$%^&*()-=~_+[]\\{}|;':\",./<>?";
    args.hash_to_crack = "";
    args.hash_algo = "";
    args.total_clients = -1;
    args.max_string_len = 0;
    args.use_fixed_str_len = false;
    args.port = -1;
    int opt;
    while ((opt = getopt(argc, argv, "i:s:l:n:a:x:c:p:fh")) != -1) {
        switch(opt) {
            case 'i': {
                args.hash_to_crack = optarg;
                break;
            }
            case 's': {
                args.search_space = optarg;
                break;
            }
            case 'l': {
                args.max_string_len = atoi(optarg);
                break;
            }
            case 'n': {
                args.total_clients = atoi(optarg);
                break;
            }
            case 'p': {
                args.port = atoi(optarg);
                break;
            }
            case 'x': {
                for (char c : std::string(optarg)) {
                    args.search_space.erase(
                        std::remove(
                            args.search_space.begin(),
                            args.search_space.end(),
                            c
                        ),
                        args.search_space.end()
                    );
                }
                break;
            }
            case 'c': {
                for (char c : std::string(optarg)) {
                    if (args.search_space.find(c) == std::string::npos) {
                        args.search_space.push_back(c);
                    }
                }
                break;
            }
            case 'a': {
                args.hash_algo = optarg;
                break;
            }
            case 'f': {
                args.use_fixed_str_len = true;
                break;
            }
            case '?':
            case 'h': {
                print_usage();
                exit(0);
            }
        }
    }
    return args;
}

void print_usage() {
    std::cout << "Usage: main -i [Checksum string] -a [Hash algorithm] -n [Total clients]\n"
              << "-i:\n    Required: Hash/checksum to crack.\n"
              << "-a:\n    Required: Hash algorithm to use.\n"
              << "-n:\n    Required: Total clients to accept.\n"
              << "-p:\n    Required: Port number to create server on.\n"
              << "-s:\n    Optional: Set search space.\n    Default:\n"
                 "    \"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRST"
                 "UVWXYZ0123456789`!@#$%^&*()-=~_+[]\\{}|;':\",./<>?\"\n"
              << "-x:\n    Optional: Exclude/remove characters from search space.\n"
              << "-c:\n    Optional: Include/add characters to search space. Duplicates ignored. \n"
              << "-l:\n    Optional: Set a max string length for cracking.\n"
                 "    Default: 0, meaning no max; try all possible string lengths.\n"
              << "-f:\n    Optional: Fixed string length; Only crack strings of length max string length.\n\n";
}
