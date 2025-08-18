cmake_minimum_required(VERSION 3.28 FATAL_ERROR)
project(OJ-BACKEND VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/config.ini" 
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/kafel.policy" 
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/judge/include
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/app/include
    ${CMAKE_CURRENT_SOURCE_DIR}/exceptions/include
    ${CMAKE_CURRENT_SOURCE_DIR}/database/include
    ${CMAKE_CURRENT_SOURCE_DIR}/database/mysql-connector/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/inih
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/kafel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jwt-cpp/include
)

add_subdirectory(app)
add_subdirectory(core)
add_subdirectory(judge)
add_subdirectory(exceptions)
add_subdirectory(database)
add_subdirectory(third_party)

add_executable(oj-backend)

target_precompile_headers(
    oj-backend PRIVATE
    "core/include/Core.hpp"
    <spdlog/spdlog.h>
    <nlohmann/json.hpp>
    <httplib.h>
)

target_sources(
    oj-backend 
    PRIVATE 
    main.cpp
)

target_include_directories(
    oj-backend 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/judge/include
    ${CMAKE_CURRENT_SOURCE_DIR}/app/include
    ${CMAKE_CURRENT_SOURCE_DIR}/exceptions/include
    ${CMAKE_CURRENT_SOURCE_DIR}/database/include
)
find_package(redis++ REQUIRED CONFIG)
target_link_libraries(
    oj-backend 
    PRIVATE
    ThirdParty
    DatabaseLayer
    CoreLayer 
    JudgeLayer
    AppLayer
    ExceptionsLayer
    redis++::redis++
)
