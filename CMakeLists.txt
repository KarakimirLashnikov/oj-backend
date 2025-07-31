cmake_minimum_required(VERSION 3.28 FATAL_ERROR)
project(OJ-BACKEND VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/judge
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
)

add_subdirectory(core)
add_subdirectory(judge)
add_subdirectory(third_party)

add_executable(
    oj-backend
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
)


target_link_libraries(
    oj-backend 
    PRIVATE 
    ThirdParty
    CoreLayer 
    JudgeLayer
)