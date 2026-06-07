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

#include "../include/lightlib/WebSocketServer.hpp"

using namespace lightlib;

WebSocketServer::WebSocketServer(const std::string& host, unsigned short port)
    : acceptor_(io_, tcp::endpoint(net::ip::make_address(host), port))
    , port_(port), host_(host) {
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(io_.get_executor());
}

bool WebSocketServer::initialize() {
    try {
        Logger::log("WebSocket server initialized on " + host_ + ":" + std::to_string(port_), "SUCCESS");
        return true;
    }
    catch (const std::exception& e) {
        Logger::log("WebSocket initialization failed: " + std::string(e.what()), "ERROR");
        return false;
    }
}

void WebSocketServer::run() {
    try {
        net::co_spawn(io_, accept_loop(), net::detached);

        int threads_count = std::thread::hardware_concurrency();
        if (threads_count == 0) threads_count = 1;

        Logger::log("Starting " + std::to_string(threads_count) + " WebSocket worker threads", "INFO");

        for (int i = 0; i < threads_count; ++i) {
            threads_.emplace_back([this, i] {
                io_.run();
                });
        }

        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }
    catch (const std::exception& e) {
        Logger::log("WebSocket server run failed: " + std::string(e.what()), "ERROR");
        throw;
    }
}

void WebSocketServer::stop() {
    WebSocketManager::instance().close_all();
    work_guard_.reset();
    io_.stop();
    Logger::log("WebSocket server stopped", "INFO");
}

WebSocketManager& WebSocketServer::get_websocket_manager() {
    return WebSocketManager::instance();
}

void WebSocketServer::set_websocket_message_handler(WebSocketSession::MessageHandler handler) {
    WebSocketManager::instance().set_global_message_handler(std::move(handler));
}

void WebSocketServer::set_websocket_binary_handler(WebSocketSession::BinaryHandler handler) {
    WebSocketManager::instance().set_global_binary_handler(std::move(handler));
}

void WebSocketServer::set_websocket_close_handler(WebSocketSession::CloseHandler handler) {
    WebSocketManager::instance().set_global_close_handler(std::move(handler));
}

void WebSocketServer::set_websocket_error_handler(WebSocketSession::ErrorHandler handler) {
    WebSocketManager::instance().set_global_error_handler(std::move(handler));
}

net::awaitable<void> WebSocketServer::handle_websocket(tcp::socket socket) {
    auto session = std::make_shared<WebSocketSession>(std::move(socket));

    session->set_close_handler([](std::shared_ptr<WebSocketSession> s) {
        WebSocketManager::instance().remove_session(s);
        });

    WebSocketManager::instance().add_session(session);
    co_await session->run();
    co_return;
}

net::awaitable<void> WebSocketServer::handle_connection(tcp::socket socket) {
    connection_count_++;
    co_await handle_websocket(std::move(socket));
    connection_count_--;
    co_return;
}

net::awaitable<void> WebSocketServer::accept_loop() {
    for (;;) {
        tcp::socket socket = co_await acceptor_.async_accept(net::use_awaitable);
        net::co_spawn(io_, handle_connection(std::move(socket)), net::detached);
    }
}