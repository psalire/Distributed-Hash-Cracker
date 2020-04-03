
#include <iostream>
#include <thread>
#include <utility>
#include <chrono>
#include <cryptopp/sha.h>
#include <cryptopp/files.h>
#include <assert.h>
#include "crack_server.h"
#include "crack_client.h"
#include "args.h"

/* Copy of hash_all_strings() in crack_client.cc, but changed check_hash_match() with string compare */
static bool hash_all_strings(std::string search_space, int length, std::string prepend, std::string to_find) {
    /* Initialize str */
    std::string str(length, search_space[0]);
    /* Prepend str */
    str.insert(0, prepend);
    /* Adjust start pos and length according to prepend */
    int start = prepend.size();
    length += start;
    /* char_i: contains index in search_space for each char in str */
    if (start < length) {
        std::vector<unsigned int> char_i(length, 0);
        unsigned int search_space_size = search_space.size()-1;
        unsigned int i;
        int pos;
        do {
            pos = start;
            for (i = 0; i <= search_space_size; i++) {
                str[pos] = search_space[i];
                if (str == to_find) {
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
        } while (pos != length);
    }
    else if (start == length) {
        if (str == to_find) {
            return true;
        }
    }
    return false;
}
/* Find strings, with limit string length of search_space */
static bool hash_all_strings_up_to_len(std::string search_space, int length, std::string prepend, std::string to_find) {
    for (int i = 0; i <= length-((int)prepend.size()); i++) {
        if (hash_all_strings(search_space, i, prepend, to_find)) {
            return true;
        }
    }
    return false;
}

int main(void) {
    std::chrono::steady_clock::time_point start, end;
    /**************************** Test argparse ****************************/
    const char *argv[] = {
        "./main_test",
        "-i", "fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603", // Hash of "ab'
        "-a", "SHA256",
        "-s", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789<>",
        "-c", "!@",
        "-x", "<>",
        "-n", "0"
    };
    Args args = get_args(13, (char **) argv);
    assert(args.search_space == "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@");
    assert(args.hash_algo == "SHA256");
    assert(args.hash_to_crack == "fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603");
    
    /**************************** Test string generator function ****************************/
    std::cout << "hash_all_strings_up_to_len():";
    std::cout << "  Search space: \"abcdefg\"\n";
    std::cout << "  Max length: 7\n";
    std::cout << "  Finding valid strings...\n";
    std::string ss = "abcdefg";
    for (std::string s : {"a", "g", "fe", "bbg", "", "ggaf", "ggggggg", "bacedd", ss.c_str()}) {
        std::cout << "  -Finding \"" << s << "\"...\n";
        assert(hash_all_strings_up_to_len(ss, ss.size(), "", s));
    }
    std::cout << "  Finding invalid strings...\n";
    for (std::string s : {"x", "ggz", "aaaaaaaa", "!!!!!"}) {
        std::cout << "  -Finding \"" << s << "\"...\n";
        assert(!hash_all_strings_up_to_len(ss, ss.size(), "", s));
    }
    std::cout << " Prepend val \"g\"\n";
    std::cout << " Finding valid strings...\n";
    for (std::string s : {"g", "ggb", "gaaaaaa", "gbcefg"}) {
        std::cout << "  -Finding \"" << s << "\"...\n";
        assert(hash_all_strings_up_to_len(ss, ss.size(), "g", s));
    }
    std::cout << " Finding invalid strings...\n";
    for (std::string s : {"", "xgb", "a!", "gaaaaa@", "abc", "gago", "gggggggg"}) {
        std::cout << "  -Finding \"" << s << "\"...\n";
        assert(!hash_all_strings_up_to_len(ss, ss.size(), "g", s));
    }
    std::cout << " Prepend val \"ab\"\n";
    std::cout << " Finding valid strings...\n";
    for (std::string s : {"ab", "abc", "abgge", "abbbbbe"}) {
        std::cout << "  -Finding \"" << s << "\"...\n";
        assert(hash_all_strings_up_to_len(ss, ss.size(), "ab", s));
    }
    std::cout << " Finding invalid strings...\n";
    for (std::string s : {"", "abbbbbee", "abbbbe!", "abbx", "x", "ab!", "abababababab"}) {
        std::cout << "  -Finding \"" << s << "\"...\n";
        assert(!hash_all_strings_up_to_len(ss, ss.size(), "ab", s));
    }
    
    /**************************** Test verify hash functions ****************************/
    HashCrack<CryptoPP::SHA256> test_verify_hash(args.hash_to_crack, args.search_space, "", 0, 0, false);
    std::cout << "check_hash_match():\n";
    std::cout << "  -Verify hash against incorrect strings...\n";
    for (std::string s : {"Hello world!", "Goodbye world!", "abc", "aa", "ba", "a", ""}) {
        std::cout << "  -Verifying \"" << s << "\"\n";
        assert(!test_verify_hash.check_hash_match(s));
    }
    std::cout << "  -Verify hash against correct string...\n";
    assert(test_verify_hash.check_hash_match("ab"));

    /**************************** Create server instance ****************************/
    HashCrackServer server_crack(
        args.hash_to_crack,
        args.search_space,
        args.hash_algo,
        2, 8080, 0, false
    );
    
    /**************************** Test server client connection ****************************/
    /* Tuple: (#clients, #threads per client, expected divisions of prefixes, search space,
                hash of string, input string, hash algo) */
    for (auto tuple : std::vector<std::tuple<int, std::vector<int>, std::string, std::vector<std::string>,
                                             std::string, std::string, std::string, int, bool>>{
        std::make_tuple(
            2, std::vector<int>{2,2}, args.search_space,
            std::vector<std::string>{"abcdefghijklmnopqrstuvwxyzABCDEF", "GHIJKLMNOPQRSTUVWXYZ0123456789!@"},
            "fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603", "ab", "SHA256", 0, false
        ),
        std::make_tuple(
            1, std::vector<int>{4}, args.search_space,
            std::vector<std::string>{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@"},
            "016c756f0e615ef70ca05eb499a74d31c2ddec7244760d59d792583c0410b36d", "boop", "SHA256", 0, false
        ),
        std::make_tuple(
            4, std::vector<int>{1,1,1,1}, args.search_space,
            std::vector<std::string>{"abcdefghijklmnop", "qrstuvwxyzABCDEF", "GHIJKLMNOPQRSTUV", "WXYZ0123456789!@"},
            "1afc325a1e5d96c494efd418a727a16a84541c777b190f2d14a935a2524bd4ea", "w0R!", "SHA256", 0, false
        ),
        std::make_tuple(
            2, std::vector<int>{1,3}, args.search_space,
            std::vector<std::string>{"abcdefghijklmnopqrstuvwxyzABCDEF", "GHIJKLMNOPQRSTUVWXYZ0123456789!@"},
            "8f7beab00c9b839a44173edb77bdf6f9062b1e7023b06c22013ae2b983945292", "XaaaB", "SHA256", 0, false
        ),
        std::make_tuple(
            4, std::vector<int>{1,1,1,1}, args.search_space,
            std::vector<std::string>{"abcdefghijklmnop", "qrstuvwxyzABCDEF", "GHIJKLMNOPQRSTUV", "WXYZ0123456789!@"},
            "38448a5ac7723caefeba44b9e96bf856d16e8a8b04a588b32b1e6e608e170e91", "worxX", "SHA256", 0, false
        )
    }) {
        int num_clients = std::get<0>(tuple);
        /* Give meaning to tuple values */
        std::vector<int> client_thread_counts = std::get<1>(tuple);
        assert((unsigned int) num_clients == client_thread_counts.size());
        std::string search_space = std::get<2>(tuple);
        std::vector<std::string> expected_prefixes = std::get<3>(tuple);
        assert((unsigned int) num_clients == expected_prefixes.size());
        std::string hash_to_crack = std::get<4>(tuple);
        std::string correct_crack = std::get<5>(tuple);
        std::string hash_algo = std::get<6>(tuple);
        unsigned int max_string_len = std::get<7>(tuple);
        bool use_fixed_string_len = std::get<8>(tuple);
        
        server_crack.set_total_clients(num_clients);
        server_crack.set_hash_to_crack(hash_to_crack);
        server_crack.set_search_space(search_space);
        server_crack.set_max_string_length(max_string_len);
        server_crack.set_use_fixed_string_length(use_fixed_string_len);
        
        std::cout << "\nTesting:\n";
        std::cout << "-Total Clients: " << num_clients << "\n";
        std::cout << "-Hash to Crack: " << hash_to_crack << "\n";
        std::cout << "-Search Space: " << search_space << "\n";
        std::cout << "-Max String Len: " << max_string_len << "\n";
        std::cout << "-Use Fixed String Len: " << use_fixed_string_len << "\n";
            
        /* Server accept clients */
        std::thread thread_accept = std::thread(&HashCrackServer::accept_clients, std::ref(server_crack));
        
        /* Create clients to connect to server */
        std::vector<HashCrackClient *> client_comms;
        std::vector<std::thread> client_comms_threads;
        for (int i = 0; i < num_clients; i++) {
            client_comms.push_back(new HashCrackClient(client_thread_counts[i]));
            client_comms_threads.push_back(std::thread(&HashCrackClient::connect_to_server, std::ref(*client_comms[i]), "127.0.0.1", 8080));
        }
        
        std::cout << "Joining at line " << __LINE__ << "...\n";
        thread_accept.join();
        puts("start()...");
        server_crack.start();
        for (int i = 0; i < num_clients; i++) {
            std::cout << "Joining at line " << __LINE__ << "...\n";
            client_comms_threads[i].join();
            assert(client_comms[i]->get_is_connected());
        }
        std::cout << "done\n";

        /**************************** Test recvd settings correct ****************************/
        std::vector<Settings> settings;
        for (int i = 0; i < num_clients; i++) {
            settings.push_back(client_comms[i]->get_settings());
        }
        int settings_i = 0;
        for (Settings s : settings) {
            std::cout << "Verifiying settings [" << settings_i++ << "]...\n";
            // std::cout << s.hash_algo << "==" << hash_algo << "\n";
            assert(s.hash_algo == hash_algo);
            // std::cout << s.search_space << "==" << search_space << "\n";
            assert(s.search_space == search_space);
            // std::cout << s.hash_to_crack << "==" << hash_to_crack << "\n";
            assert(s.hash_to_crack == hash_to_crack);
            // std::cout << s.use_fixed_string_len << "==" << use_fixed_string_len << "\n";
            assert(s.use_fixed_string_len == use_fixed_string_len);
            // std::cout << s.max_string_len << "==" << max_string_len << "\n";
            assert(s.max_string_len == max_string_len);
        }
        for (unsigned int i = 0; i < settings.size(); i++) {
            bool prefix_match = false;
            for (unsigned int j = 0; j < expected_prefixes.size(); j++) {
                // std::cout << settings[i].prefixes << "==" << expected_prefixes[j] << "\n";
                if (strcmp(settings[i].prefixes, expected_prefixes[j].c_str()) == 0) {
                    prefix_match = true;
                    expected_prefixes.erase(expected_prefixes.begin()+j);
                    break;
                }
            }
            assert(prefix_match);
        }
        assert(expected_prefixes.size() == 0);

        /**************************** Create HashCracks from settings ****************************/
        std::vector<HashCrack<CryptoPP::SHA256> *> client_cracks;
        for (int i = 0; i < num_clients; i++) {
            client_cracks.push_back(new HashCrack<CryptoPP::SHA256>(settings[i], client_thread_counts[i]));
        }

        /**************************** Test distributed crack w/ 2 clients ****************************/
        printf("Cracking hashed string of \"%s\" with %d clients...\n", correct_crack.c_str(), num_clients);
        std::vector<std::thread> crack_threads;
        start = std::chrono::steady_clock::now();
        for (int i = 0; i < num_clients; i++) {
            crack_threads.push_back(std::thread([&, i]() {
                client_cracks[i]->multithreaded_crack(*client_comms[i]);
                client_comms[i]->send_results_to_server(client_cracks[i]->get_is_cracked(), client_cracks[i]->get_cracked_hash());
            }));
        }
        for (std::thread &t : crack_threads) {
            std::cout << "Joining at line " << __LINE__ << "...\n";
            t.join();
        }

        puts("wait_for_clients()...");
        server_crack.wait_for_clients();
        end = std::chrono::steady_clock::now();
        std::cout << "Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0 << "s\n";

        /**************************** Test results correct ****************************/
        bool is_cracked = false;
        for (int i = 0; i < num_clients; i++) {
            if (client_cracks[i]->get_is_cracked()) {
                is_cracked = true;
                assert(client_cracks[i]->get_cracked_hash() == correct_crack);
                if (++i == num_clients) {
                    break;
                }
                for (; i < num_clients; i++) {
                    assert(client_cracks[i]->get_is_cracked() == false);
                }
            }
        }
        assert(is_cracked == true);
        
        /**************************** Free memory, close sockets ****************************/
        for (int i = 0; i < num_clients; i++) {
            delete client_cracks[i];
            client_comms[i]->close_socket();
            delete client_comms[i];
        }
    }
    
    /**************************** Test distributed crack w/ 1 client ****************************/
    // server_crack.set_total_clients(1);
    // server_crack.set_is_started(false);
    // std::thread thread_accept_2(&HashCrackServer::accept_clients, std::ref(server_crack));

    // HashCrackClient client_comm_3(4);
    // std::thread thread_c3(&HashCrackClient::connect_to_server, std::ref(client_comm_3), "127.0.0.1", 8080);
    
    // std::cout << "Joining at line " << __LINE__ << "...\n";
    // thread_accept_2.join();
    // puts("start()...");
    // server_crack.start();
    // std::cout << "Joining at line " << __LINE__ << "...\n";
    // thread_c3.join();

    // HashCrack<CryptoPP::SHA256> client_crack_3(client_comm_3.get_settings(), 4);
    
    // std::cout << "Cracking hashed string of \"ab\" with 1 client, 4 threads...\n";
    // start = std::chrono::steady_clock::now();
    // std::thread thread_crack3([&client_crack_3, &client_comm_3]() {
        // client_crack_3.multithreaded_crack(client_comm_3);
        // client_comm_3.send_results_to_server(client_crack_3.get_is_cracked(), client_crack_3.get_cracked_hash());
    // });

    // std::cout << "Joining at line " << __LINE__ << "...\n";
    // thread_crack3.join();

    // puts("wait_for_clients()...");
    // server_crack.wait_for_clients();
    // end = std::chrono::steady_clock::now();
    // std::cout << "Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0 << "s\n";

    // /**************************** Test results correct ****************************/
    // assert(client_crack_3.get_is_cracked());
    // assert(client_crack_3.get_cracked_hash() == "ab");
    
    // client_comm_3.close_socket();

    /* Create HashCrack w/ 4 threads */
    /* HashCrack<CryptoPP::SHA256> client_crack(
        args.hash_to_crack,
        // "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`!@#$%^&*()-=~_+[]\\{}|;':\",./<>?",
        args.search_space,
        4, 0, false
    ); */

    std::cout << "\nAll tests passed!\n";
    return 0;

    // std::cout << "check_hash_match():\n";
    // std::cout << "  -Verify hash against incorrect strings...\n";
    // for (std::string s : {"Hello world!", "Goodbye world!", "abc", "aa", "ba", "a", ""}) {
        // std::cout << "  -Verifying \"" << s << "\"\n";
        // assert(!client_crack.check_hash_match(s));
    // }

    // std::cout << "  -Verify hash against correct string...\n";
    // assert(client_crack.check_hash_match("ab"));

    // std::cout << "hash_all_strings():\n";
    // std::string ss = "abcdefg";
    // std::cout << "  Search space: \"abcdefg\"\n";
    // std::cout << "  Max length: 7\n";
    // std::cout << "  Finding valid strings...\n";
    // for (std::string s : {"a", "g", "fe", "bbg", "", "ggaf", "ggggggg", "bacedd", ss.c_str()}) {
        // std::cout << "  -Finding \"" << s << "\"...\n";
        // assert(hash_all_strings_up_to_len(ss, ss.size(), "", s));
    // }
    // std::cout << "  Finding invalid strings...\n";
    // for (std::string s : {"x", "ggz", "aaaaaaaa", "!!!!!"}) {
        // std::cout << "  -Finding \"" << s << "\"...\n";
        // assert(!hash_all_strings_up_to_len(ss, ss.size(), "", s));
    // }
    // std::cout << " Prepend val \"g\"\n";
    // std::cout << " Finding valid strings...\n";
    // for (std::string s : {"g", "ggb", "gaaaaaa", "gbcefg"}) {
        // std::cout << "  -Finding \"" << s << "\"...\n";
        // assert(hash_all_strings_up_to_len(ss, ss.size(), "g", s));
    // }
    // std::cout << " Finding invalid strings...\n";
    // for (std::string s : {"", "xgb", "a!", "gaaaaa@", "abc", "gago", "gggggggg"}) {
        // std::cout << "  -Finding \"" << s << "\"...\n";
        // assert(!hash_all_strings_up_to_len(ss, ss.size(), "g", s));
    // }
    // std::cout << " Prepend val \"ab\"\n";
    // std::cout << " Finding valid strings...\n";
    // for (std::string s : {"ab", "abc", "abgge", "abbbbbe"}) {
        // std::cout << "  -Finding \"" << s << "\"...\n";
        // assert(hash_all_strings_up_to_len(ss, ss.size(), "ab", s));
    // }
    // std::cout << " Finding invalid strings...\n";
    // for (std::string s : {"", "abbbbbee", "abbbbe!", "abbx", "x", "ab!", "abababababab"}) {
        // std::cout << "  -Finding \"" << s << "\"...\n";
        // assert(!hash_all_strings_up_to_len(ss, ss.size(), "ab", s));
    // }

    // std::chrono::steady_clock::time_point start, end;

    // std::cout << "\nCracking hashes using 4 threads...\n";
    // for (auto p : std::vector<std::pair<std::string,std::string>>{
        // /* Below should take only a moment to crack */
        // std::make_pair("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
        // std::make_pair("a", "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb"),
        // std::make_pair("@", "c3641f8544d7c02f3580b07c0f9887f0c6a27ff5ab1d4a3e29caf197cfc299ae"),
        // std::make_pair("gg", "cbd3cfb9b9f51bbbfbf08759e243f5b3519cbf6ecc219ee95fe7c667e32c0a8d"),
        // std::make_pair("zoo", "24fe93c9e17cb80452e5d86f1c186fc2fda2482cd5f81d7d305c2295bdab0aec"),
        // std::make_pair("@@", "3330e5ba53cb09ab97dd287ad8ba30380b88a5aca467b6c231b0a46f261c16e1"),
        // std::make_pair("WHAt", "74467d1529a0dffec762f43ae1f2428261cf69ad2daa021f24726a68de54d73a"),
        // std::make_pair("0014", "07a8e31c03ce18180509eeb3107b8f7788f06e60b69cc91eccf6e1ec87917fc9"),
        // std::make_pair("w0R!", "1afc325a1e5d96c494efd418a727a16a84541c777b190f2d14a935a2524bd4ea"),
        // std::make_pair("aaaaa", "ed968e840d10d2d313a870bc131a4e2c311d7ad09bdf32b3418147221f51a6e2"),
        // std::make_pair("Xaaaa", "72d3c2fa494ded13f3a30351138b38037bbae3fca7d67979c173a3ae7392e9c5"),
        // /* Below takes ~120 seconds to crack on an Intel i5 @ 2.50GHz */
        // std::make_pair("worxX", "38448a5ac7723caefeba44b9e96bf856d16e8a8b04a588b32b1e6e608e170e91")
    // }) {
        // printf("Cracking hash for \"%s\"...\n", p.first.c_str());
        // client_crack.set_hash_to_crack(p.second);
        // start = std::chrono::steady_clock::now();
        // client_crack.multithreaded_crack();
        // end = std::chrono::steady_clock::now();
        // std::cout << "Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0 << "s\n";
        // assert(client_crack.get_is_cracked());
        // assert(client_crack.get_cracked_hash() == p.first);
    // }

    // bool caught_ex = false;
    // try {
        // client_crack.set_use_fixed_string_len(true);
    // }
    // catch (const std::exception &e) {
        // caught_ex = std::string(e.what()) == "max_string_len must be > 0 to use fixed string len";
        // assert(caught_ex);
    // }

    // std::cout << "\nCracking hashes using 4 threads and fixed string length of 5...\n";
    // client_crack.set_max_string_len(5);
    // client_crack.set_use_fixed_string_len(true);
    // for (auto p : std::vector<std::pair<std::string,std::string>>{
        // /* Below take ~25 seconds to crack on an Intel i5 @ 2.50GHz */
        // std::make_pair("aaaaa", "ed968e840d10d2d313a870bc131a4e2c311d7ad09bdf32b3418147221f51a6e2"),
        // std::make_pair("Xaaaa", "72d3c2fa494ded13f3a30351138b38037bbae3fca7d67979c173a3ae7392e9c5"),
        // std::make_pair("baaaB", "95b07834fe08fa42afa930cb4490a5096fd20dad0203ded86aa9aa0dc73413af"),
        // std::make_pair("XaaaB", "8f7beab00c9b839a44173edb77bdf6f9062b1e7023b06c22013ae2b983945292"),
        // /* Below take ~100 seconds to crack on an Intel i5 @ 2.50GHz */
        // std::make_pair("13337", "4ef4f04a59429e5de4840ef21bdc5116cf4bf83e3ce58067221bec2dd8e16654"),
        // std::make_pair("worxX", "38448a5ac7723caefeba44b9e96bf856d16e8a8b04a588b32b1e6e608e170e91")
    // }) {
        // printf("Cracking hash for \"%s\"...\n", p.first.c_str());
        // client_crack.set_hash_to_crack(p.second);
        // start = std::chrono::steady_clock::now();
        // client_crack.multithreaded_crack();
        // end = std::chrono::steady_clock::now();
        // std::cout << "Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0 << "s\n";
        // assert(client_crack.get_is_cracked());
        // assert(client_crack.get_cracked_hash() == p.first);
    // }

    // std::cout << "All tests passed!\n";
    // return 0;
}
