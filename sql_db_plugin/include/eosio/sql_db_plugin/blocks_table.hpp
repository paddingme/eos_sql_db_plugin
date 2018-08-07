#pragma once

#include <eosio/sql_db_plugin/table.hpp>

#include <chrono>

#include <eosio/chain/block_state.hpp>

namespace eosio {

class blocks_table : public mysql_table {
    public:
        blocks_table(std::shared_ptr<soci::session> session);
        blocks_table(std::shared_ptr<soci_session_pool> session_pool);

        // void add(chain::signed_block_ptr block);
        void add( const chain::block_state_ptr& );
        bool irreversible_set( std::string block_id, bool irreversible );

    private:
        // std::shared_ptr<soci::session> m_session;
        std::shared_ptr<soci_session_pool> m_session_pool;
};

} // namespace


