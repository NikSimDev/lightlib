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
#include <random>
#include <thread>
#include <optional>
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include "../../../include/lightlib/Filesystem/FileDriver.hpp"

namespace fs = std::filesystem;

class TestBase : public ::testing::Test {
protected:
    fs::path createTempDir() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dist(100000, 999999);
        auto tempRoot = fs::temp_directory_path() / ("lightlib_test_" + std::to_string(dist(gen)));
        fs::create_directories(tempRoot);
        return tempRoot;
    }

    template <typename Awaitable>
    auto runAsync(Awaitable&& awaitable) -> typename std::decay_t<Awaitable>::value_type {
        boost::asio::io_context ctx;
        auto work = boost::asio::make_work_guard(ctx);
        std::thread t([&ctx]() { ctx.run(); });

        using ResultType = typename std::decay_t<Awaitable>::value_type;
        std::optional<ResultType> result;
        std::exception_ptr ex;

        boost::asio::co_spawn(ctx, std::forward<Awaitable>(awaitable),
            [&](std::exception_ptr e, ResultType res) {
                if (e) ex = e;
                else result = std::move(res);
                work.reset();
            });

        t.join();
        if (ex) std::rethrow_exception(ex);
        return std::move(*result);
    }

    void runAsyncVoid(auto&& awaitable) {
        boost::asio::io_context ctx;
        auto work = boost::asio::make_work_guard(ctx);
        std::thread t([&ctx]() { ctx.run(); });

        std::exception_ptr ex;
        boost::asio::co_spawn(ctx, std::forward<decltype(awaitable)>(awaitable),
            [&](std::exception_ptr e) {
                ex = e;
                work.reset();
            });

        t.join();
        if (ex) std::rethrow_exception(ex);
    }
};

class FileDriverTest : public TestBase {
protected:
    void SetUp() override {
        root_ = createTempDir();
        driver_ = std::make_shared<lightlib::FileDriver>();
        driver_->setRootPath(root_.string());
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(root_, ec);
        driver_.reset();
    }

    fs::path root_;
    std::shared_ptr<lightlib::FileDriver> driver_;
};

TEST_F(FileDriverTest, SetRootPathCreatesDirectory) {
    auto newRoot = createTempDir();
    driver_->setRootPath(newRoot.string());
    EXPECT_TRUE(fs::exists(newRoot));
    EXPECT_EQ(driver_->getRootPath(), newRoot.string());
    fs::remove_all(newRoot);
}

TEST_F(FileDriverTest, PutAndGet) {
    const std::string path = "test.txt";
    const std::string content = "Hello, World!";
    driver_->put(path, content);
    EXPECT_TRUE(fs::exists(root_ / path));
    EXPECT_EQ(driver_->get(path), content);
}

TEST_F(FileDriverTest, PutCreatesDirectories) {
    const std::string path = "a/b/c/file.txt";
    const std::string content = "nested";
    driver_->put(path, content);
    EXPECT_TRUE(fs::exists(root_ / path));
    EXPECT_EQ(driver_->get(path), content);
}

TEST_F(FileDriverTest, Exists) {
    const std::string path = "exists.txt";
    EXPECT_FALSE(driver_->exists(path));
    driver_->put(path, "data");
    EXPECT_TRUE(driver_->exists(path));
}

TEST_F(FileDriverTest, DeleteFile) {
    const std::string path = "todelete.txt";
    driver_->put(path, "data");
    EXPECT_TRUE(driver_->exists(path));
    driver_->deleteFile(path);
    EXPECT_FALSE(driver_->exists(path));
}

TEST_F(FileDriverTest, DeleteNonExistentThrows) {
    EXPECT_THROW(driver_->deleteFile("missing.txt"), std::runtime_error);
}

TEST_F(FileDriverTest, GetNonExistentThrows) {
    EXPECT_THROW(driver_->get("missing.txt"), std::runtime_error);
}

TEST_F(FileDriverTest, Copy) {
    const std::string src = "src.txt";
    const std::string dst = "dst.txt";
    const std::string content = "copy me";
    driver_->put(src, content);
    driver_->copy(src, dst);
    EXPECT_TRUE(driver_->exists(src));
    EXPECT_TRUE(driver_->exists(dst));
    EXPECT_EQ(driver_->get(dst), content);
}

TEST_F(FileDriverTest, CopyNonExistentThrows) {
    EXPECT_THROW(driver_->copy("missing.txt", "dest.txt"), std::runtime_error);
}

