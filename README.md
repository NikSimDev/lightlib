# LightLib Framework
**LightLib** — это современный C++ MVC фреймворк, разработанный для создания высокопроизводительных асинхронных API. Фреймворк сочетает в себе мощь Boost.Asio с современными возможностями C++20/23.
##  Особенности

-   **Полная асинхронность** на основе корутин Boost.Asio
    
-   **MVC архитектура** с четким разделением ответственности
    
-   **Встроенная система миграций** БД
    
-   **Поддержка JWT аутентификации**
    
-   **Кэширование через Redis**
    
-   **Высокая производительность** с минимальными накладными расходами
## Требования
### **Обязательные компоненты:**

-   **PostgreSQL** 12+ (для работы с базой данных)
    
-   **CMake** 3.15+ (система сборки)
    
-   **vcpkg** (менеджер зависимостей C++)
    
-   **C++ компилятор** с поддержкой C++20 (GCC 10+, Clang 10+, MSVC 2019 16.8+)
-   **Файл .env** с конфигурацией окружения
    
-   **LightLib библиотеки** (предварительно собранные)

### **Поддерживаемые платформы:**

-   **Windows** 10/11 (Visual Studio 2019/2022)
    
-   **Linux** (Ubuntu 20.04+, CentOS 8+)
    
-   **macOS** 11+ (Apple Clang) 	

LightLib **требует** наличие файла `.env` в корне проекта со следующими параметрами:
```
# Серверные настройки
S_HOST=127.0.0.1
S_PORT=3500

# База данных PostgreSQL
DB_HOST=127.0.0.1
DB_PORT=5432
DB_USERNAME=your_username
DB_PASSWORD=your_password
DB_DATABASE=lightlib_db

# Redis для кэширования и сессий
REDIS_HOST=127.0.0.1
REDIS_PORT=6379

# Безопасность
AUTH_SECRET=your_super_secret_jwt_key_here
```
## Структура проекта
```
launchpad/
├── .env                          # Конфигурация окружения
├── CMakeLists.txt               # Файл сборки
├── vcpkg.json                   # Зависимости vcpkg
├── Launchpad/
│   └── Proj.cpp           # Главный файл приложения
├── libs/
│   └── LightLib/
│       ├── x64-windows/        # Windows библиотеки
│       └── x64-linux/          # Linux библиотеки
└── include/                     # Заголовочные файлы
```

## Файлы конфигурации
### CmakeLists.txt
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
## Сборка проекта 
```
# Windows
.\build.bat

# Linux/macOS
sh build.sh
```

# Система логирования LightLib
LightLib предоставляет высокопроизводительную систему логирования с поддержкой цветного вывода, ротацией логов и обработкой сигналов. Логирование осуществляется как в файл, так и в консоль с цветовой разметкой.
## Основные возможности

-   **Цветное логирование** в консоль
    
-   **Автоматическая ротация** логов
    
-   **Потокобезопасность** с использованием мьютексов
    
-   **Высокая производительность** с кэшированием времени
    
-   **Обработка сигналов** для graceful shutdown
    
-   **Логирование в файл** с буферизацией

| Уровень  | Цвет |Описание|
| ------------- | ------------- |------------- |
| ERROR  | 🔴 Красный  |Критические ошибки  |
| WARNING  | 🟡 Желтый  |Предупреждения  |
| INFO  |   🔵 Синий  |Информационные сообщения  |
| SUCCESS  |   🟢 Зеленый  |Успешные операции  |
| DEBUG|     |Отладочная информация  |

### Примеры использования

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
### Автоматическая ротация логов

Система автоматически ротирует логи при достижении лимита строк:

-   Текущий файл: `app.log`
    
-   Старый файл: `app_old.log`
    
-   Лимит по умолчанию: 10,000 строк
### Кастомные уровни логирования

```
Logger::log("Custom business event", "BUSINESS");
Logger::log("Performance metric", "METRIC");
Logger::log("Security audit", "AUDIT");
```
## Структура лог-файлов
```
Storage/
└── Logs/
    ├── app.log      # Текущий лог-файл
    └── app_old.log  # Предыдущий лог после ротации
```
## Формат логов

### Консольный вывод (с цветами):
```
2024-01-15 14:30:25 [INFO] Application started
2024-01-15 14:30:26 [SUCCESS] Database connected
2024-01-15 14:30:27 [ERROR] Connection timeout
```
Система логирования LightLib готова к использованию в production-средах и предоставляет все необходимые функции для эффективного мониторинга и отладки приложений.
# Система маршрутизации LightLib
LightLib предоставляет мощную систему маршрутизации с поддержкой асинхронных обработчиков, параметризованных путей и CORS. Система построена на основе Boost.Beast и Boost.Asio корутин.

### 1. Роутер (Router)

Центральный класс для управления маршрутами и обработки запросов.

### 2. Маршрут (Route)

Представляет отдельный endpoint с параметрами и обработчиком.

### 3. Регистратор маршрутов (RouterRegisterer)

Упрощает объявление маршрутов через макросы.

