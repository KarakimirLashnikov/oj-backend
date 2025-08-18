file(GLOB
    core_sources
    "src/*.cpp"
    "src/utilities/*.cpp"
    "src/compilers/*.cpp"
    "src/actuators/*.cpp"
)

find_package(Boost REQUIRED CONFIG COMPONENTS filesystem system)

# 添加 seccomp 查找逻辑
find_library(SECCOMP_LIBRARY NAMES seccomp)
find_path(SECCOMP_INCLUDE_DIR NAMES seccomp.h)

if(SECCOMP_LIBRARY AND SECCOMP_INCLUDE_DIR)
    message(STATUS "Found seccomp: ${SECCOMP_LIBRARY}")
    add_library(seccomp::seccomp UNKNOWN IMPORTED)
    set_target_properties(seccomp::seccomp PROPERTIES
        IMPORTED_LOCATION "${SECCOMP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SECCOMP_INCLUDE_DIR}"
    )
else()
    message(FATAL_ERROR "seccomp library not found! Install libseccomp-dev")
endif()

add_library(
    JudgeLayer 
    SHARED 
    ${core_sources}
)

target_link_libraries(
    JudgeLayer 
    PRIVATE
    seccomp::seccomp
    Boost::filesystem 
    Boost::system
)

target_include_directories(
    JudgeLayer
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../exceptions/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/include/compilers
    ${CMAKE_CURRENT_SOURCE_DIR}/include/actuators
)