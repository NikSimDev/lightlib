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

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/config.hpp>
#include "vendor/Handlers/ENV.hpp"
#include "Database/Queue.hpp"
#include "Database/Cache.hpp"
#include "Database/Migrations/MigrationManager.hpp"
#include "App/Http/Services/AuthService.hpp"
#include "Router/RouterRegisterer.hpp"
#include "Router/Router.hpp"
#include "Engine.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::chrono_literals;

namespace lightlib {

    class Server {
    private:
        net::io_context io_;
        tcp::acceptor acceptor_;
        unsigned short port_;
        std::string host_;

    public:
        Server(const std::string& host, unsigned short port)
            : acceptor_(io_, tcp::endpoint(net::ip::make_address(host), port))
            , port_(port), host_(host) {
        }

        inline bool initialize() {
            try {
                ENV::initialize();
                Logger::init("debug.log");
                Logger::registerSignalHandlers();
                AuthService::secret = ENV::env_variables["AUTH_SECRET"];

                initializeConnections();
                RouterRegisterer::init(io_);
                Engine::init(io_);

                Logger::log("Server initialized on " + host_ + ":" + std::to_string(port_), "SUCCESS");
                return true;
            }
            catch (const std::exception& e) {
                Logger::log("Initialization failed: " + std::string(e.what()), "ERROR");
                return false;
            }
        }

        inline void run() {
            try {
                net::co_spawn(io_, accept_loop(acceptor_), net::detached);
                io_.run();
            }
            catch (const std::exception& e) {
                Logger::log("Server run failed: " + std::string(e.what()), "ERROR");
                throw;
            }
        }

        inline void stop() {
            io_.stop();
            Logger::log("Server stopped", "INFO");
        }

        inline unsigned short getPort() const { return port_; }
        inline const std::string& getHost() const { return host_; }

    private:
        inline void initializeConnections() {
            try {
                Queue::connect(ENV::env_variables["REDIS_HOST"], std::stoi(ENV::env_variables["REDIS_PORT"]));
            }
            catch (const std::exception& e) {
                Logger::log("Connection to queue failed: " + std::string(e.what()), "ERROR");
            }

            try {
                Cache::connect(ENV::env_variables["REDIS_HOST"], std::stoi(ENV::env_variables["REDIS_PORT"]));
            }
            catch (const std::exception& e) {
                Logger::log("Connection to NOSQL database failed: " + std::string(e.what()), "ERROR");
            }

            try {
                Database db;
                (new MigrationManager(db))->Initialize();
            }
            catch (const std::exception& e) {
                Logger::log("Database migration failed: " + std::string(e.what()), "ERROR");
            }
        }

        static inline net::awaitable<void> handle_connection(tcp::socket socket) {
            try {
                beast::flat_buffer buffer;
                bool keep_alive = true;

                while (keep_alive) {
                    http::request<http::string_body> req;
                    http::response<http::string_body> res;

                    beast::error_code ec;
                    co_await http::async_read(socket, buffer, req, net::redirect_error(net::use_awaitable, ec));

                    if (ec == http::error::end_of_stream) {
                        break;
                    }
                    if (ec) {
                        throw boost::system::system_error(ec);
                    }

                    keep_alive = req.keep_alive();
                    res.version(req.version());
                    res.keep_alive(keep_alive);
                    res.set(http::field::connection, keep_alive ? "keep-alive" : "close");

                    co_await Router::handle_request(req, res);

                    if (res.body().empty() && res.count(http::field::content_length) == 0) {
                        res.content_length(res.body().size());
                    }

                    co_await http::async_write(socket, res, net::use_awaitable);

                    if (!keep_alive) {
                        break;
                    }

                    buffer.consume(buffer.size());
                }

                beast::error_code ec;
                socket.shutdown(tcp::socket::shutdown_send, ec);
            }
            catch (const std::exception& e) {
                Logger::log("Exception in connection: " + std::string(e.what()), "ERROR");
            }
            catch (...) {
                Logger::log("Unknown exception in connection handler", "ERROR");
            }
            co_return;
        }

        static inline net::awaitable<void> accept_loop(tcp::acceptor& acceptor) {
            for (;;) {
                beast::error_code ec;
                tcp::socket socket = co_await acceptor.async_accept(net::use_awaitable);
                net::co_spawn(acceptor.get_executor(), handle_connection(std::move(socket)), net::detached);
            }
        }
    };

    inline void initializeEnvironment() {
        ENV::initialize();
    }

    inline std::string getEnvironmentVariable(const std::string& key) {
        return ENV::env_variables[key];
    }

    inline bool isEnvironmentInitialized() {
        return ENV::initialized;
    }

}