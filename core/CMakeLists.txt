file(GLOB
    core_sources
    "include/*.hpp"
    "src/*.cpp"
)

add_library(
    CoreLayer STATIC ${core_sources}
)

target_include_directories(
    CoreLayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/spdlog/include/
)

target_link_libraries(
    CoreLayer 
    PRIVATE 
    ThirdParty
)