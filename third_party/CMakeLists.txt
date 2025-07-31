add_library(
    ThirdParty INTERFACE
)

add_subdirectory(cpp-httplib)
add_subdirectory(spdlog)
add_subdirectory(json)
add_subdirectory(inih)

# target_include_directories(
#     ThirdParty  
#     INTERFACE
#     ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib
#     ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/json/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/inih
# )

target_link_libraries(
    ThirdParty  
    INTERFACE
    httplib::httplib
    spdlog::spdlog
    nlohmann_json
    inih
)