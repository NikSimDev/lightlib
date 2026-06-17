#pragma once

#include "Cryptography.hpp"

namespace lightlib::crypto {
    class DiffieHellmann : public Cryptography {
        crypt_int prime;
        crypt_int generator;
        crypt_int private_key;
        crypt_int public_key;
        crypt_int shared_secret;

        crypt_int xorEncrypt(crypt_int data, crypt_int key) const;
        crypt_int encryptChar(const char& plainchar) const;
        char decryptChar(const crypt_int& cipherchar) const;

    public:
        DiffieHellmann(crypt_int prime, crypt_int generator, crypt_int private_key);

        crypt_int getPublicKey() const;
        crypt_int getSharedSecret() const;

        void evaluateSharedSecret(crypt_int partner_public_key);

        crypt_str encrypt(const std::string& plaintext) override;
        std::string decrypt(const crypt_str& ciphertext) override;
    };
}