TEST_F(FileDriverTest, Move) {
    const std::string src = "src.txt";
    const std::string dst = "dst.txt";
    const std::string content = "move me";
    driver_->put(src, content);
    driver_->move(src, dst);
    EXPECT_FALSE(driver_->exists(src));
    EXPECT_TRUE(driver_->exists(dst));
    EXPECT_EQ(driver_->get(dst), content);
}

TEST_F(FileDriverTest, MoveNonExistentThrows) {
    EXPECT_THROW(driver_->move("missing.txt", "dest.txt"), std::runtime_error);
}

TEST_F(FileDriverTest, PutOverwritesExistingFile) {
    const std::string path = "overwrite.txt";
    driver_->put(path, "old content");
    EXPECT_EQ(driver_->get(path), "old content");
    driver_->put(path, "new content");
    EXPECT_EQ(driver_->get(path), "new content");
}

TEST_F(FileDriverTest, GetDriverType) {
    EXPECT_FALSE(driver_->getDriverType().empty());
}

class FileDriverAsyncTest : public FileDriverTest {
protected:
    void SetUp() override {
        FileDriverTest::SetUp();
        driver_->initAsync(2);
    }
};

TEST_F(FileDriverAsyncTest, PutAsyncAndGetAsync) {
    const std::string path = "async.txt";
    const std::string content = "async data";
    runAsyncVoid(driver_->putAsync(path, content));
    auto result = runAsync(driver_->getAsync(path));
    EXPECT_EQ(result, content);
    auto exists = runAsync(driver_->existsAsync(path));
    EXPECT_TRUE(exists);
}

TEST_F(FileDriverAsyncTest, DeleteAsync) {
    const std::string path = "async_del.txt";
    driver_->put(path, "data");
    EXPECT_TRUE(driver_->exists(path));
    runAsyncVoid(driver_->deleteFileAsync(path));
    EXPECT_FALSE(driver_->exists(path));
}

TEST_F(FileDriverAsyncTest, CopyAsync) {
    const std::string src = "async_src.txt";
    const std::string dst = "async_dst.txt";
    const std::string content = "copy async";
    driver_->put(src, content);
    runAsyncVoid(driver_->copyAsync(src, dst));
    EXPECT_TRUE(driver_->exists(dst));
    EXPECT_EQ(driver_->get(dst), content);
}

TEST_F(FileDriverAsyncTest, MoveAsync) {
    const std::string src = "async_src.txt";
    const std::string dst = "async_dst.txt";
    const std::string content = "move async";
    driver_->put(src, content);
    runAsyncVoid(driver_->moveAsync(src, dst));
    EXPECT_FALSE(driver_->exists(src));
    EXPECT_TRUE(driver_->exists(dst));
    EXPECT_EQ(driver_->get(dst), content);
}

TEST_F(FileDriverAsyncTest, AsyncWithoutInitThrows) {
    auto freshDriver = std::make_shared<lightlib::FileDriver>();
    freshDriver->setRootPath(root_.string());
    EXPECT_THROW(runAsyncVoid(freshDriver->putAsync("any", "data")), std::runtime_error);
    EXPECT_THROW(runAsync(freshDriver->getAsync("any")), std::runtime_error);
}

TEST_F(FileDriverAsyncTest, AsyncDeleteNonExistentThrows) {
    EXPECT_THROW(runAsyncVoid(driver_->deleteFileAsync("missing.txt")), std::runtime_error);
}

TEST_F(FileDriverAsyncTest, AsyncCopyNonExistentThrows) {
    EXPECT_THROW(runAsyncVoid(driver_->copyAsync("missing_src.txt", "dst.txt")), std::runtime_error);
}

TEST_F(FileDriverAsyncTest, AsyncMoveNonExistentThrows) {
    EXPECT_THROW(runAsyncVoid(driver_->moveAsync("missing_src.txt", "dst.txt")), std::runtime_error);
}

TEST_F(FileDriverAsyncTest, InitAsyncMultipleTimes) {
    EXPECT_NO_THROW(driver_->initAsync(2));
    EXPECT_NO_THROW(driver_->initAsync(4));
    const std::string path = "after_reinit.txt";
    const std::string content = "content";
    runAsyncVoid(driver_->putAsync(path, content));
    auto result = runAsync(driver_->getAsync(path));
    EXPECT_EQ(result, content);
}