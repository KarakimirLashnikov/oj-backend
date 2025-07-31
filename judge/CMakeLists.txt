file(GLOB
    core_sources
    "include/*.hpp"
    "src/*.cpp"
)

add_library(
    JudgeLayer STATIC ${core_sources}
)

target_include_directories(
    JudgeLayer
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cpp-httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)