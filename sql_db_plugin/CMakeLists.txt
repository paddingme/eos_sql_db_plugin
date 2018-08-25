find_package(Soci)
if(NOT SOCI_FOUND)
    message(STATUS "Database SQL plugin: disabled")
    return()
endif()

FIND_LIBRARY(
    MYSQLCLIENT
    NAMES mysqlclient
    HINTS /usr/local/lib /usr/lib
    PATH_SUFFIXES lib${LIB_SUFFIX})
MARK_AS_ADVANCED(MYSQLCLIENT)

if( MYSQLCLIENT EQUAL NO_DEFAULT_PATH )
    message(STATUS "Database MYSQL plugin: disabled")
    return()
endif()

message(STATUS "Database SQL plugin: enabled")
file(GLOB HEADERS "include/eosio/sql_db_plugin/*.hpp")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} include db)

add_library(sql_db_plugin
    db/database.cpp
    db/accounts_table.cpp
    db/transactions_table.cpp
    db/blocks_table.cpp
    db/actions_table.cpp
    sql_db_plugin.cpp
    )

target_link_libraries(sql_db_plugin
    chain_plugin
    eosio_chain
    ${SOCI_LIBRARY}
    ${MYSQLCLIENT}
    )
target_include_directories( sql_db_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
#add_subdirectory(test)

