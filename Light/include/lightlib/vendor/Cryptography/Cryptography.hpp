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

#include <vector>
#include <boost/multiprecision/cpp_int.hpp>

namespace mp = boost::multiprecision;

typedef mp::int256_t crypt_int;
typedef std::vector<crypt_int> crypt_str;
//typedef uint8_t byte;
//typedef std::vector<byte> bytevec;

namespace lightlib {

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
