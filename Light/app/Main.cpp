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

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/config.hpp>
#include "../include/lightlib/vendor/Handlers/ENV.hpp"
#include "../include/lightlib/Database/Queue.hpp"
#include "../include/lightlib/Database/Cache.hpp"
#include "../include/lightlib/Database/Migrations/MigrationManager.hpp"
#include "../include/lightlib/App/Http/Services/AuthService.hpp"
#include "../include/lightlib/Router/RouterRegisterer.hpp"
#include "../include/lightlib/Router/Router.hpp"
#include "../include/lightlib/server.hpp"

int main() {
    try {
        const unsigned short port = std::stoi(lightlib::getEnvironmentVariable("S_PORT"));
        const std::string host = lightlib::getEnvironmentVariable("S_HOST");

        lightlib::Server server(host, port);

        if (!server.initialize()) {
            std::cerr << "Failed to initialize server" << std::endl;
            return 1;
        }

        std::cout << "Starting server on " << host << ":" << port << std::endl;

        server.run();

        lightlib::Logger::log("Application finished", "INFO");
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        lightlib::Logger::log(std::string(e.what()), "ERROR");
        return 1;
    }
}