LightLib Framework
LightLib is a modern C++ MVC framework designed for creating high-performance asynchronous APIs.
The framework combines the power of Boost.Asio with modern C++20/23 features.

## Features

- **Full asynchrony** based on Boost.Asio coroutines
- **MVC architecture** with clear separation of concerns
- **Built-in database migration** system
- **JWT authentication** support
- **Redis caching**
- **High performance** with minimal overhead

## Requirements

### **Required components:**

- **PostgreSQL** 12+ (for database operations)
- **CMake** 3.15+ (build system)
- **vcpkg** (C++ dependency manager)
- **C++ compiler** with C++20 support (GCC 10+, Clang 10+, MSVC 2019 16.8+)
- **.env file** with environment configuration
- **LightLib libraries** (pre-built)

### **Supported platforms:**

- **Windows** 10/11 (Visual Studio 2019/2022)
- **Linux** (Ubuntu 20.04+, CentOS 8+)
- **macOS** 11+ (Apple Clang)

LightLib **requires** a `.env` file in the project root with the following parameters:

```
# Server settings
S_HOST=127.0.0.1
S_PORT=3500

# PostgreSQL database
DB_HOST=127.0.0.1
DB_PORT=5432
DB_USERNAME=your_username
DB_PASSWORD=your_password
DB_DATABASE=lightlib_db

# Redis for caching and sessions
REDIS_HOST=127.0.0.1
REDIS_PORT=6379

# Security
AUTH_SECRET=your_super_secret_jwt_key_here
```

## Project structure

```
launchpad/
├── .env                          # Environment configuration
├── CMakeLists.txt               # Build file
├── vcpkg.json                   # vcpkg dependencies
├── Launchpad/
│   └── Proj.cpp           # Main application file
├── libs/
│   └── LightLib/
│       ├── x64-windows/        # Windows libraries
│       └── x64-linux/          # Linux libraries
└── include/                     # Header files
```

## Configuration files

### CMakeLists.txt

```
cmake_minimum_required(VERSION 3.14)
project(proj VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(WIN32)
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows" CACHE STRING "")
elseif(UNIX)
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux" CACHE STRING "")
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Boost 1.86 REQUIRED COMPONENTS 
    beast 
    asio 
    filesystem 
    system 
    thread 
    date_time 
    smart_ptr 
    optional 
    variant 
    algorithm
)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

if(WIN32)
    set(LIGHTLIB_DIR "${CMAKE_SOURCE_DIR}/libs/LightLib/x64-windows")
elseif(UNIX)
    set(LIGHTLIB_DIR "${CMAKE_SOURCE_DIR}/libs/LightLib/x64-linux")
endif()

add_library(LightLib SHARED IMPORTED)

set_target_properties(LightLib PROPERTIES
    IMPORTED_IMPLIB "${LIGHTLIB_DIR}/lib/LightLib.lib" 
    IMPORTED_LOCATION "${LIGHTLIB_DIR}/bin/LightLib.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${LIGHTLIB_DIR}/include"
)

target_include_directories(LightLib INTERFACE
    "${LIGHTLIB_DIR}"
    "${LIGHTLIB_DIR}/include"
)

target_link_libraries(LightLib INTERFACE
    Boost::beast  
    Boost::asio
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::date_time
    Boost::smart_ptr
    Boost::optional
    Boost::variant
    Boost::algorithm
    fmt::fmt
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    hiredis::hiredis
    ZLIB::ZLIB
    PostgreSQL::PostgreSQL
    OpenSSL::SSL
    OpenSSL::Crypto
)

add_executable(proj Proj/Proj.cpp)
target_link_libraries(proj PRIVATE LightLib)

target_include_directories(proj PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

if(WIN32)
    target_include_directories(proj PRIVATE 
        ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/include
    )
endif()

if(WIN32)
    add_custom_command(TARGET proj POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${LIGHTLIB_DIR}/bin/LightLib.dll"
            $<TARGET_FILE_DIR:proj>
        COMMENT "Copying LightLib.dll to output directory"
    )
endif()
```

### vcpkg.json

```
{
  "name": "launchpad",
  "version": "1.0.0",
  "dependencies": [
    "boost-algorithm",
    "boost-asio",
    "boost-beast",
    "boost-date-time",
    "boost-filesystem",
    "boost-optional",
    "boost-smart-ptr",
    "boost-system",
    "boost-thread",
    "boost-variant",
    "fmt",
    "hiredis",
    "jwt-cpp",
    "libpq",
    "nlohmann-json",
    "zlib"
  ]
}
```

## Building the project

```
# Windows
.\build.bat

# Linux/macOS
sh build.sh
```

# LightLib Logging System

