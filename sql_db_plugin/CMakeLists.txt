find_package(Soci)
if(NOT SOCI_FOUND)
    message(STATUS "Database SQL plugin: disabled")
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
    db/traces_table.cpp
    sql_db_plugin.cpp
    )

target_link_libraries(sql_db_plugin
    chain_plugin
    eosio_chain
    ${SOCI_LIBRARY}
    )
target_include_directories( sql_db_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
#add_subdirectory(test)

