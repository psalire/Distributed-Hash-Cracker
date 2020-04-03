
#include "crack_server.h"

HashCrackServer::HashCrackServer(std::string checksum_hex_string, std::string ss, std::string a, int t, int p, int m, bool b) {
    if (t <= 0) {
        throw std::runtime_error("invalid client count");
    }
    set_is_started(false);
    set_hash_to_crack(checksum_hex_string);
    set_hash_algo(a);
    set_total_clients(t);
    set_max_string_length(m);
    set_use_fixed_string_length(b);
    set_search_space(ss);
    socket = TCPComm();
    init_server(p);
}

void HashCrackServer::init_server(int port) {
    socket.bind_socket(port);
    socket.listen_socket(total_clients);
}

void HashCrackServer::accept_clients() {
    set_is_started(false); // Assume starts a new session
    /* Accept all clients */
    for (int i = 0; i < total_clients; i++) {
        #ifdef SERVER_VERBOSE
        printf("[INFO] Accepting client #%d...\n", i);
        #endif
        client_sockfds[i] = socket.accept_connection();
        #ifdef SERVER_VERBOSE
        printf("[INFO] Accepted client #%d\n", i);
        #endif
    }
}

void HashCrackServer::start() {
    set_is_started(true);
    /* Divide search space to assign prefixes */
    std::vector<std::string> divided_searchspace;
    int div = search_space.size() / total_clients;
    if ((unsigned int ) total_clients > search_space.size() || !div) {
        throw std::runtime_error("tot_threads cannot be greater than length of search_space");
    }
    /* Divide search_space into total_clients number of subdivided_searchspace */
    for (unsigned int i = 0; i < search_space.size(); i += div) {
        divided_searchspace.push_back(std::string(search_space, i, div));
    }
    /* Redistribute remainder */
    while (divided_searchspace.size() > (unsigned int) total_clients) {
        std::string rem = divided_searchspace.back();
        divided_searchspace.pop_back();
        for (unsigned int i = divided_searchspace.size() - rem.size(), j = 0; i < divided_searchspace.size(); i++, j++) {
            divided_searchspace[i] += rem[j];
        }
    }
    assert(divided_searchspace.size() == (unsigned int) total_clients);
    
    /* Send to each client their prefixes, hash_to_crack, and search_space */
    Settings settings;
    memset(&settings, 0, sizeof(settings));
    strcpy(settings.search_space, search_space.c_str());
    strcpy(settings.hash_to_crack, hash_to_crack.c_str());
    strcpy(settings.hash_algo, hash_algo.c_str());
    memcpy(&settings.max_string_len, &max_string_length, sizeof(max_string_length));
    memcpy(&settings.use_fixed_string_len, &use_fixed_string_length, sizeof(use_fixed_string_length));
    /* Send settings with designated prefix to each client */
    for (int i = 0; i < total_clients; i++) {
        strcpy(settings.prefixes, divided_searchspace[i].c_str());
        socket.send_message(client_sockfds[i], (void *) &settings, sizeof(settings));
        /* Spawn thread to wait for reply of results for each client */
        client_connections.push_back(std::thread([&, i]() {
            /* Receive results from client */
            Results results;
            memset(&results, 0, sizeof(results));
            socket.recv_message(client_sockfds[i], &results, sizeof(results));
            if (results.success) {
                char *cracked = new char[results.string_len];
                socket.recv_message(client_sockfds[i], cracked, results.string_len);
                #ifdef SERVER_VERBOSE
                printf("[INFO] Client #%d cracked the hash.\n", i);
                printf("[INFO] CRACKED HASH: %s\n", cracked);
                #endif
                delete[] cracked;
                /* Signal all other clients that done */
                const char *done = "DONE";
                for (int j = 0; j < total_clients; j++) {
                    if (j != i) {
                        std::cout << "[INFO] Server send message " << j << "...\n";
                        socket.send_message(client_sockfds[j], (void *) done, 4);
                    }
                }
            }
            #ifdef SERVER_VERBOSE
            else {
                printf("[INFO] Client #%d finished without cracking the hash.\n", i);
            }
            #endif
        }));
    }
}

void HashCrackServer::wait_for_clients() {
    if (!is_started) {
        throw std::runtime_error("wait_for_clients() requires start()");
    }
    assert(client_connections.size() == (unsigned int) total_clients);
    for (std::thread &t : client_connections) {
        t.join();
    }
    client_connections.clear();
    for (int i = 0; i < total_clients; i++) {
        close(client_sockfds[i]);
    }
    client_sockfds.clear();
}

/* Setters and getters */
void HashCrackServer::set_hash_to_crack(std::string s) {
    hash_to_crack = s;
}
void HashCrackServer::set_search_space(std::string s) {
    search_space = s;
}
void HashCrackServer::set_hash_algo(std::string s) {
    hash_algo = s;
}
void HashCrackServer::set_total_clients(int n) {
    total_clients = n;
    client_sockfds.clear();
    client_sockfds.resize(total_clients, -1);
}
void HashCrackServer::set_max_string_length(int n) {
    max_string_length = n;
}
void HashCrackServer::set_use_fixed_string_length(bool b) {
    use_fixed_string_length = b;
}
void HashCrackServer::set_is_started(bool b)  {
    is_started = b;
}
