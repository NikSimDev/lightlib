#include "../../include/lightlib/vendor/Cryptography/Dh.hpp"
using namespace lightLib::crypto;

DiffieHellmann::DiffieHellmann(crypt_int prime, crypt_int generator, crypt_int private_key)
    : prime(prime), generator(generator), private_key(private_key), shared_secret(0)
{
    if (!isPrime(prime)) {
        throw std::runtime_error("DH key exchange exception: Non-prime number.");
    }
    if (generator >= prime || generator < 2) {
        throw std::runtime_error("DH key exchange exception: Invalid generator. Must be between 2 and prime-1.");
    }

    public_key = powm(generator, private_key, prime);
}

crypt_int DiffieHellmann::getPublicKey() const {
    return public_key;
}

crypt_int DiffieHellmann::getSharedSecret() const {
    return shared_secret;
}

void DiffieHellmann::evaluateSharedSecret(crypt_int partner_public_key) {
    shared_secret = powm(partner_public_key, private_key, prime);
}

crypt_int DiffieHellmann::xorEncrypt(crypt_int data, crypt_int key) const {
    return data ^ key;
}

crypt_str DiffieHellmann::encrypt(const std::string& plaintext) {
    if (shared_secret == 0) {
        throw std::runtime_error("DH key exchange exception: No shared secret.");
    }

    crypt_str result;

    for (char plainchar : plaintext) {
        result.push_back(encryptChar(plainchar));
    }
    return result;
}

std::string DiffieHellmann::decrypt(const crypt_str& ciphertext) {
    if (shared_secret == 0) {
        throw std::runtime_error("DH key exchange exception: No shared secret.");
    }
    std::string result;
    for (size_t i = 0; i < ciphertext.size(); i++) {
        result[i] = decryptChar(ciphertext[i]);
    }

    return result;
}

crypt_int DiffieHellmann::encryptChar(const char& plainchar) const {
    return xorEncrypt(static_cast<crypt_int>(plainchar), shared_secret);
}

char DiffieHellmann::decryptChar(const crypt_int& cipherchar) const {
    return static_cast<char>(xorEncrypt(cipherchar, shared_secret));
}



