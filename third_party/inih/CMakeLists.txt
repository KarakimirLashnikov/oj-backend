add_library(
    inih 
    STATIC 
    ini.h
    cpp/INIReader.h
    ini.c
    cpp/INIReader.cpp
)

target_include_directories(
    inih 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)