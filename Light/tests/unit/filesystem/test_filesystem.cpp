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

#include <gtest/gtest.h>
#include <filesystem>
#include "../../../include/lightlib/Filesystem/Filesystem.hpp"
#include "../../../include/lightlib/Filesystem/FileDriver.hpp"
#include "../../../include/lightlib/Filesystem/StorageManager.hpp"
#include "test_utils.hpp"

namespace fs = std::filesystem;

class FilesystemTest : public TestBase {
protected:
    void SetUp() override {
        tempRoot_ = createTempDir();
        driver_ = std::make_shared<lightlib::FileDriver>();
        driver_->setRootPath(tempRoot_.string());
        driver_->initAsync(2);
        auto& mgr = lightlib::StorageManager::getInstance();
        for (const auto& name : mgr.getDriverNames()) {
            mgr.unregisterDriver(name);
        }
        mgr.registerDriver("test_driver", driver_);
        mgr.setDefaultDriver("test_driver");
    }

    void TearDown() override {
        fs::remove_all(tempRoot_);
        auto& mgr = lightlib::StorageManager::getInstance();
        for (const auto& name : mgr.getDriverNames()) {
            mgr.unregisterDriver(name);
        }
        driver_.reset();
    }

    fs::path tempRoot_;
    std::shared_ptr<lightlib::FileDriver> driver_;
};

TEST_F(FilesystemTest, DefaultConstructor) {
    lightlib::Filesystem fs;
    EXPECT_EQ(fs.getDriverName(), "default");
    EXPECT_EQ(fs.getDriverType(), "Filesystem");
    fs.put("test.txt", "hello");
    EXPECT_TRUE(driver_->exists("test.txt"));
}

TEST_F(FilesystemTest, ConstructorWithName) {
    lightlib::Filesystem fs("test_driver");
    EXPECT_EQ(fs.getDriverName(), "test_driver");
    fs.put("named.txt", "content");
    EXPECT_TRUE(driver_->exists("named.txt"));
}

TEST_F(FilesystemTest, ConstructorWithDriver) {
    auto customDriver = std::make_shared<lightlib::FileDriver>();
    auto customRoot = createTempDir();
    customDriver->setRootPath(customRoot.string());
    lightlib::Filesystem fs(customDriver);
    EXPECT_EQ(fs.getDriverName(), "custom");
    fs.put("custom.txt", "data");
    EXPECT_TRUE(customDriver->exists("custom.txt"));
    fs::remove_all(customRoot);
}

TEST_F(FilesystemTest, StaticDriverFactory) {
    auto fs = lightlib::Filesystem::driver("test_driver");
    EXPECT_EQ(fs.getDriverName(), "test_driver");
    fs.put("factory.txt", "test");
    EXPECT_TRUE(driver_->exists("factory.txt"));
}

TEST_F(FilesystemTest, StaticHasDriver) {
    EXPECT_TRUE(lightlib::Filesystem::hasDriver("test_driver"));
    EXPECT_FALSE(lightlib::Filesystem::hasDriver("non_existent"));
}

TEST_F(FilesystemTest, MethodsDelegate) {
    lightlib::Filesystem fs("test_driver");
    fs.put("delegate.txt", "delegated");
    EXPECT_TRUE(driver_->exists("delegate.txt"));
    EXPECT_EQ(fs.get("delegate.txt"), "delegated");
    EXPECT_TRUE(fs.exists("delegate.txt"));
    EXPECT_FALSE(fs.exists("missing.txt"));
    fs.copy("delegate.txt", "copy.txt");
    EXPECT_TRUE(driver_->exists("copy.txt"));
    EXPECT_EQ(fs.get("copy.txt"), "delegated");
    fs.move("copy.txt", "moved.txt");
    EXPECT_FALSE(driver_->exists("copy.txt"));
    EXPECT_TRUE(driver_->exists("moved.txt"));
    EXPECT_EQ(fs.get("moved.txt"), "delegated");
    fs.deleteFile("moved.txt");
    EXPECT_FALSE(driver_->exists("moved.txt"));
}

TEST_F(FilesystemTest, AsyncMethods) {
    lightlib::Filesystem fs("test_driver");
    const std::string path = "async_fs.txt";
    const std::string content = "async fs";
    runAsyncVoid(fs.putAsync(path, content));
    auto result = runAsync(fs.getAsync(path));
    EXPECT_EQ(result, content);
    auto exists = runAsync(fs.existsAsync(path));
    EXPECT_TRUE(exists);
    runAsyncVoid(fs.deleteFileAsync(path));
    EXPECT_FALSE(driver_->exists(path));
}

TEST_F(FilesystemTest, ConstructorDefaultNoDriverThrows) {
    auto& mgr = lightlib::StorageManager::getInstance();
    for (const auto& name : mgr.getDriverNames()) {
        mgr.unregisterDriver(name);
    }
    EXPECT_THROW(lightlib::Filesystem fs, std::runtime_error);
}

TEST_F(FilesystemTest, ConstructorWithNameNotFoundThrows) {
    EXPECT_THROW(lightlib::Filesystem fs("unknown_driver"), std::runtime_error);
}

TEST_F(FilesystemTest, GetDriverReturnsNonNull) {
    lightlib::Filesystem fs("test_driver");
    auto driver = fs.getDriver();
    EXPECT_NE(driver, nullptr);
    EXPECT_EQ(driver, driver_);
}