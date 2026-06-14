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

#include "../include/lightlib/vendor/ConfigManager.hpp"
#include "../include/lightlib/Engine.hpp"
#include <sstream>

namespace lightlib {

    void ConfigManager::flattenJson(const std::string& prefix, const json& j,
        std::unordered_map<std::string, std::string>& output) {
        if (j.is_object()) {
            for (auto& [key, value] : j.items()) {
                std::string newPrefix = prefix.empty() ? key : prefix + "." + key;
                flattenJson(newPrefix, value, output);
            }
        }
        else if (j.is_array()) {
            output[prefix] = j.dump();
        }
        else if (j.is_string()) {
            output[prefix] = j.get<std::string>();
        }
        else {
            output[prefix] = j.dump();
        }
    }

    json ConfigManager::unflattenMap(const std::unordered_map<std::string, std::string>& map) const {
        json result;

        for (const auto& [key, value] : map) {
            std::vector<std::string> parts;
            std::stringstream ss(key);
            std::string part;
            while (std::getline(ss, part, '.')) {
                parts.push_back(part);
            }

            json* current = &result;
            for (size_t i = 0; i < parts.size() - 1; ++i) {
                if (!current->contains(parts[i])) {
                    (*current)[parts[i]] = json::object();
                }
                current = &(*current)[parts[i]];
            }

            const std::string& lastPart = parts.back();
            try {
                (*current)[lastPart] = json::parse(value);
            }
            catch (...) {
                (*current)[lastPart] = value;
            }
        }

        return result;
    }

    ConfigManager::ConfigManager(const std::string& configFilePath,
        std::shared_ptr<BaseDriver> driver)
        : driver_(driver)
        , configFilePath_(configFilePath) {
        if (driver_) {
            load();
        }
    }

    ConfigManager::ConfigManager(const std::string& configFilePath,
        const std::string& driverName) : configFilePath_(configFilePath) {
        try {
            Filesystem fs = Filesystem::driver(driverName);
            driver_ = fs.getDriver();
        }
        catch (std::exception& e) {
            throw std::runtime_error("Failed to initialize ConfigManager with driver '" + driverName + "': " + e.what());
        }
    }

    void ConfigManager::load() {
        if (!driver_) {
            throw std::runtime_error("Driver is not initialized");
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (!driver_->exists(configFilePath_)) {
            config_.clear();
            return;
        }

        try {
            std::string content = driver_->get(configFilePath_);
            json j = json::parse(content);

            config_.clear();
            flattenJson("", j, config_);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to load config: " + std::string(e.what()));
        }
    }

    void ConfigManager::save() {
        if (!driver_) {
            throw std::runtime_error("Driver is not initialized");
        }

        std::lock_guard<std::mutex> lock(mutex_);

        json j = unflattenMap(config_);
        std::string content = j.dump(4);
        driver_->put(configFilePath_, content);
    }

    bool ConfigManager::has(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.find(key) != config_.end();
    }

    bool ConfigManager::remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t erased = config_.erase(key);
        if (erased > 0 && autoSave_) {
            save();
        }
        return erased > 0;
    }

    std::vector<std::string> ConfigManager::getKeys() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> keys;
        keys.reserve(config_.size());
        for (const auto& [key, _] : config_) {
            keys.push_back(key);
        }
        return keys;
    }

    json ConfigManager::getAllAsJson() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return unflattenMap(config_);
    }

    void ConfigManager::loadFromJson(const json& j) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.clear();
        flattenJson("", j, config_);
        if (autoSave_) {
            save();
        }
    }

    void ConfigManager::clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.clear();
        if (autoSave_) {
            save();
        }
    }

    void ConfigManager::setAutoSave(bool autoSave) {
        std::lock_guard<std::mutex> lock(mutex_);
        autoSave_ = autoSave;
    }

    void ConfigManager::reload() {
        load();
    }

    std::unordered_map<std::string, std::string> ConfigManager::getRawData() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    bool ConfigManager::hasNested(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.find(key) != config_.end();
    }

    std::vector<std::string> ConfigManager::getKeysNested() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> keys;
        keys.reserve(config_.size());
        for (const auto& [key, _] : config_) {
            keys.push_back(key);
        }
        return keys;
    }

    void ConfigManager::loadFromJsonNested(const json& j) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.clear();
        flattenJson("", j, config_);
        if (autoSave_) {
            save();
        }
    }

    json ConfigManager::getNestedJson(const std::string& prefix) const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::unordered_map<std::string, std::string> filtered;
        if (prefix.empty()) {
            filtered = config_;
        }
        else {
            for (const auto& [key, value] : config_) {
                if (key.find(prefix) == 0) {
                    std::string subKey = key.substr(prefix.length() + 1);
                    filtered[subKey] = value;
                }
            }
        }

        return unflattenMap(filtered);
    }

}