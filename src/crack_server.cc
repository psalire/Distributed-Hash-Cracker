
#include "crack_server.h"

HashCrackServer::HashCrackServer(std::string checksum_hex_string, std::string ss, std::string a, int t, int p, int m, bool b) {
    if (t <= 0) {
        throw std::runtime_error("invalid client count");
    }
    set_is_started(false);
    set_is_cracked_hash(false);
    set_hash_to_crack(checksum_hex_string);
    set_hash_algo(a);
    set_total_clients(t);
    set_max_string_length(m);
    set_use_fixed_string_length(b);
    set_search_space(ss);
    socket_ = TCPComm();
    init_server(p);
}

void HashCrackServer::init_server(int port) {
    socket_.bind_socket(port);
    socket_.listen_socket(total_clients_);
}

void HashCrackServer::accept_clients() {
    set_cracked_hash("");
    set_is_cracked_hash(false);
    set_is_started(false); // Assume starts a new session
    /* Accept all clients */
    for (int i = 0; i < total_clients_; i++) {
        #ifdef SERVER_VERBOSE
        printf("[INFO] Accepting client #%d...\n", i);
        #endif
        client_sockfds_[i] = socket_.accept_connection();
        #ifdef SERVER_VERBOSE
        printf("[INFO] Accepted client #%d\n", i);
        #endif
    }
}

void HashCrackServer::start() {
    set_is_started(true);
    /* Divide search space to assign prefixes */
    std::vector<std::string> divided_searchspace;
    int div = search_space_.size() / total_clients_;
    if ((unsigned int ) total_clients_ > search_space_.size() || !div) {
        throw std::runtime_error("tot_threads cannot be greater than length of search_space_");
    }
    /* Divide search_space_ into total_clients_ number of subdivided_searchspace */
    for (unsigned int i = 0; i < search_space_.size(); i += div) {
        divided_searchspace.push_back(std::string(search_space_, i, div));
    }
    /* Redistribute remainder */
    while (divided_searchspace.size() > (unsigned int) total_clients_) {
        std::string rem = divided_searchspace.back();
        divided_searchspace.pop_back();
        for (unsigned int i = divided_searchspace.size() - rem.size(), j = 0; i < divided_searchspace.size(); i++, j++) {
            divided_searchspace[i] += rem[j];
        }
    }
    assert(divided_searchspace.size() == (unsigned int) total_clients_);
    
    /* Send to each client their prefixes, hash_to_crack_, and search_space_ */
    Settings settings;
    memset(&settings, 0, sizeof(settings));
    strcpy(settings.search_space, search_space_.c_str());
    strcpy(settings.hash_to_crack, hash_to_crack_.c_str());
    strcpy(settings.hash_algo, hash_algo_.c_str());
    memcpy(&settings.max_string_len, &max_string_length_, sizeof(max_string_length_));
    memcpy(&settings.use_fixed_string_len, &use_fixed_string_length_, sizeof(use_fixed_string_length_));
    /* Send settings with designated prefix to each client */
    for (int i = 0; i < total_clients_; i++) {
        strcpy(settings.prefixes, divided_searchspace[i].c_str());
        socket_.send_message(client_sockfds_[i], (void *) &settings, sizeof(settings));
        /* Spawn thread to wait for reply of results for each client */
        client_connections.push_back(std::thread([&, i]() {
            /* Receive results from client */
            Results results;
            memset(&results, 0, sizeof(results));
            socket_.recv_message(client_sockfds_[i], &results, sizeof(results));
            if (results.success) {
                /* Receive cracked hash string */
                char *cracked = new char[results.string_len+1];
                socket_.recv_message(client_sockfds_[i], cracked, results.string_len+1);
                printf("[SERVER] Client #%d cracked the hash.\n", i);
                #ifdef SERVER_VERBOSE
                printf("[INFO] Client #%d CRACKED HASH: %s\n", i, cracked);
                #endif
                
                /* Save cracked hash */
                set_cracked_hash(cracked);
                set_is_cracked_hash(true);
                
                delete[] cracked;
                /* Signal all other clients that done */
                const char *done = "DONE";
                for (int j = 0; j < total_clients_; j++) {
                    if (j != i) {
                        // std::cout << "[INFO] Server send message " << j << "...\n";
                        socket_.send_message(client_sockfds_[j], (void *) done, 4);
                    }
                }
            }
            else {
                printf("[SERVER] Client #%d finished without cracking the hash.\n", i);
            }
        }));
    }
}

void HashCrackServer::wait_for_clients() {
    assert(is_started_);
    assert(client_connections.size() == (unsigned int) total_clients_);
    for (std::thread &t : client_connections) {
        t.join();
    }
    client_connections.clear();
    for (int i = 0; i < total_clients_; i++) {
        close(client_sockfds_[i]);
    }
    client_sockfds_.clear();
}

/* Setters and getters */
void HashCrackServer::set_hash_to_crack(std::string s) {
    hash_to_crack_ = s;
}
void HashCrackServer::set_search_space(std::string s) {
    search_space_ = s;
}
void HashCrackServer::set_hash_algo(std::string s) {
    hash_algo_ = s;
}
void HashCrackServer::set_total_clients(int n) {
    total_clients_ = n;
    client_sockfds_.clear();
    client_sockfds_.resize(total_clients_, -1);
}
void HashCrackServer::set_max_string_length(int n) {
    max_string_length_ = n;
}
void HashCrackServer::set_use_fixed_string_length(bool b) {
    use_fixed_string_length_ = b;
}
void HashCrackServer::set_is_started(bool b)  {
    is_started_ = b;
}
void HashCrackServer::set_is_cracked_hash(bool b) {
    is_cracked_hash_ = b;
}
void HashCrackServer::set_cracked_hash(std::string s) {
    std::lock_guard<std::mutex> lock(lock_cracked_hash_);
    cracked_hash_ = s;
}
bool HashCrackServer::get_is_cracked_hash() {
    return is_cracked_hash_;
}
std::string HashCrackServer::get_cracked_hash() {
    std::lock_guard<std::mutex> lock(lock_cracked_hash_);
    return cracked_hash_;
}
