cmake_minimum_required(VERSION 3.28 FATAL_ERROR)
project(OJ-BACKEND VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/judge
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/inih
)

add_subdirectory(core)
add_subdirectory(judge)
add_subdirectory(third_party)


# 在Windows上需要显式链接filesystem库
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++fs")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

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

# 定义配置文件源路径和目标路径
set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config/config.ini")
set(TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# 复制配置文件到构建目录
add_custom_command(
    TARGET oj-backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CONFIG_FILE}"
        "${TARGET_DIR}/config.ini"
    COMMENT "Copying config.ini to executable directory"
)