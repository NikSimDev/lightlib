# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.14)
project(lightlib VERSION 0.1.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(CMAKE_THREAD_PREFER_PTHREAD OFF)
    set(THREADS_PREFER_PTHREAD_FLAG OFF)
    set(CMAKE_USE_WIN32_THREADS_INIT ON)
    set(CMAKE_THREAD_LIBS_INIT "")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_PTHREADS_INIT 0)
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(MSVC)
    add_compile_options(/utf-8 /bigobj)
endif()

find_package(Boost CONFIG REQUIRED COMPONENTS beast asio filesystem system thread date_time smart_ptr optional variant algorithm)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES
    "src/*.cpp"
    "src/Database/*.cpp"
    "src/Database/Migrations/*.cpp"
    "src/Router/*.cpp"
    "src/Filesystem/*.cpp"
    "src/Cryptography/*.cpp"
)

list(FILTER PROJECT_SOURCES EXCLUDE REGEX "app/Main\\.cpp$")

add_library(lightlib_static STATIC ${PROJECT_SOURCES})
add_library(lightlib_shared SHARED ${PROJECT_SOURCES})

if(MSVC)
    set_target_properties(lightlib_shared PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

set(LIBRARIES
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

target_include_directories(lightlib_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(lightlib_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(lightlib_static PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/Database
    ${CMAKE_SOURCE_DIR}/Router
    ${CMAKE_SOURCE_DIR}/App
    ${CMAKE_SOURCE_DIR}/Storage
)

target_include_directories(lightlib_shared PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/Database
    ${CMAKE_SOURCE_DIR}/Router
    ${CMAKE_SOURCE_DIR}/App
    ${CMAKE_SOURCE_DIR}/Storage
)

target_link_libraries(lightlib_static PUBLIC ${LIBRARIES})
target_link_libraries(lightlib_shared PUBLIC ${LIBRARIES})

target_compile_features(lightlib_static PUBLIC cxx_std_20)
target_compile_features(lightlib_shared PUBLIC cxx_std_20)

target_compile_definitions(lightlib_shared PRIVATE LIGHTLIB_EXPORTS)
target_compile_definitions(lightlib_shared PUBLIC LIGHTLIB_SHARED)
target_compile_definitions(lightlib_static PUBLIC)

target_compile_definitions(lightlib_static PUBLIC 
    BOOST_ASIO_HAS_CO_AWAIT
    BOOST_ASIO_HAS_STD_COROUTINE
)

target_compile_definitions(lightlib_shared PUBLIC 
    BOOST_ASIO_HAS_CO_AWAIT
    BOOST_ASIO_HAS_STD_COROUTINE
)

export(TARGETS lightlib_static lightlib_shared
    NAMESPACE lightlib::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/lightlibTargets.cmake"
)

add_library(lightlib INTERFACE)
target_link_libraries(lightlib INTERFACE 
    lightlib_static
    lightlib_shared
)

install(TARGETS lightlib_static
    EXPORT lightlibTargets
    ARCHIVE DESTINATION lib
)

install(TARGETS lightlib_shared
    EXPORT lightlibTargets
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)

install(TARGETS lightlib
    EXPORT lightlibTargets
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT lightlibTargets
    NAMESPACE lightlib::
    DESTINATION share/lightlib
    FILE lightlibTargets.cmake      
)

install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lightlibConfig.cmake"
    DESTINATION share/lightlib
)