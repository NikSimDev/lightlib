/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of lightlib.
 *
 * lightlib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * lightlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lightlib; if not, see <https://www.gnu.org/licenses/>.
 */


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