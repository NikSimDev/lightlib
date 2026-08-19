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

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include "../include/lightlib/Core"
#include "../include/lightlib/DB"
#include "../include/lightlib/Http"
#include "../include/lightlib/Engine.hpp"

extern std::shared_ptr<lightlib::Server> g_test_server;
extern std::atomic<bool> g_server_ready;
extern std::thread g_server_thread;

extern int port_global;
extern std::string host_global;