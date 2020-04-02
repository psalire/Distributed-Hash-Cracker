
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

    HashCrackServer server_crack(
        args.hash_to_crack,
        args.search_space,
        args.hash_algo,
        2, 8080, 0, false
    );

    std::thread thread_accept(&HashCrackServer::accept_clients, std::ref(server_crack));

    HashCrackClient client_comm_1(4);
    HashCrackClient client_comm_2(4);
    std::thread thread_c1(&HashCrackClient::connect_to_server, std::ref(client_comm_1), "127.0.0.1", 8080);
    std::thread thread_c2(&HashCrackClient::connect_to_server, std::ref(client_comm_2), "127.0.0.1", 8080);

    std::cout << "Joining at line " << __LINE__ << "...\n";
    thread_accept.join();
    puts("start()...");
    server_crack.start();
    std::cout << "Joining at line " << __LINE__ << "...\n";
    thread_c1.join();
    std::cout << "Joining at line " << __LINE__ << "...\n";
    thread_c2.join();

    std::vector<Settings> settings;
    settings.push_back(client_comm_1.get_settings());
    settings.push_back(client_comm_2.get_settings());
    int i = 0;
    for (Settings s : settings) {
        std::cout << "Verifiying settings " << i++ << "...\n";
        assert(s.hash_algo == args.hash_algo);
        assert(s.search_space == args.search_space);
        assert(s.hash_to_crack == args.hash_to_crack);
        assert(s.use_fixed_string_len == false);
        assert(s.max_string_len == 0);
    }
    std::cout << "settings[0].prefixes = \"" << settings[0].prefixes << "\"\n";
    assert((strcmp(settings[0].prefixes, "abcdefghijklmnopqrstuvwxyzABCDEF") == 0) ||
            (strcmp(settings[1].prefixes, "GHIJKLMNOPQRSTUVWXYZ0123456789!@") == 0));
    std::cout << "settings[1].prefixes = \"" << settings[1].prefixes << "\"\n";
    if (strcmp(settings[0].prefixes, "abcdefghijklmnopqrstuvwxyzABCDEF") == 0) {
        assert(strcmp(settings[1].prefixes, "GHIJKLMNOPQRSTUVWXYZ0123456789!@") == 0);
    }
    else {
        assert(strcmp(settings[1].prefixes, "abcdefghijklmnopqrstuvwxyzABCDEF") == 0);
    }

    HashCrack<CryptoPP::SHA256> client_crack_1(settings[0], 2);
    HashCrack<CryptoPP::SHA256> client_crack_2(settings[1], 2);

    std::thread thread_crack1([&client_crack_1, &client_comm_1]() {
        client_crack_1.multithreaded_crack(client_comm_1);
        client_comm_1.send_results_to_server(client_crack_1.get_is_cracked(), client_crack_1.get_cracked_hash());
    });
    std::thread thread_crack2([&client_crack_2, &client_comm_2]() {
        client_crack_2.multithreaded_crack(client_comm_2);
        client_comm_2.send_results_to_server(client_crack_2.get_is_cracked(), client_crack_2.get_cracked_hash());
    });
    
    std::cout << "Joining at line " << __LINE__ << "...\n";
    thread_crack1.join();
    std::cout << "Joining at line " << __LINE__ << "...\n";
    thread_crack2.join();
    
    puts("wait_for_clients()...");
    server_crack.wait_for_clients();
    /* Create HashCrack w/ 4 threads */
    /* HashCrack<CryptoPP::SHA256> client_crack(
        args.hash_to_crack,
        // "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`!@#$%^&*()-=~_+[]\\{}|;':\",./<>?",
        args.search_space,
        4, 0, false
    ); */


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
