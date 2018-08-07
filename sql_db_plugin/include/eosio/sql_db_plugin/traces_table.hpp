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
        traces_table(std::shared_ptr<soci_session_pool> session_pool);

        void add( const chain::transaction_trace_ptr& );
        std::string list( string, chain::block_timestamp_type );
        void parse_traces(chain::transaction_trace trace);
        auto add_data(chain::action action);
        void parse_actions( chain::action action );
        void dfs_inline_traces( vector<chain::action_trace> itc );
        // void irreversible_set( std::string block_id, bool irreversible, std::string transaction_id_str );
        // bool find_transaction( std::string transaction_id_str);

        long long block_timestamp;

    private:
        // std::shared_ptr<soci::session> m_session;
        std::shared_ptr<soci_session_pool> m_session_pool;
    };

} // namespace


