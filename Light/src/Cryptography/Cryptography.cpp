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