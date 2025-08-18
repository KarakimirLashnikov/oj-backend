add_library(
    ThirdParty INTERFACE
)

add_subdirectory(cpp-httplib)

set(SPDLOG_BUILD_SHARED ON CACHE BOOL "Build spdlog as shared library" FORCE)
add_subdirectory(spdlog)

add_subdirectory(json)

add_subdirectory(inih)

add_subdirectory(kafel)

add_subdirectory(jwt-cpp)

# 查找 MySQL Connector/C++ 头文件路径
find_path(SODIUM_INCLUDE_DIR NAMES sodium.h PATHS
    /usr/include/sodium
)

# 查找 MySQL Connector/C++ 库文件
find_library(SODIUM_LIBRARY NAMES sodium PATHS
    /usr/lib/x86_64-linux-gnu
)

target_include_directories(
    ThirdParty
    INTERFACE 
    ${SODIUM_INCLUDE_DIR}
)

target_link_libraries(
    ThirdParty  
    INTERFACE
    httplib::httplib
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    inih
    kafel_shared
    jwt-cpp
    ${SODIUM_LIBRARY}
)