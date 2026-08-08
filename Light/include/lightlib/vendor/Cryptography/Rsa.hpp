#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/bio.h>

namespace lightlib::crypto {

    class RSA {
    public:
        static constexpr size_t KEY_SIZE_2048 = 2048;
        static constexpr size_t KEY_SIZE_4096 = 4096;

        static std::pair<std::string, std::string> generateKeyPair(int bits = KEY_SIZE_2048);
        static std::string encryptWithPublic(const std::string& plaintext, const std::string& publicKey);
        static std::string decryptWithPrivate(const std::string& ciphertext, const std::string& privateKey);
        static std::string encryptWithPrivate(const std::string& plaintext, const std::string& privateKey);
        static std::string decryptWithPublic(const std::string& ciphertext, const std::string& publicKey);
        static std::string sign(const std::string& data, const std::string& privateKey);
        static bool verify(const std::string& data, const std::string& signature, const std::string& publicKey);
        static bool validatePublicKey(const std::string& publicKey);
        static bool validatePrivateKey(const std::string& privateKey);
        static std::string extractPublicKey(const std::string& privateKey);
        static std::string formatPublicKey(const std::string& pem);
        static std::string formatPrivateKey(const std::string& pem);

    private:
        static std::string bioToString(BIO* bio);
        static std::string getLastOpenSSLError();
        static EVP_PKEY* loadPublicKey(const std::string& pem);
        static EVP_PKEY* loadPrivateKey(const std::string& pem);
    };
}