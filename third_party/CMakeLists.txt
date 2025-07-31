include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
)

add_library(
    ThirdParty INTERFACE
)

target_include_directories(
    ThirdParty  
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/json/include
)