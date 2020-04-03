
#include "crack_client.h"

/*************************** class HashCrackClient ***************************/

/* Constructor */
HashCrackClient::HashCrackClient(int t) {
    socket = TCPComm();
    tot_threads = t;
    is_connected = false;
}

/* Methods */
bool HashCrackClient::connect_to_server(const char *ip_addr, int port) {
    try {
        /* Connect to server ip:port */
        socket.connect_socket(ip_addr, port);
        /* Receive hash_to_crack, search_space, and prefixes */
        socket.recv_message((void *) &settings, sizeof(settings));
    }
    catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        return false;
    }
    is_connected = true;
    return true;
}
void HashCrackClient::send_results_to_server(bool b, std::string cracked_hash) {
    assert(get_is_connected());
    Results results;
    memset(&results, 0, sizeof(results));
    results.success = b;
    results.string_len = cracked_hash.size();
    socket.send_message(socket.get_sockfd(), (void *) &results, sizeof(results));
    if (results.success) {
        // std::cout << "[INFO] Client send message...\n";
        socket.send_message(socket.get_sockfd(), (void *) cracked_hash.c_str(), cracked_hash.size()+1);
    }
}
bool HashCrackClient::recv_message(void *buf, int msg_len) {
    return socket.recv_message(buf, msg_len);
}
Settings HashCrackClient::get_settings() {
    return settings;
}
bool HashCrackClient::get_is_connected() {
    return is_connected;
}
void HashCrackClient::close_socket() {
    socket.close_socket();
    is_connected = false;
}

/*************************** class HashCrack ***************************/

/* Constructors */
template <typename T> HashCrack<T>::HashCrack(Settings settings, int n) {
    /* Save checksum hex string as byte string */
    set_hash_to_crack(settings.hash_to_crack);
    set_search_space(settings.search_space);
    set_max_string_len(settings.max_string_len);
    set_use_fixed_string_len(settings.use_fixed_string_len);
    set_tot_threads(n);
    set_is_cracked(false);
    set_is_done(false);
    prefixes = settings.prefixes;
    #ifdef DEBUG
    curr_length = 0;
    #endif
}
template <typename T> HashCrack<T>::HashCrack(std::string checksum_hex_string, std::string s, std::string p, int n, int m, bool f) {
    /* Save checksum hex string as byte string */
    set_hash_to_crack(checksum_hex_string);
    set_search_space(s);
    set_max_string_len(m);
    set_use_fixed_string_len(f);
    set_tot_threads(n);
    set_is_cracked(false);
    set_is_done(false);
    prefixes = p;
    #ifdef DEBUG
    curr_length = 0;
    #endif
}

/* Methods */
/* Verify string input against hash_to_crack */
template <typename HashAlgo> bool HashCrack<HashAlgo>::check_hash_match(std::string input) {
    HashAlgo hash;
    hash.Update((const byte*) input.data(), input.size());
    return hash.Verify(hash_to_crack_byte_arr);
}

template <typename HashAlgo>
bool HashCrack<HashAlgo>::hash_all_strings(std::string search_space, int length, std::string prefix) {
    /* Initialize str */
    std::string str(length, search_space[0]);
    /* Prepend str */
    str.insert(0, prefix);
    /* Adjust start position and length according to prefix */
    int start = prefix.size();
    length += start;
    if (start < length) {
        /* char_i: contains index in search_space for each char in str */
        std::vector<unsigned int> char_i(length, 0);
        unsigned int search_space_size = search_space.size()-1;
        unsigned int i;
        int pos;
        do {
            pos = start;
            for (i = 0; i <= search_space_size; i++) {
                str[pos] = search_space[i];
                #ifdef DEBUG
                /* Print all found strings */
                /* {
                    std::lock_guard<std::mutex> lg(lock_stdout);
                    std::cout << str << std::endl;
                } */
                #endif
                if (check_hash_match(str)) {
                    set_is_cracked(true);
                    set_is_done(true);
                    cracked_hash = str;
                    return true;
                }
            }
            for (pos++; pos < length; pos++) {
                if (char_i[pos] != search_space_size) {
                    str[pos] = search_space[++char_i[pos]];
                    break;
                }
                else {
                    str[pos] = search_space[(char_i[pos] = 0)];
                }
            }
            if (get_is_done()) {
                return true;
            }
        } while (pos != length);
    }
    else if (start == length) {
        #ifdef DEBUG
        // std::cout << str << "\n";
        #endif
        if (check_hash_match(str)) {
            set_is_cracked(true);
            set_is_done(true);
            cracked_hash = str;
            return true;
        }
    }
    return false;
}

