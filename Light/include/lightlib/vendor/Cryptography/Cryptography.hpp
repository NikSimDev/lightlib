#pragma once

#include <vector>
#include <boost/multiprecision/cpp_int.hpp>
using namespace boost::multiprecision;

typedef int256_t crypt_int;
typedef std::vector<crypt_int> crypt_str;
//typedef uint8_t byte;
//typedef std::vector<byte> bytevec;

namespace lightLib {

    class Cryptography {
    public:
        virtual ~Cryptography() = default;

        virtual crypt_str encrypt(const std::string& plaintext) = 0;
        virtual std::string decrypt(const crypt_str& ciphertext) = 0;

        static bool isPrime(const crypt_int &n);

    protected:
        virtual crypt_int encryptChar(char ch) = 0;
        virtual char decryptChar(crypt_int ch) = 0;
    };
}
