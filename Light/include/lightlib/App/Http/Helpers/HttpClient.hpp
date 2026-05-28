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
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>
#include <regex>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/connect.hpp>
#include <iomanip>
#include <sstream>
#include <cctype>


namespace lightlib {

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    namespace ssl = net::ssl;
    using tcp = net::ip::tcp;
    using Response = http::response<http::string_body>;
    using Request = http::request<http::string_body>;

    class HttpClient {
    public:
        using json = nlohmann::json;

        HttpClient();
        Response get(const std::string& url, const json& body = json{});
        Response post(const std::string& url, const json& body);
        Response put(const std::string& url, const json& body);
        Response del(const std::string& url, const json& body = json{});

    private:
        ssl::context ctx_;
        std::chrono::milliseconds timeout_ = std::chrono::seconds(30);

        struct UrlParts {
            std::string protocol;
            std::string host;
            std::string port;
            std::string path;
        };

        UrlParts parse_url(const std::string& url);
        Response send_request(const std::string& url, http::verb method, const json& body);
        Response send_http_request(const UrlParts& url_parts, http::verb method, const json& body);
        Response send_https_request(const UrlParts& url_parts, http::verb method, const json& body);
        void setup_common_headers(Request& req, const std::string& host);
        std::string json_to_query_string(const json& j);
        std::string encode_url(const std::string& value);
        void set_timeout(const std::chrono::milliseconds& timeout);
        bool is_success(const Response& res);

    };
}