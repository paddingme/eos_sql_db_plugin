#pragma once

#include <eosio/sql_db_plugin/table.hpp>
#include <eosio/chain/transaction_metadata.hpp>

namespace eosio {

class transactions_table : public mysql_table {
    public:
        transactions_table( std::shared_ptr<soci::session> session );
        transactions_table(std::shared_ptr<soci_session_pool> session_pool);

        void add( chain::transaction );
        void add( chain::transaction, chain::block_timestamp_type, uint32_t );
        void add_scheduled_or_except( string, chain::block_timestamp_type, uint32_t );
        void irreversible_set( string, bool, string );
        bool irreversible_set( bool, std::string );
        bool find_transaction( string );
        void delete_transaction( string );
        uint32_t get_transaction_block_num( string );
        void get_irreversible_transaction( string,string&, int64_t& );
        uint32_t get_max_irreversible_block_num();
        bool is_max_block_num_in_current_state( string );
        bool is_current_transaction( string, uint32_t );

    private:
        // std::shared_ptr<soci::session> m_session;
        std::shared_ptr<soci_session_pool> m_session_pool;
    };

} // namespace


