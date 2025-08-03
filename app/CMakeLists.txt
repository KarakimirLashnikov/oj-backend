add_library(
    AppLayer
)

file(
    GLOB
    app_sources
    "src/api/*.cpp"
    "src/*.cpp"
)

target_sources(
    AppLayer
    PUBLIC
    ${app_sources}
)

target_include_directories(
    AppLayer
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../judge/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/json/include
)