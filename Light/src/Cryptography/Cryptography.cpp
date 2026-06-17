#include "../../include/lightlib/vendor/Cryptography/Cryptography.hpp"
using namespace lightlib;

bool Cryptography::isPrime(const crypt_int &n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    // primality test (optimised)
    for (crypt_int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

//byte char_to_byte(const unsigned char& ch) {
//    return static_cast<byte>(ch);
//}

//bytevec Cryptography::string_to_bytes(const std::string str) {
//    bytevec bytes;
//    for (unsigned char c : str) {
//        bytes.push_back(static_cast<std::vector<byte>::value_type>(c));
//    }
//    return bytes;
//}
//
//std::string Cryptography::bytes_to_string(const bytevec &bytes) {
//    std::string result;
//    result.reserve(bytes.size());
//
//    for (byte b : bytes) {
//        result.push_back(static_cast<char>(b));
//    }
//    return result;
//}