#pragma once

#include <eosio/sql_db_plugin/table.hpp>

#include <vector>

#include <eosio/chain/trace.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/abi_serializer.hpp>

namespace eosio {

using namespace std;

class traces_table : public mysql_table {
    public:
        traces_table( std::shared_ptr<soci::session> session );

        void drop();
        void create();
        void add( const chain::transaction_trace_ptr& );
        void list( string );
        auto add_data(chain::action action);
        void parse_actions( chain::action action );
        void dfs_inline_traces( vector<chain::action_trace> itc );
        // void irreversible_set( std::string block_id, bool irreversible, std::string transaction_id_str );
        // bool find_transaction( std::string transaction_id_str);

    private:
        std::shared_ptr<soci::session> m_session;
    };

} // namespace