LightLib provides a high-performance logging system with colored output, log rotation, and signal handling.
Logging is done both to file and console with color coding.

## Key Features

- **Colored logging** to console
- **Automatic log rotation**
- **Thread safety** using mutexes
- **High performance** with time caching
- **Signal handling** for graceful shutdown
- **File logging** with buffering

| Level     | Color       | Description              |
|-----------|-------------|--------------------------|
| ERROR     | 🔴 Red       | Critical errors          |
| WARNING   | 🟡 Yellow    | Warnings                 |
| INFO      | 🔵 Blue      | Informational messages   |
| SUCCESS   | 🟢 Green     | Successful operations    |
| DEBUG     |             | Debug information        |

### Usage examples

```
Logger::log("Database connection established", "SUCCESS");

std::string username = "john_doe";
Logger::log("User " + username + " logged in", "INFO");
Logger::log("Debugging username", "DEBUG");

try {
    
} catch (const std::exception& e) {
    Logger::log(std::string("Exception: ") + e.what(), "ERROR");
}
```

### Automatic log rotation

The system automatically rotates logs when the line limit is reached:

- Current file: `app.log`
- Old file: `app_old.log`
- Default limit: 10,000 lines

### Custom log levels

```
Logger::log("Custom business event", "BUSINESS");
Logger::log("Performance metric", "METRIC");
Logger::log("Security audit", "AUDIT");
```

## Log file structure

```
Storage/
└── Logs/
    ├── app.log      # Current log file
    └── app_old.log  # Previous log after rotation
```

## Log format

### Console output (with colors):

```
2024-01-15 14:30:25 [INFO] Application started
2024-01-15 14:30:26 [SUCCESS] Database connected
2024-01-15 14:30:27 [ERROR] Connection timeout
```

The LightLib logging system is ready for use in production environments and provides all necessary functions
for effective application monitoring and debugging.

# LightLib Routing System

LightLib provides a powerful routing system with support for asynchronous handlers, parameterized paths, and CORS.
The system is built on Boost.Beast and Boost.Asio coroutines.

### 1. Router

Central class for managing routes and handling requests.

### 2. Route

Represents a single endpoint with parameters and a handler.

### 3. RouterRegisterer

Simplifies route declaration through macros.

### Route registration

```
auto userController = std::make_shared<UserController>();
    
// Basic routes
R(GET, "/api/users", userController, getUsers);
R(POST, "/api/users", userController, createUser);
    
// Parameterized routes
R(GET, "/api/users/:id", userController, getUserById);
R(PUT, "/api/users/:id", userController, updateUser);
R(DELETE_, "/api/users/:id", userController, deleteUser);
    
// Nested parameters
R(GET, "/api/users/:userId/posts/:postId", userController, getUserPost);
    
// CORS for API endpoints
CORS("/api/users", userController);
CORS("/api/users/:id", userController);
```

## Route types

### Static routes

```
R(GET, "/api/health", healthController, checkHealth);
R(GET, "/api/version", infoController, getVersion);
```

### Parameterized routes

```
R(GET, "/api/users/:id", userController, getUserById);
R(GET, "/api/categories/:categoryId/products/:productId", productController, getProduct);
```

## Request handling

### Accessing parameters

```
boost::asio::awaitable<void> getUserPost(const Request& req, Response& res, const Params& params) {
    auto userId = params.at("userId");
    auto postId = params.at("postId");
    
    auto user = co_await userService.findById(userId);
    auto post = co_await postService.findByUserAndId(userId, postId);
    
    nlohmann::json response = {
        {"user", user},
        {"post", post}
    };
    
    res.result(http::status::ok);
    res.body() = response.dump();
    res.set(http::field::content_type, "application/json");
    co_return;
}
```

### Working with request body

```
boost::asio::awaitable<void> createUser(const Request& req, Response& res, const Params& params) {
    try {
        auto body = nlohmann::json::parse(req.body());
        
        if (!body.contains("email") || !body.contains("name")) {
            res.result(http::status::bad_request);
            co_return;
        }
        
        auto user = co_await userService.create(body);
        
        res.result(http::status::created);
        res.body() = user.toJson().dump();
        res.set(http::field::content_type, "application/json");
        
    } catch (const std::exception& e) {
        res.result(http::status::bad_request);
    }
    co_return;
}
```

# Controllers in LightLib Framework

Controllers in LightLib are central components that handle HTTP requests and return responses.
They implement application business logic, work with data models, and form HTTP responses.

## Controller Architecture

### Base structure

