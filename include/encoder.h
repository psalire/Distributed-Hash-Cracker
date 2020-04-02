
#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <cryptopp/hex.h>
#include <stdint.h>

class Encoder {
    private:
        /* Expect T to be HexEncoder or HexDecoder, output results to string */
        template <typename T> std::string output_to_string(std::string);
    public:
        std::string encode_bytes_to_hex(std::string);
        std::string decode_hex_to_bytes(std::string);
};

#endif