template <typename HashAlgo> bool HashCrack<HashAlgo>::crack(std::string prefix, int max_str_len) {
    if (!search_space.size()) {
        throw std::runtime_error("search_space must not be empty");
    }
    if (get_is_done()) {
        return true;
    }

    /* Crack with max length */
    // if (max_str_len) {
        if (hash_all_strings(search_space, max_str_len-prefix.size(), prefix)) {
            return true;
        }
    // }
    /* No max length, loop forever until match */
    // else {
        // for (int i = 0;; i++) {
            // if (hash_all_strings(search_space, i, prefix)) {
                // return true;
            // }
        // }
    // }
    return false;
}

template <typename HashAlgo> void HashCrack<HashAlgo>::multithreaded_crack(HashCrackClient &client_comm) {
    assert(tot_threads > 0);
    set_is_cracked(false);
    set_is_done(false);
    /* Try empty string */
    if (check_hash_match("")) {
        set_is_cracked(true);
        cracked_hash = "";
        delete[] hash_to_crack_byte_arr;
        hash_to_crack_byte_arr = NULL;
        return;
    }
    
    /* Spawn thread to recv and set signal if done */
    std::thread thread_signal([&]() {
        char done_msg[4]; // expect to recv "DONE"
        while (1) {
            if (!client_comm.recv_message((void *) done_msg, 4)) {
                return;
            }
            if (strcmp(done_msg, "DONE") == 0) {
                set_is_done(true);
                return;
            }
            else {
                std::cout << "[INFO] Recvd unexpected msg: " << done_msg << "\n";
            }
        }
    });
    thread_signal.detach();

    /* Multithread hash cracking by dividing search space between starting chars */
    int div = prefixes.size() / tot_threads;
    if ((unsigned int ) tot_threads > prefixes.size() || !div) {
        throw std::runtime_error("tot_threads cannot be greater than length of prefixes");
    }
    /* Divide prefixes into tot_threads number of subdivided_searchspace */
    std::vector<std::string> divided_searchspace;
    for (unsigned int i = 0; i < prefixes.size(); i += div) {
        divided_searchspace.push_back(std::string(prefixes, i, div));
    }
    /* Redistribute remainder */
    if (divided_searchspace.size() > (unsigned int) tot_threads) {
        std::string rem = divided_searchspace.back();
        divided_searchspace.pop_back();
        for (unsigned int i = divided_searchspace.size() - rem.size(), j = 0; i < divided_searchspace.size(); i++, j++) {
            divided_searchspace[i] += rem[j];
        }
    }
    /* Use subdivided_searchspace as prefixes for crack() */
    std::vector<std::thread> threads;
    #ifdef DEBUG
    /* Print divided search space */
    /* for (std::string s : divided_searchspace) {
        std::cout << s << std::endl;
    } */
    #endif
    #ifdef VERBOSE
    curr_length = 0;
    #endif
    #ifdef DEBUG
    int thread_num = 0;
    #endif
    /* If use_fixed_str_len is false */
    if (!use_fixed_str_len) {
        auto f_check_str_len = max_string_len ? [](int l, int s) {
            return l <= s;
        } : []([[maybe_unused]] int l, [[maybe_unused]] int s) {
            return true;
        };
        for (std::string s : divided_searchspace) {
            threads.push_back(std::thread([&](std::string prepend_set, auto check_str_len
            #ifdef DEBUG
            , int t_num
            #endif
            ) {
                /* Try string lengths until max_string_len
                   if max_string_len==0, is an infinite loop , see check_str_len
                */
                for (int l = 1; check_str_len(l, max_string_len); l++) {
                    /* Verbose or debug logging */
                    #if defined DEBUG || defined VERBOSE
                    {
                        std::lock_guard<std::mutex> lg(lock_stdout);
                        #ifdef VERBOSE
                        if (l > curr_length) {
                            curr_length = l;
                            std::cout << "[INFO] Cracking lengths: " << curr_length << "\n";
                        }
                        #endif
                        #ifdef DEBUG
                        printf("[THREAD_%d] Cracking lengths: %d\n", t_num, l);
                        #endif
                    }
                    #endif
                    /* Use each char in divided search space as prefix */
                    for (unsigned int i = 0; i < prepend_set.size(); i++) {
                        if (crack(prepend_set.substr(i, 1), l)) {
                            return; /* End once hash cracked */
                        }
                    }
                }
            }, s, f_check_str_len
            #ifdef DEBUG
            , thread_num++
            #endif
            ));
        }
    }
    /* Else use_fixed_str_len is true */
    else {
        for (std::string s : divided_searchspace) {
            threads.push_back(std::thread([&](std::string prepend_set
                #ifdef DEBUG
                , int t_num
                #endif
                ) {
                    /* Verbose or debug logging */
                    #if defined DEBUG || defined VERBOSE
                    {
                        std::lock_guard<std::mutex> lg(lock_stdout);
                        #ifdef VERBOSE
                        std::cout << "[INFO] Cracking lengths: " << max_string_len << "\n";
                        #endif
                        #ifdef DEBUG
                        printf("[THREAD_%d] Cracking lengths: %d\n", t_num, max_string_len);
                        #endif
                    }
                    #endif
                    /* Use each char in divided search space as prefix */
                    for (unsigned int i = 0; i < prepend_set.size(); i++) {
                        if (crack(prepend_set.substr(i, 1), max_string_len)) {
                            return; /* End once hash cracked */
                        }
                    }
            }, s
            #ifdef DEBUG
            , thread_num++
            #endif
            ));
        }
    }
    for (std::thread &t : threads) {
        t.join();
    }
    delete[] hash_to_crack_byte_arr;
    hash_to_crack_byte_arr = NULL;
}

