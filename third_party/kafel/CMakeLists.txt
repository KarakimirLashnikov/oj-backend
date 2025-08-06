cmake_minimum_required(VERSION 3.13)
project(kafel C)

# 设置编译选项
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找依赖工具
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_program(OBJCOPY NAMES objcopy)
find_program(OBJDUMP NAMES objdump)

# 生成的源文件
set(GENERATED_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.c
    ${CMAKE_CURRENT_BINARY_DIR}/parser.c
)

# 源文件列表
set(SRCS
    src/kafel.c
    src/context.c
    src/codegen.c
    src/expression.c
    src/includes.c
    src/parser_types.c
    src/policy.c
    src/range_rules.c
    src/syscall.c
    src/syscalls/amd64_syscalls.c
    src/syscalls/i386_syscalls.c
    src/syscalls/aarch64_syscalls.c
    src/syscalls/mipso32_syscalls.c
    src/syscalls/mips64_syscalls.c
    src/syscalls/arm_syscalls.c
    src/syscalls/riscv64_syscalls.c
    ${GENERATED_SRCS}
)

# 使用绝对路径引用源文件
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lexer.c ${CMAKE_CURRENT_BINARY_DIR}/lexer.h
    COMMAND ${FLEX_EXECUTABLE} --header-file=${CMAKE_CURRENT_BINARY_DIR}/lexer.h -o ${CMAKE_CURRENT_BINARY_DIR}/lexer.c ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.l
    COMMENT "Generating lexer"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parser.c ${CMAKE_CURRENT_BINARY_DIR}/parser.h
    COMMAND ${BISON_EXECUTABLE} -d -o ${CMAKE_CURRENT_BINARY_DIR}/parser.c ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
    COMMENT "Generating parser"
)

# 禁用生成代码的警告
set_source_files_properties(${GENERATED_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-error")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(kafel_static STATIC ${SRCS})
set_target_properties(kafel_static PROPERTIES OUTPUT_NAME kafel)