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

        struct privateKey {
            crypt_int s; // exponent
            crypt_int p; // prime
            crypt_int q; // prime
        };

        struct publicKey {
            crypt_int r; // exponent
            crypt_int m; // modulus
        };

        class RSA : public Cryptography {
            // private key members
            crypt_int p;
            crypt_int q;
            crypt_int phi; // ϕ(m)

            // public key members
            crypt_int m;
            crypt_int r;

            privateKey* private_key;
            publicKey* public_key;

            void setParameters(crypt_int p, crypt_int q, crypt_int r);
            void evaluateKeys();
            void evaluatePrivateKey();
            void evaluatePublicKey();

            static crypt_int makePositive(crypt_int n, crypt_int mod);
            static crypt_int evaluatePhi(crypt_int a, crypt_int b);

            crypt_int encryptChar(const char &plainchar) const;
            char decryptChar(const crypt_int& cipherchar) const;

        public:
            RSA() = delete;
            RSA(const crypt_int &p, const crypt_int &q, const crypt_int &r);
            ~RSA() override;

            publicKey* getPublicKey() const;
            privateKey* getPrivateKey() const;

            crypt_str encrypt(const std::string& plaintext) override;
            std::string decrypt(const crypt_str& ciphertext) override;
        };
    }
