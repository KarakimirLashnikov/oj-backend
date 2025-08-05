include(ExternalProject)
set(MYSQL_CONNECTOR_VERSION 8.4.5-0ubuntu0.2) # 填入正确的版本即可
set(MYSQL_CONNECTOR_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mysql)
set(MYSQL_CONNECTOR_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/mysql-build)
set(MYSQL_CONNECTOR_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/mysql-install)

ExternalProject_Add(
    mysql_connector_cpp 
    URL "https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-${MYSQL_CONNECTOR_VERSION}-src.tar.gz" 
    SOURCE_DIR "${MYSQL_CONNECTOR_SOURCE_DIR}"
    BINARY_DIR "${MYSQL_CONNECTOR_BUILD_DIR}"
    INSTALL_DIR "${MYSQL_CONNECTOR_INSTALL_DIR}"
    CMAKE_ARGS 
        -DCMAKE_INSTALL_
)


file(
    GLOB 
    database_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library( 
    database
)

target_sources(
    database
    PRIVATE 
    ${database_sources}
)

