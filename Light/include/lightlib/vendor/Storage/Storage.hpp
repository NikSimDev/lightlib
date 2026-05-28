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

#include <string>
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace lightlib {

    namespace fs = std::filesystem;

    class Storage {
    private:
        Storage() = default;
        Storage(const Storage&) = delete;
        Storage& operator=(const Storage&) = delete;

        std::string rootPath_;

    public:
        static Storage& getInstance();

        void setRootPath(const std::string& path);
        std::string getRootPath() const;

        void put(const std::string& path, const std::string& content);
        std::string get(const std::string& path);
        bool exists(const std::string& path) const;
        void deleteFile(const std::string& path);
        void copy(const std::string& source, const std::string& destination);
        void move(const std::string& source, const std::string& destination);
    };

}