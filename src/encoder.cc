
#include "encoder.h"

/* Expect T to be HexEncoder or HexDecoder, output results to string */
template <typename T> std::string Encoder::output_to_string(std::string input) {
    T coder;
    coder.Put((byte *) input.data(), input.size());
    coder.MessageEnd();
    CryptoPP::word64 size = coder.MaxRetrievable();
    std::string out;
    if (size) {
        out.resize(size);
        coder.Get((byte*) &out[0], out.size());
    }
    return out;
}
std::string Encoder::encode_bytes_to_hex(std::string raw) {
    /* Using filter */
    /* std::string out;
    CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(out));
    CryptoPP::StringSource(raw, true, new CryptoPP::Redirector(encoder)); 
    return out; */

    return output_to_string<CryptoPP::HexEncoder>(raw);
}
std::string Encoder::decode_hex_to_bytes(std::string raw) {
    /* Using filter */
    /* std::string out;
    CryptoPP::HexDecoder decoder(new CryptoPP::StringSink(out));
    CryptoPP::StringSource(raw, true, new CryptoPP::Redirector(decoder));
    return out; */

    return output_to_string<CryptoPP::HexDecoder>(raw);
}