/* Setters and getters */
template <typename HashAlgo> void HashCrack<HashAlgo>::set_hash_to_crack(std::string checksum_hex_string) {
    /* Save checksum hex string as byte array */
    std::string hash_to_crack = decode_hex_to_bytes(checksum_hex_string);
    hash_to_crack_byte_arr = new byte[hash_to_crack.size()];
    memcpy(hash_to_crack_byte_arr, hash_to_crack.data(), hash_to_crack.size());
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_search_space(std::string ss) {
    search_space = ss;
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_tot_threads(int n) {
    tot_threads = n;
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_max_string_len(int n) {
    max_string_len = n;
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_use_fixed_string_len(bool b) {
    if (b && !max_string_len) {
        throw std::runtime_error("max_string_len must be > 0 to use fixed string len");
    }
    use_fixed_str_len = b;
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_is_cracked(bool b) {
    is_cracked.store(b, std::memory_order_relaxed);
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_is_done(bool b) {
    is_done.store(b, std::memory_order_relaxed);
}
template <typename HashAlgo> std::string HashCrack<HashAlgo>::get_cracked_hash() {
    return cracked_hash;
}
template <typename HashAlgo> bool HashCrack<HashAlgo>::get_is_cracked() {
    return is_cracked.load(std::memory_order_relaxed);
}
template <typename HashAlgo> bool HashCrack<HashAlgo>::get_is_done() {
    return is_done.load(std::memory_order_relaxed);
}

/* Declare possible types for template class */
template class HashCrack<CryptoPP::SHA1>;
template class HashCrack<CryptoPP::SHA224>;
template class HashCrack<CryptoPP::SHA256>;
template class HashCrack<CryptoPP::SHA384>;
template class HashCrack<CryptoPP::SHA512>;
template class HashCrack<CryptoPP::SHA3_224>;
template class HashCrack<CryptoPP::SHA3_256>;
template class HashCrack<CryptoPP::SHA3_384>;
template class HashCrack<CryptoPP::SHA3_512>;
