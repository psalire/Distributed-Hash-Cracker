
#include "crack_client.h"

/*************************** class HashCrackClient ***************************/

/* Constructor */
HashCrackClient::HashCrackClient(int t) {
    socket_ = TCPComm();
    tot_threads_ = t;
    is_connected = false;
}

/* Methods */
bool HashCrackClient::connect_to_server(const char *ip_addr, int port) {
    try {
        /* Connect to server ip:port */
        socket_.connect_socket(ip_addr, port);
        /* Receive hash_to_crack, search_space_, and prefixes_ */
        #ifdef CLIENT_VERBOSE
        std::cout << "[INFO] Receiving settings from server...\n";
        #endif
        socket_.recv_message((void *) &settings_, sizeof(settings_));
    }
    catch (const std::exception &e) {
        perror(e.what());
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
    socket_.send_message(socket_.get_sockfd(), (void *) &results, sizeof(results));
    if (results.success) {
        std::cout << "[CLIENT] Successfully cracked the hash: " << cracked_hash << "\n";
        #ifdef CLIENT_VERBOSE
        std::cout << "[INFO] Client send results message...\n";
        #endif
        socket_.send_message(socket_.get_sockfd(), (void *) cracked_hash.c_str(), cracked_hash.size()+1);
    }
    else {
        std::cout << "[CLIENT] Finished assignment without cracking the hash.\n";
    }
}
bool HashCrackClient::recv_message(void *buf, int msg_len) {
    return socket_.recv_message(buf, msg_len);
}
Settings HashCrackClient::get_settings() {
    return settings_;
}
bool HashCrackClient::get_is_connected() {
    return is_connected;
}
void HashCrackClient::close_socket() {
    socket_.close_socket();
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
    prefixes_ = settings.prefixes;
    #ifdef CLIENT_DEBUG
    curr_length_ = 0;
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
    prefixes_ = p;
    #ifdef CLIENT_DEBUG
    curr_length_ = 0;
    #endif
}

/* Methods */
/* Verify string input against hash_to_crack */
template <typename HashAlgo> bool HashCrack<HashAlgo>::check_hash_match(std::string input) {
    HashAlgo hash;
    hash.Update((const byte*) input.data(), input.size());
    return hash.Verify(hash_to_crack_byte_arr_);
}

template <typename HashAlgo>
bool HashCrack<HashAlgo>::hash_all_strings(int length, std::string prefix) {
    /* Initialize str */
    std::string str(length, search_space_[0]);
    /* Prepend str */
    str.insert(0, prefix);
    /* Adjust start position and length according to prefix */
    int start = prefix.size();
    length += start;
    if (start < length) {
        /* char_i: contains index in search_space_ for each char in str */
        std::vector<unsigned int> char_i(length, 0);
        unsigned int search_space_size = search_space_.size()-1;
        unsigned int i;
        int pos;
        do {
            pos = start;
            for (i = 0; i <= search_space_size; i++) {
                str[pos] = search_space_[i];
                #ifdef CLIENT_DEBUG
                /* Print all found strings */
                /* {
                    std::lock_guard<std::mutex> lg(lock_stdout_);
                    std::cout << str << std::endl;
                } */
                #endif
                if (check_hash_match(str)) {
                    set_is_cracked(true);
                    set_is_done(true);
                    cracked_hash_ = str;
                    return true;
                }
            }
            for (pos++; pos < length; pos++) {
                if (char_i[pos] != search_space_size) {
                    str[pos] = search_space_[++char_i[pos]];
                    break;
                }
                else {
                    str[pos] = search_space_[(char_i[pos] = 0)];
                }
            }
            if (get_is_done()) {
                return true;
            }
        } while (pos != length);
    }
    else if (start == length) {
        #ifdef CLIENT_DEBUG
        // std::cout << str << "\n";
        #endif
        if (check_hash_match(str)) {
            set_is_cracked(true);
            set_is_done(true);
            cracked_hash_ = str;
            return true;
        }
    }
    return false;
}

template <typename HashAlgo> bool HashCrack<HashAlgo>::crack(std::string prefix, int max_str_len) {
    if (!search_space_.size()) {
        throw std::runtime_error("search_space_ must not be empty");
    }
    if (get_is_done()) {
        return true;
    }

    if (hash_all_strings(max_str_len-prefix.size(), prefix)) {
        return true;
    }
    return false;
}

template <typename HashAlgo> void HashCrack<HashAlgo>::multithreaded_crack(HashCrackClient &client_comm) {
    assert(tot_threads_ > 0);
    set_is_cracked(false);
    set_is_done(false);
    /* Try empty string */
    if (check_hash_match("")) {
        set_is_cracked(true);
        cracked_hash_ = "";
        delete[] hash_to_crack_byte_arr_;
        hash_to_crack_byte_arr_ = NULL;
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
                #if defined CLIENT_VERBOSE || defined CLIENT_DEBUG
                std::lock_guard<std::mutex> lg(lock_stdout_);
                #endif
                std::cout << "[CLIENT] Recvd unexpected msg: " << done_msg << "\n";
            }
        }
    });
    thread_signal.detach();

    /* Multithread hash cracking by dividing search space between starting chars */
    int div = prefixes_.size() / tot_threads_;
    if ((unsigned int ) tot_threads_ > prefixes_.size() || !div) {
        throw std::runtime_error("tot_threads_ cannot be greater than length of prefixes_");
    }
    /* Divide prefixes_ into tot_threads_ number of subdivided_searchspace */
    std::vector<std::string> divided_searchspace;
    for (unsigned int i = 0; i < prefixes_.size(); i += div) {
        divided_searchspace.push_back(std::string(prefixes_, i, div));
    }
    /* Redistribute remainder */
    if (divided_searchspace.size() > (unsigned int) tot_threads_) {
        std::string rem = divided_searchspace.back();
        divided_searchspace.pop_back();
        for (unsigned int i = divided_searchspace.size() - rem.size(), j = 0; i < divided_searchspace.size(); i++, j++) {
            divided_searchspace[i] += rem[j];
        }
    }
    /* Use subdivided_searchspace as prefixes_ for crack() */
    std::vector<std::thread> threads;
    #ifdef CLIENT_DEBUG
    /* Print divided search space */
    /* for (std::string s : divided_searchspace) {
        std::cout << s << std::endl;
    } */
    #endif
    #ifdef CLIENT_VERBOSE
    curr_length_ = 0;
    #endif
    #ifdef CLIENT_DEBUG
    int thread_num = 0;
    #endif
    /* If use_fixed_str_len_ is false */
    if (!use_fixed_str_len_) {
        auto f_check_str_len = max_string_len_ ? [](int l, int s) {
            return l <= s;
        } : []([[maybe_unused]] int l, [[maybe_unused]] int s) {
            return true;
        };
        for (std::string s : divided_searchspace) {
            threads.push_back(std::thread([&](std::string prepend_set, auto check_str_len
            #ifdef CLIENT_DEBUG
            , int t_num
            #endif
            ) {
                /* Try string lengths until max_string_len_
                   if max_string_len_==0, is an infinite loop , see check_str_len
                */
                for (int l = 1; check_str_len(l, max_string_len_); l++) {
                    /* Verbose or debug logging */
                    #if defined CLIENT_DEBUG || defined CLIENT_VERBOSE
                    {
                        std::lock_guard<std::mutex> lg(lock_stdout_);
                        #ifdef CLIENT_VERBOSE
                        if (l > curr_length_) {
                            curr_length_ = l;
                            std::cout << "[INFO] Cracking lengths: " << curr_length_ << "\n";
                        }
                        #endif
                        #ifdef CLIENT_DEBUG
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
            #ifdef CLIENT_DEBUG
            , thread_num++
            #endif
            ));
        }
    }
    /* Else use_fixed_str_len_ is true */
    else {
        for (std::string s : divided_searchspace) {
            threads.push_back(std::thread([&](std::string prepend_set
                #ifdef CLIENT_DEBUG
                , int t_num
                #endif
                ) {
                    /* Verbose or debug logging */
                    #if defined CLIENT_DEBUG || defined CLIENT_VERBOSE
                    {
                        std::lock_guard<std::mutex> lg(lock_stdout_);
                        #ifdef CLIENT_VERBOSE
                        std::cout << "[INFO] Cracking lengths: " << max_string_len_ << "\n";
                        #endif
                        #ifdef CLIENT_DEBUG
                        printf("[THREAD_%d] Cracking lengths: %d\n", t_num, max_string_len_);
                        #endif
                    }
                    #endif
                    /* Use each char in divided search space as prefix */
                    for (unsigned int i = 0; i < prepend_set.size(); i++) {
                        if (crack(prepend_set.substr(i, 1), max_string_len_)) {
                            return; /* End once hash cracked */
                        }
                    }
            }, s
            #ifdef CLIENT_DEBUG
            , thread_num++
            #endif
            ));
        }
    }
    for (std::thread &t : threads) {
        t.join();
    }
    delete[] hash_to_crack_byte_arr_;
    hash_to_crack_byte_arr_ = NULL;
}

/* Setters and getters */
template <typename HashAlgo> void HashCrack<HashAlgo>::set_hash_to_crack(std::string checksum_hex_string) {
    /* Save checksum hex string as byte array */
    std::string hash_to_crack = decode_hex_to_bytes(checksum_hex_string);
    hash_to_crack_byte_arr_ = new byte[hash_to_crack.size()];
    memcpy(hash_to_crack_byte_arr_, hash_to_crack.data(), hash_to_crack.size());
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_search_space(std::string ss) {
    search_space_ = ss;
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_tot_threads(int n) {
    tot_threads_ = n;
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_max_string_len(int n) {
    max_string_len_ = n;
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_use_fixed_string_len(bool b) {
    if (b && !max_string_len_) {
        throw std::runtime_error("max_string_len_ must be > 0 to use fixed string len");
    }
    use_fixed_str_len_ = b;
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_is_cracked(bool b) {
    is_cracked_.store(b, std::memory_order_relaxed);
}
template <typename HashAlgo> void HashCrack<HashAlgo>::set_is_done(bool b) {
    is_done_.store(b, std::memory_order_relaxed);
}
template <typename HashAlgo> std::string HashCrack<HashAlgo>::get_cracked_hash() {
    return cracked_hash_;
}
template <typename HashAlgo> bool HashCrack<HashAlgo>::get_is_cracked() {
    return is_cracked_.load(std::memory_order_relaxed);
}
template <typename HashAlgo> bool HashCrack<HashAlgo>::get_is_done() {
    return is_done_.load(std::memory_order_relaxed);
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
