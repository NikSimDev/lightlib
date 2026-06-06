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
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
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
        std::vector<std::thread> threads_;
        std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;

        std::atomic<int> connection_count_{ 0 };
        std::atomic<int> total_requests_{ 0 };

    public:
        Server(const std::string& host, unsigned short port)
            : acceptor_(io_, tcp::endpoint(net::ip::make_address(host), port))
            , port_(port), host_(host) {
            acceptor_.set_option(tcp::acceptor::reuse_address(true));
            work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(io_.get_executor());
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
                net::co_spawn(io_, accept_loop(), net::detached);

                int threads_count = std::thread::hardware_concurrency();
                if (threads_count == 0) threads_count = 1;

                Logger::log("Starting " + std::to_string(threads_count) + " worker threads", "INFO");

                for (int i = 0; i < threads_count; ++i) {
                    threads_.emplace_back([this, i] {
                        Logger::log("Worker thread " + std::to_string(i) + " started on core", "DEBUG");
                        io_.run();
                        Logger::log("Worker thread " + std::to_string(i) + " stopped", "DEBUG");
                        });
                }

                std::thread stats_thread([this] {
                    while (true) {
                        std::this_thread::sleep_for(10s);
                        Logger::log("STATS - Active connections: " + std::to_string(connection_count_.load()) +
                            ", Total requests: " + std::to_string(total_requests_.load()), "INFO");
                    }
                    });
                stats_thread.detach();

                for (auto& t : threads_) {
                    if (t.joinable()) {
                        t.join();
                    }
                }
            }
            catch (const std::exception& e) {
                Logger::log("Server run failed: " + std::string(e.what()), "ERROR");
                throw;
            }
        }

        inline void stop() {
            work_guard_.reset();
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

        net::awaitable<void> handle_connection(tcp::socket socket) {
            connection_count_++;

            try {
                http::request<http::string_body> req;
                http::response<http::string_body> res;
                beast::flat_buffer buffer;
                bool keep_alive = true;

                socket.set_option(tcp::no_delay(true));

                while (keep_alive) {
                    beast::error_code ec;
                    co_await http::async_read(socket, buffer, req, net::redirect_error(net::use_awaitable, ec));

                    if (ec == http::error::end_of_stream) {
                        break;
                    }
                    if (ec) {
                        throw boost::system::system_error(ec);
                    }

                    total_requests_++;
                    keep_alive = req.keep_alive();

                    res = {};
                    res.version(req.version());
                    res.keep_alive(keep_alive);
                    res.set(http::field::connection, keep_alive ? "keep-alive" : "close");
                    res.set(http::field::server, "lightlib");

                    co_await Router::handle_request(req, res);

                    if (res.body().empty() && res.count(http::field::content_length) == 0) {
                        res.content_length(res.body().size());
                    }

                    co_await http::async_write(socket, res, net::use_awaitable);

                    buffer.consume(buffer.size());

                    if (!keep_alive) {
                        break;
                    }
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

            connection_count_--;
            co_return;
        }

        net::awaitable<void> accept_loop() {
            for (;;) {
                tcp::socket socket = co_await acceptor_.async_accept(net::use_awaitable);
                net::co_spawn(io_, handle_connection(std::move(socket)), net::detached);
            }
        }
    };
}