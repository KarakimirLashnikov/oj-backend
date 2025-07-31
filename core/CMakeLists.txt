file(GLOB
    core_sources
    "include/*.hpp"
    "src/*.cpp"
)

add_library(
    CoreLayer 
    STATIC 
    ${core_sources}
)

target_include_directories(
    CoreLayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)