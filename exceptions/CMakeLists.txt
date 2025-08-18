file(
    GLOB
    exceptions_src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_library(
    ExceptionsLayer
    SHARED 
)

target_sources(
    ExceptionsLayer
    PRIVATE 
    ${exceptions_src}
)

target_include_directories(
    ExceptionsLayer
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)