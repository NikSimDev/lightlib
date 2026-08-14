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

#include <gtest/gtest.h>
#include <string>
#include "../../../include/lightlib/Core"

TEST(ConfigTest, LoadConfig) {
	lightlib::ConfigManager::initGlobal();
	lightlib::global_config->setAutoSave(false);
	bool is_nullptr = lightlib::global_config == nullptr;

	EXPECT_FALSE(is_nullptr);
}

TEST(ConfigTest, GetConfigValue) {
	lightlib::ConfigManager::initGlobal("config_test.json");
	lightlib::global_config->setAutoSave(false);
	std::string value = lightlib::global_config->get("app_name", "default_app");
	EXPECT_EQ(value, "LightlibApp");
}

TEST(ConfigTest, SetConfigValue) {
	lightlib::ConfigManager::initGlobal("config_test.json");
	lightlib::global_config->setAutoSave(false);
	lightlib::global_config->set<std::string>("app_name", "NewLightApp");

	std::string value = lightlib::global_config->get("app_name", "default_app");

	EXPECT_EQ(value, "NewLightApp");
}

TEST(ConfigTest, RemoveConfigValue) {
	lightlib::ConfigManager::initGlobal("config_test.json");
	lightlib::global_config->setAutoSave(false);
	lightlib::global_config->remove("app_name");
	std::string value = lightlib::global_config->get("app_name", "default_app");

	EXPECT_EQ(value, "default_app");
}

TEST(ConfigTest, HasConfigValue) {
	lightlib::ConfigManager::initGlobal("config_test.json");
	lightlib::global_config->setAutoSave(false);
	lightlib::global_config->remove("app_name");
	bool has_value = lightlib::global_config->has("app_name");

	EXPECT_FALSE(has_value);
}

TEST(ConfigTest, GetKeysWithPrefix) {
	lightlib::ConfigManager::initGlobal("config_test.json");
	lightlib::global_config->setAutoSave(false);
	lightlib::global_config->set("server.host", "localhost");
	lightlib::global_config->set<int>("server.port", 3502);
	nlohmann::json keys = lightlib::global_config->getNestedJson("server");

	EXPECT_EQ(keys["host"], "localhost");
	EXPECT_EQ(keys["port"], 3502);
}

TEST(ConfigTest, ClearConfig) {
	lightlib::ConfigManager::initGlobal("config_test.json");
	lightlib::global_config->setAutoSave(false);
	lightlib::global_config->clear();
	std::string value = lightlib::global_config->get("app_name", "default_app");

	EXPECT_EQ(value, "default_app");
}

TEST(ConfigTest, ReloadConfig) {
	lightlib::ConfigManager::initGlobal("config_test.json");
	lightlib::global_config->setAutoSave(false);
	lightlib::global_config->set<std::string>("app_name", "TempApp");
	lightlib::global_config->reload();
	std::string value = lightlib::global_config->get("app_name", "default_app");

	EXPECT_EQ(value, "LightlibApp");
}