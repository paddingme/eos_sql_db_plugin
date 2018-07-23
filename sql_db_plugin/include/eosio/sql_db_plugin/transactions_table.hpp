#pragma once

#include <eosio/sql_db_plugin/table.hpp>
#include <eosio/chain/transaction_metadata.hpp>

namespace eosio {

class transactions_table : public mysql_table {
    public:
        transactions_table( std::shared_ptr<soci::session> session );

        void drop();
        void create();
        void add( chain::transaction transaction );
        void irreversible_set( std::string block_id, bool irreversible, std::string transaction_id_str );
        bool find_transaction( std::string transaction_id_str);

    private:
        std::shared_ptr<soci::session> m_session;
    };

} // namespace


