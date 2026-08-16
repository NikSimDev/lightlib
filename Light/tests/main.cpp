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

#include "main.h"


int main(int argc, char** argv) {
	try {
		testing::InitGoogleTest(&argc, argv);
		lightlib::ConfigManager::initGlobal("config_test.json");

		auto server = lightlib::Server(
			lightlib::global_config->get<std::string>("server.host", "0.0.0.0"),
			lightlib::global_config->get<unsigned short>("server.port", 8080)
		);

		server.run();

		return RUN_ALL_TESTS();
	}
	catch (std::exception& e) {
		lightlib::Logger::log("Exception: " + std::string(e.what()), "ERROR");
		return -1;
	}
}