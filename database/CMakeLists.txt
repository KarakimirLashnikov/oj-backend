# 查找 MySQL Connector/C++ 头文件路径
find_path(MYSQLCPPCONN_INCLUDE_DIR NAMES mysql_connection.h PATHS
    /usr/include/mysql
)

# 查找 MySQL Connector/C++ 库文件
find_library(MYSQLCPPCONN_LIBRARY NAMES mysqlcppconn PATHS
    /usr/lib/x86_64-linux-gnu
)

if(MYSQLCPPCONN_INCLUDE_DIR AND MYSQLCPPCONN_LIBRARY)
    message(STATUS "Found MySQL Connector/C++: ${MYSQLCPPCONN_LIBRARY}")
    add_library(mysqlcppconn INTERFACE IMPORTED)
    target_include_directories(mysqlcppconn INTERFACE ${MYSQLCPPCONN_INCLUDE_DIR})
    target_link_libraries(mysqlcppconn INTERFACE ${MYSQLCPPCONN_LIBRARY})
else()
    message(FATAL_ERROR "MySQL Connector/C++ not found!")
endif()


set(db_sources_root ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(db_sources_header ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(
    GLOB 
    database_sources
    ${db_sources_root}/judgedb/*.cpp
)

add_library( 
    DatabaseLayer
)

target_sources(
    DatabaseLayer
    PRIVATE 
    ${database_sources}
)

target_include_directories(
    DatabaseLayer
    PRIVATE 
    ${db_sources_header}
    ${MYSQL_CONCPP_DIR}/include
)

target_link_libraries(
    DatabaseLayer 
    PRIVATE  
    mysqlcppconn
)
