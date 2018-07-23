#pragma once

#include <eosio/sql_db_plugin/table.hpp>

#include <chrono>

#include <eosio/chain/block_state.hpp>

namespace eosio {

class blocks_table : public mysql_table {
    public:
        blocks_table(std::shared_ptr<soci::session> session);

        void drop();
        void create();
        // void add(chain::signed_block_ptr block);
        void add( const chain::block_state_ptr& );
        void irreversible_set( std::string block_id, bool irreversible );

    private:
        std::shared_ptr<soci::session> m_session;
};

} // namespace


