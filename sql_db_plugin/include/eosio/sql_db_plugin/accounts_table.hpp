#pragma once

#include <eosio/sql_db_plugin/table.hpp>

namespace eosio {

using std::string;

class accounts_table  : public mysql_table {
    public:
        accounts_table(){};
        accounts_table(std::shared_ptr<soci::session> session);
        accounts_table(std::shared_ptr<soci_session_pool> session_pool);

        void add(string name);
        bool exist(string name);
        void add_eosio(string name,string abi);

    private:
        // std::shared_ptr<soci::session> m_session;
        std::shared_ptr<soci_session_pool> m_session_pool;
};

} // namespace

