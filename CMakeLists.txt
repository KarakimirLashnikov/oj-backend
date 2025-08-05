cmake_minimum_required(VERSION 3.28 FATAL_ERROR)
project(OJ-BACKEND VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/judge
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/exceptions
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/inih
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/kafel/include
)

add_subdirectory(app)
add_subdirectory(core)
add_subdirectory(judge)
add_subdirectory(exceptions)
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
)

target_link_libraries(
    oj-backend 
    PRIVATE 
    ThirdParty
    CoreLayer 
    JudgeLayer
    AppLayer
    ExceptionsLayer
)