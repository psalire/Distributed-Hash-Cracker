
#include "args.h"

ArgsClient ArgParser::client_get_args(int argc, char **argv) {
    ArgsClient args;
    /* Initialize values */
    args.tot_threads = -1;
    args.server_ip_addr = "";
    args.port = -1;
    int opt;
    while ((opt = getopt(argc, argv, "i:p:n:h")) != -1) {
        switch(opt) {
            case 'n': {
                args.tot_threads = atoi(optarg);
                break;
            }
            case 'p': {
                args.port = atoi(optarg);
                break;
            }
            case 'i': {
                args.server_ip_addr = optarg;
                break;
            }
            case '?':
            case 'h': {
                client_print_usage();
                exit(0);
            }
        }
    }
    return args;
}

void ArgParser::client_print_usage() {
    std::cout << "Usage: main_client -i [Server IP Address] -p [Server Port] -n [Total Threads] \n"
              << "-i:\n    Required: IP address of server.\n"
              << "-p:\n    Required: Port number of server.\n"
              << "-n:\n    Required: Total threads to use.\n\n";
}
