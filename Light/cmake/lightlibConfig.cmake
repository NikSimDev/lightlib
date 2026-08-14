include(CMakeFindDependencyMacro)

find_dependency(Boost CONFIG REQUIRED COMPONENTS beast asio system thread filesystem)
find_dependency(fmt CONFIG REQUIRED)
find_dependency(nlohmann_json CONFIG REQUIRED)
find_dependency(hiredis CONFIG REQUIRED)
find_dependency(ZLIB REQUIRED)
find_dependency(PostgreSQL REQUIRED)
find_dependency(OpenSSL REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/lightlibTargets.cmake")

if(NOT TARGET lightlib::lightlib)
    add_library(lightlib::lightlib ALIAS lightlib)
endif()