```
#include <LightLib/Core>
#include <LightLib/DB>
#include <LightLib/Http>

class UserController : public Controller {
public:
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;
    using Params = std::unordered_map<std::string, std::string>;

    // Asynchronous handler methods
    boost::asio::awaitable<void> actionName(const Request& req, Response& res, const Params& params);
    
    // CORS handler
    void setCors(const Request& req, Response& res);
};
```

### **CORS handlers**

```
void setCors(const Request& req, Response& res) {
    res.set(http::field::access_control_allow_origin, "*");
    res.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
    res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
    res.set(http::field::access_control_allow_credentials, "true");
    res.result(http::status::ok);
}
```

## Working with requests

### Parsing request body

```
boost::asio::awaitable<void> createUser(const Request& req, Response& res, const Params& params) {
    try {
        // Parse JSON
        auto body = nlohmann::json::parse(req.body());
        
        // Validate fields
        if (!body.contains("name") || !body.contains("email")) {
            res.result(http::status::bad_request);
            res.body() = "Missing required fields";
            co_return;
        }
        
        std::string name = body["name"];
        std::string email = body["email"];
        
    } catch (const std::exception& e) {
        res.result(http::status::bad_request);
        res.body() = "Invalid JSON"
    }
    co_return;
}
```

### Working with URL parameters

```
boost::asio::awaitable<void> getUser(const Request& req, Response& res, const Params& params) {
    // Get parameters from URL (e.g., /users/:id)
    auto userId = params.at("id");
    
    // Use parameter
    auto user = co_await User::find(std::stoi(userId));
    
    if (!user) {
        res.result(http::status::not_found);
        co_return;
    }
    
    // Form response
    res.result(http::status::ok);
    res.body() = user->toJson().dump();
    res.set(http::field::content_type, "application/json");
    co_return;
}
```

# Helpers in LightLib Framework

LightLib provides a set of utility classes (helpers) for common tasks: working with cookies, code generation,
data validation, and other helper functions.

## `Code` Class - Code Generation

### Purpose

Generate random codes for various purposes: verification, one-time passwords, tokens, etc.

### Methods

#### Generate random code

```
// Random code with letters and digits
std::string code = Code::generateRandomCode(8);
```

#### Generate numeric code

```
std::string smsCode = Code::generateNumericCode(6);
```

#### Generate multiple codes

```
// 5 random codes of 10 characters each
auto codes = Code::generateMultipleCodes(5, 10);
// ["Ab3kL9mN2p", "Xy8zQ1wV4r", ...]

auto numericCodes = Code::generateMultipleCodes(3, 4, true);
// ["3842", "9017", "5623"]
```

### Usage examples

```
std::string verificationCode = Code::generateRandomCode(16);
std::string smsVerification = Code::generateNumericCode(6);

auto backupCodes = Code::generateMultipleCodes(10, 8);
for (const auto& code : backupCodes) {
    // Save to database...
}
```

## `Cookie` Class - Working with Cookies

### Purpose

Simplified HTTP cookie handling: parsing from requests and setting in responses.

### Methods

#### Parse cookies from request

```
auto cookies = Cookie::parseCookies("session=abc123; user=john; theme=dark");

std::string sessionId = cookies["session"]; // "abc123"
std::string userName = cookies["user"];     // "john"
```

#### Set cookies in response

```
std::map<std::string, std::string> cookies = {
    {"session_id", "xyz789"}
};
Cookie::set(response, cookies);

std::map<std::string, std::string> multipleCookies = {
    {"user_id", "42"},
    {"theme", "dark"},
    {"language", "en"}
};
Cookie::set(response, multipleCookies);
```

### Usage examples

Setting cookies

```
boost::asio::awaitable<void> login(const Request& req, Response& res, const Params& params) {
    std::map<std::string, std::string> authCookies = {
        {"auth_token", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"},
        {"user_id", "12345"},
        {"session_id", Code::generateRandomCode(32)}
    };
    Cookie::set(res, authCookies);
    
    res.result(http::status::ok);
    co_return;
}
```

Reading cookies

```
boost::asio::awaitable<bool> checkAuth(const Request& req) {
    auto cookieHeader = req.find(http::field::cookie);
    if (cookieHeader != req.end()) {
        auto cookies = Cookie::parseCookies(std::string(cookieHeader->value()));
        if (cookies.find("auth_token") != cookies.end()) {
            
            return true;
        }
    }
    return false;
}
```

## `Validator` Class - Data Validation

### Purpose

Validate user input: passwords, email, and other data.

### Methods

#### Password validation

```

bool isValid = Validator::password("MySecure123!");
// Requirements:
// - Length: 8-20 characters
// - At least one digit
// - At least one uppercase letter
// - At least one special character
```

#### Email validation

```
bool isValid = Validator::email("user@example.com");
// RFC 5322 standard
```