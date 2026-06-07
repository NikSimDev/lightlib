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
#include <boost/beast/websocket.hpp>
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
#include "vendor/WebSocket/WebSocketSession.hpp"
#include "vendor/WebSocket/WebSocketManager.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::chrono_literals;

namespace lightlib {

    class WebSocketServer {
    private:
        net::io_context io_;
        tcp::acceptor acceptor_;
        unsigned short port_;
        std::string host_;
        std::vector<std::thread> threads_;
        std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;

        std::atomic<int> connection_count_{ 0 };

    public:
        WebSocketServer(const std::string& host, unsigned short port);

        bool initialize();
        void run();
        void stop();
        WebSocketManager& get_websocket_manager();
        void set_websocket_message_handler(WebSocketSession::MessageHandler handler);
        void set_websocket_binary_handler(WebSocketSession::BinaryHandler handler);
        void set_websocket_close_handler(WebSocketSession::CloseHandler handler);
        void set_websocket_error_handler(WebSocketSession::ErrorHandler handler);

    private:
        net::awaitable<void> handle_websocket(tcp::socket socket);
        net::awaitable<void> handle_connection(tcp::socket socket);
        net::awaitable<void> accept_loop();
    };

}