### Регистрация маршрутов
```
auto userController = std::make_shared<UserController>();
    
// Базовые маршруты
R(GET, "/api/users", userController, getUsers);
R(POST, "/api/users", userController, createUser);
    
// Параметризованные маршруты
R(GET, "/api/users/:id", userController, getUserById);
R(PUT, "/api/users/:id", userController, updateUser);
R(DELETE_, "/api/users/:id", userController, deleteUser);
    
// Вложенные параметры
R(GET, "/api/users/:userId/posts/:postId", userController, getUserPost);
    
// CORS для API endpoints
CORS("/api/users", userController);
CORS("/api/users/:id", userController);
```
## Типы маршрутов

### Статические маршруты

```
R(GET, "/api/health", healthController, checkHealth);
R(GET, "/api/version", infoController, getVersion);
```
### Параметризованные маршруты

```
R(GET, "/api/users/:id", userController, getUserById);
R(GET, "/api/categories/:categoryId/products/:productId", productController, getProduct);
```
## Обработка запросов

### Доступ к параметрам
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

### Работа с телом запроса

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
# Контроллеры в LightLib Framework

Контроллеры в LightLib - это центральные компоненты, которые обрабатывают HTTP запросы и возвращают ответы. Они реализуют бизнес-логику приложения, работают с моделями данных и формируют HTTP ответы.

## Архитектура контроллеров

### Базовая структура

```
#include <LightLib/Core>
#include <LightLib/DB>
#include <LightLib/Http>

class UserController : public Controller {
public:
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;
    using Params = std::unordered_map<std::string, std::string>;

    // Асинхронные методы-обработчики
    boost::asio::awaitable<void> actionName(const Request& req, Response& res, const Params& params);
    
    // CORS обработчик
    void setCors(const Request& req, Response& res);
};
```
### **CORS обработчики**

```
void setCors(const Request& req, Response& res) {
    res.set(http::field::access_control_allow_origin, "*");
    res.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
    res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
    res.set(http::field::access_control_allow_credentials, "true");
    res.result(http::status::ok);
}
```
## Работа с запросами

### Парсинг тела запроса

```
boost::asio::awaitable<void> createUser(const Request& req, Response& res, const Params& params) {
    try {
        // Парсинг JSON
        auto body = nlohmann::json::parse(req.body());
        
        // Валидация полей
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

### Работа с параметрами URL

```
boost::asio::awaitable<void> getUser(const Request& req, Response& res, const Params& params) {
    // Получение параметров из URL (например: /users/:id)
    auto userId = params.at("id");
    
    // Использование параметра
    auto user = co_await User::find(std::stoi(userId));
    
    if (!user) {
        res.result(http::status::not_found);
        co_return;
    }
    
    // Формирование ответа
    res.result(http::status::ok);
    res.body() = user->toJson().dump();
    res.set(http::field::content_type, "application/json");
    co_return;
}
```
# Хелперы в LightLib Framework
LightLib предоставляет набор utility-классов (хелперов) для решения распространенных задач: работа с cookies, генерация кодов, валидация данных и другие вспомогательные функции.
## Класс `Code` - Генерация кодов

### Назначение

Генерация случайных кодов для различных целей: верификация, одноразовые пароли, токены и т.д.
### Методы

#### Генерация случайного кода

```
// Случайный код из букв и цифр
std::string code = Code::generateRandomCode(8);
```
#### Генерация числового кода
```
std::string smsCode = Code::generateNumericCode(6);
```
#### Генерация нескольких кодов

```
// 5 случайных кодов по 10 символов
auto codes = Code::generateMultipleCodes(5, 10);
// ["Ab3kL9mN2p", "Xy8zQ1wV4r", ...]

auto numericCodes = Code::generateMultipleCodes(3, 4, true);
// ["3842", "9017", "5623"]
```
### Примеры использования

```
std::string verificationCode = Code::generateRandomCode(16);
std::string smsVerification = Code::generateNumericCode(6);

auto backupCodes = Code::generateMultipleCodes(10, 8);
for (const auto& code : backupCodes) {
    // Сохранение в базу...
}
```

## Класс `Cookie` - Работа с cookies

### Назначение

Упрощенная работа с HTTP cookies: парсинг запросов и установка в ответы.

### Методы

#### Парсинг cookies из запроса
```
auto cookies = Cookie::parseCookies("session=abc123; user=john; theme=dark");

std::string sessionId = cookies["session"]; // "abc123"
std::string userName = cookies["user"];     // "john"
```
#### Установка cookies в ответ

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
### Примеры использования
Установка cookies

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
Чтение cookie
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
## Класс `Validator` - Валидация данных

### Назначение

Проверка корректности пользовательского ввода: пароли, email и другие данные.

### Методы

#### Валидация пароля
```

bool isValid = Validator::password("MySecure123!");
// Требования:
// - Длина: 8-20 символов
// - Минимум одна цифра
// - Минимум одна заглавная буква 
// - Минимум один специальный символ
```

#### Валидация email

```
bool isValid = Validator::email("user@example.com");
// Стандарт RFC 5322
```