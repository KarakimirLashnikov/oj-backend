add_library(
    AppLayer
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

target_compile_options(AppLayer PRIVATE ${SODIUM_CFLAGS_OTHER})

file(
    GLOB
    app_sources
    "src/api/*.cpp"
    "src/authentication/*.cpp"
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
    ${SODIUM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../judge/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../exceptions/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/json/include
)

target_link_libraries(AppLayer PRIVATE ${SODIUM_LIBRARIES})