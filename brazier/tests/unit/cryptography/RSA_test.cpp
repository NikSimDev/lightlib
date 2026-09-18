/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of brazier.
 *
 * brazier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * brazier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with brazier; if not, see <https://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <string>
#include <stdexcept>
#include "../../../include/brazier/vendor/Cryptography/RSA.hpp"

class RSATestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        auto pair = brazier::crypto::RSA::generateKeyPair(brazier::crypto::RSA::KEY_SIZE_2048);
        publicKey_ = pair.first;
        privateKey_ = pair.second;

        auto pair2 = brazier::crypto::RSA::generateKeyPair(brazier::crypto::RSA::KEY_SIZE_2048);
        otherPublicKey_ = pair2.first;
        otherPrivateKey_ = pair2.second;
    }

    void TearDown() override {}

    std::string publicKey_;
    std::string privateKey_;
    std::string otherPublicKey_;
    std::string otherPrivateKey_;
};

TEST_F(RSATestFixture, GenerateKeyPair) {
    auto pair = brazier::crypto::RSA::generateKeyPair(brazier::crypto::RSA::KEY_SIZE_2048);
    EXPECT_FALSE(pair.first.empty());
    EXPECT_FALSE(pair.second.empty());
    EXPECT_TRUE(brazier::crypto::RSA::validatePublicKey(pair.first));
    EXPECT_TRUE(brazier::crypto::RSA::validatePrivateKey(pair.second));

    auto pair4096 = brazier::crypto::RSA::generateKeyPair(brazier::crypto::RSA::KEY_SIZE_4096);
    EXPECT_FALSE(pair4096.first.empty());
    EXPECT_FALSE(pair4096.second.empty());

    EXPECT_NO_THROW(brazier::crypto::RSA::generateKeyPair(1024));
}

TEST_F(RSATestFixture, ValidateKeys) {
    EXPECT_TRUE(brazier::crypto::RSA::validatePublicKey(publicKey_));
    EXPECT_FALSE(brazier::crypto::RSA::validatePublicKey("invalid"));
    EXPECT_FALSE(brazier::crypto::RSA::validatePublicKey(privateKey_));

    EXPECT_TRUE(brazier::crypto::RSA::validatePrivateKey(privateKey_));
    EXPECT_FALSE(brazier::crypto::RSA::validatePrivateKey("invalid"));
    EXPECT_FALSE(brazier::crypto::RSA::validatePrivateKey(publicKey_));
}

TEST_F(RSATestFixture, ExtractPublicKey) {
    std::string extracted = brazier::crypto::RSA::extractPublicKey(privateKey_);
    EXPECT_EQ(extracted, publicKey_);
    EXPECT_TRUE(brazier::crypto::RSA::validatePublicKey(extracted));
    EXPECT_THROW(brazier::crypto::RSA::extractPublicKey(publicKey_), std::runtime_error);
}

TEST_F(RSATestFixture, FormatKeys) {
    std::string formattedPub = brazier::crypto::RSA::formatPublicKey(publicKey_);
    EXPECT_FALSE(formattedPub.empty());
    EXPECT_TRUE(brazier::crypto::RSA::validatePublicKey(formattedPub));

    std::string formattedPriv = brazier::crypto::RSA::formatPrivateKey(privateKey_);
    EXPECT_FALSE(formattedPriv.empty());
    EXPECT_TRUE(brazier::crypto::RSA::validatePrivateKey(formattedPriv));
}

TEST_F(RSATestFixture, EncryptDecrypt) {
    const std::string plaintext = "Hello RSA!";
    std::string ciphertext = brazier::crypto::RSA::encryptWithPublic(plaintext, publicKey_);
    EXPECT_NE(ciphertext, plaintext);
    std::string decrypted = brazier::crypto::RSA::decryptWithPrivate(ciphertext, privateKey_);
    EXPECT_EQ(decrypted, plaintext);

    const std::string empty;
    ciphertext = brazier::crypto::RSA::encryptWithPublic(empty, publicKey_);
    decrypted = brazier::crypto::RSA::decryptWithPrivate(ciphertext, privateKey_);
    EXPECT_EQ(decrypted, empty);

    EXPECT_THROW(brazier::crypto::RSA::encryptWithPublic("test", "invalid"), std::runtime_error);
    std::string cipher = brazier::crypto::RSA::encryptWithPublic("test", publicKey_);
    EXPECT_THROW(brazier::crypto::RSA::decryptWithPrivate(cipher, "invalid"), std::runtime_error);
    EXPECT_THROW(brazier::crypto::RSA::decryptWithPrivate(cipher, otherPrivateKey_), std::runtime_error);
}

TEST_F(RSATestFixture, SignVerify) {
    const std::string data = "Important document";
    std::string signature = brazier::crypto::RSA::sign(data, privateKey_);
    EXPECT_FALSE(signature.empty());
    EXPECT_TRUE(brazier::crypto::RSA::verify(data, signature, publicKey_));
    EXPECT_FALSE(brazier::crypto::RSA::verify(data + "x", signature, publicKey_));

    std::string badSig = signature;
    if (!badSig.empty()) badSig[0] ^= 0x01;
    EXPECT_FALSE(brazier::crypto::RSA::verify(data, badSig, publicKey_));
    EXPECT_FALSE(brazier::crypto::RSA::verify(data, signature, otherPublicKey_));

    const std::string empty;
    signature = brazier::crypto::RSA::sign(empty, privateKey_);
    EXPECT_TRUE(brazier::crypto::RSA::verify(empty, signature, publicKey_));

    EXPECT_THROW(brazier::crypto::RSA::sign("data", "invalid"), std::runtime_error);
    EXPECT_THROW(brazier::crypto::RSA::verify("data", "sig", "invalid"), std::runtime_error);
}

TEST_F(RSATestFixture, EncryptWithPrivateIsSign) {
    const std::string data = "test";
    std::string signature = brazier::crypto::RSA::encryptWithPrivate(data, privateKey_);
    EXPECT_TRUE(brazier::crypto::RSA::verify(data, signature, publicKey_));
}

TEST_F(RSATestFixture, Consistency) {
    std::string extracted = brazier::crypto::RSA::extractPublicKey(privateKey_);
    EXPECT_EQ(extracted, publicKey_);
    std::string plaintext = "Consistency test";
    std::string cipher = brazier::crypto::RSA::encryptWithPublic(plaintext, extracted);
    std::string decrypted = brazier::crypto::RSA::decryptWithPrivate(cipher, privateKey_);
    EXPECT_EQ(decrypted, plaintext);

    std::string formatted = brazier::crypto::RSA::formatPublicKey(publicKey_);
    cipher = brazier::crypto::RSA::encryptWithPublic(plaintext, formatted);
    decrypted = brazier::crypto::RSA::decryptWithPrivate(cipher, privateKey_);
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(RSATestFixture, DifferentKeysDifferentCiphertext) {
    const std::string plaintext = "Same message";
    std::string c1 = brazier::crypto::RSA::encryptWithPublic(plaintext, publicKey_);
    std::string c2 = brazier::crypto::RSA::encryptWithPublic(plaintext, otherPublicKey_);
    EXPECT_NE(c1, c2);
}