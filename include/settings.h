
#ifndef _SETTINGS_H_
#define _SETTINGS_H_

// #define CLIENT_DEBUG
#define CLIENT_VERBOSE
#define SERVER_VERBOSE

typedef struct Settings {
    char search_space[2048],
         prefixes[2048],
         hash_to_crack[512],
         hash_algo[20];
    int max_string_len;
    bool use_fixed_string_len;
} Settings;

typedef struct Results {
    bool success;
    unsigned int string_len;
} Results;

#endif
