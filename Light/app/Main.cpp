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

#include "../include/lightlib/Core"
#include "../include/lightlib/DB"
#include "../include/lightlib/Http"
#include "../include/lightlib/Engine.hpp"

int main() {
    try {
        lightlib::ConfigManager::initGlobal("config_test.json");
		lightlib::global_config->setAutoSave(false);

        lightlib::Logger::log("Server initialized.", "INFO");
        std::cout << lightlib::global_config << std::endl;

        std::string server_host = lightlib::global_config->get("server.host", "0.0.0.1");
		int server_port = lightlib::global_config->get("server.port", 8080);

        lightlib::Logger::log("server host: " + server_host, "INFO");
        lightlib::Logger::log("server port: " + std::to_string(server_port), "INFO");

        lightlib::global_config->set("server.host", server_host);
        lightlib::Logger::log("UPDATE VALUE: " + server_host, "INFO");
        lightlib::global_config->set("server.host_new", "0.0.0.11");

        server_host = lightlib::global_config->get("server.host", "0.0.0.1");
        std::string host_new = lightlib::global_config->get("server.host_new", "0.0.0.2");

        lightlib::Logger::log("server updated host: " + server_host, "INFO");
        lightlib::Logger::log("server new host: " + host_new, "INFO");

		lightlib::Server server(server_host, server_port);
		server.run();
		return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        lightlib::Logger::log(std::string(e.what()), "ERROR");
        return 1;
    }
}