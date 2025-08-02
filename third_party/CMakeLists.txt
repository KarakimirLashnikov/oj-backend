add_library(
    ThirdParty INTERFACE
)

add_subdirectory(cpp-httplib)
add_subdirectory(spdlog)
add_subdirectory(json)
add_subdirectory(inih)

target_link_libraries(
    ThirdParty  
    INTERFACE
    httplib::httplib
    spdlog::spdlog
    nlohmann_json
    inih
)