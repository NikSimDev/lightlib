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

#include "../include/lightlib/vendor/Storage/Storage.hpp"

namespace lightlib {

    Storage& Storage::getInstance() {
        static Storage instance;
        return instance;
    }

    void Storage::setRootPath(const std::string& path) {
        rootPath_ = path;
        if (!fs::exists(rootPath_)) {
            fs::create_directories(rootPath_);
        }
    }

    std::string Storage::getRootPath() const {
        return rootPath_;
    }

    void Storage::put(const std::string& path, const std::string& content) {
        std::string fullPath = rootPath_ + "/" + path;
        std::ofstream file(fullPath, std::ios::out);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + path);
        }
        file << content;
    }

    std::string Storage::get(const std::string& path) {
        std::string fullPath = rootPath_ + "/" + path;
        std::ifstream file(fullPath, std::ios::in);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for reading: " + path);
        }
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    bool Storage::exists(const std::string& path) const {
        return fs::exists(rootPath_ + "/" + path);
    }

    void Storage::deleteFile(const std::string& path) {
        if (!exists(path)) {
            throw std::runtime_error("File does not exist: " + path);
        }
        fs::remove(rootPath_ + "/" + path);
    }

    void Storage::copy(const std::string& source, const std::string& destination) {
        if (!exists(source)) {
            throw std::runtime_error("Source file does not exist: " + source);
        }
        fs::copy(rootPath_ + "/" + source, rootPath_ + "/" + destination);
    }

    void Storage::move(const std::string& source, const std::string& destination) {
        if (!exists(source)) {
            throw std::runtime_error("Source file does not exist: " + source);
        }
        fs::rename(rootPath_ + "/" + source, rootPath_ + "/" + destination);
    }

}