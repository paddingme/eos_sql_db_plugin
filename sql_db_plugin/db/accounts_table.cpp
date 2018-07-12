// #include "accounts_table.hpp"
#include <eosio/sql_db_plugin/accounts_table.hpp>
#include <fc/log/logger.hpp>

namespace eosio {

accounts_table::accounts_table(std::shared_ptr<soci::session> session):
    m_session(session)
{

}

void accounts_table::drop()
{
    try {
        *m_session << "DROP TABLE IF EXISTS accounts_keys";
        *m_session << "DROP TABLE IF EXISTS accounts";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void accounts_table::create()
{
    *m_session << "CREATE TABLE `accounts` ("
                    "`id` bigint(20) NOT NULL AUTO_INCREMENT,"
                    "`name` varchar(12) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '',"
                    "`abi` json DEFAULT NULL,"
                    "`created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                    "`updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
                    "PRIMARY KEY (`id`),"
                    "KEY `idx_accounts_name` (`name`)"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;";

    *m_session << "CREATE TABLE `accounts_keys` ("
                "`id` bigint(20) NOT NULL AUTO_INCREMENT,"
                "`account` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '',"
                "`public_key` varchar(64) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '',"
                "`permission` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '',"
                "PRIMARY KEY (`id`),"
                "KEY `account` (`account`)"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;";
}

void accounts_table::add(string name) {
    *m_session << "INSERT INTO accounts (name) VALUES (:name)",
            soci::use(name);
}

void accounts_table::add_eosio(string name,string abi) {
    *m_session << "INSERT INTO accounts (name,abi) VALUES (:name,:abi)",
            soci::use(name),soci::use(abi);
}

bool accounts_table::exist(string name)
{
    int amount;
    try {
        *m_session << "SELECT COUNT(*) FROM accounts WHERE name = :name", soci::into(amount), soci::use(name);
    }
    catch (std::exception const & e)
    {
        amount = 0;
    }
    return amount > 0;
}

} // namespace
