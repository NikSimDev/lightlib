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

#include "WSRouter.hpp"

namespace lightlib {

    class WebSocketRouterRegisterer {
    public:
        static void init();
    };

#define WS_R(path, controller, handler) \
    lightlib::WebSocketRouter::instance().add_route(path, \
        [controller](std::shared_ptr<lightlib::WebSocketSession> session, const lightlib::Params& params) { \
            controller->handler(session, params); \
        })

#define WS_M(path, controller, handler) \
    do { \
        lightlib::Params dummy_params; \
        auto* route = lightlib::WebSocketRouter::instance().find_route(path, dummy_params); \
        if (route) { \
            route->set_message_handler([controller](const std::string& msg, std::shared_ptr<lightlib::WebSocketSession> session) { \
                controller->handler(msg, session); \
            }); \
        } \
    } while(0)

#define WS_B(path, controller, handler) \
    do { \
        lightlib::Params dummy_params; \
        auto* route = lightlib::WebSocketRouter::instance().find_route(path, dummy_params); \
        if (route) { \
            route->set_binary_handler([controller](std::vector<uint8_t>&& data, std::shared_ptr<lightlib::WebSocketSession> session) { \
                controller->handler(std::move(data), session); \
            }); \
        } \
    } while(0)

#define WS_C(path, controller, handler) \
    do { \
        lightlib::Params dummy_params; \
        auto* route = lightlib::WebSocketRouter::instance().find_route(path, dummy_params); \
        if (route) { \
            route->set_close_handler([controller](std::shared_ptr<lightlib::WebSocketSession> session) { \
                controller->handler(session); \
            }); \
        } \
    } while(0)

#define WS_E(path, controller, handler) \
    do { \
        lightlib::Params dummy_params; \
        auto* route = lightlib::WebSocketRouter::instance().find_route(path, dummy_params); \
        if (route) { \
            route->set_error_handler([controller](const std::string& error, std::shared_ptr<lightlib::WebSocketSession> session) { \
                controller->handler(error, session); \
            }); \
        } \
    } while(0)

}