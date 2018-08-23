// #include "database.hpp"
#include <eosio/sql_db_plugin/database.hpp>

namespace eosio
{

    sql_database::sql_database(const std::string &uri, uint32_t block_num_start, size_t pool_size) {
        m_session_pool          = std::make_shared<soci_session_pool>(pool_size,uri);
        m_accounts_table        = std::make_unique<accounts_table>();
        m_blocks_table          = std::make_unique<blocks_table>();
        m_transactions_table    = std::make_unique<transactions_table>();
        m_actions_table         = std::make_unique<actions_table>();
        m_block_num_start       = block_num_start;
        system_account          = chain::name(chain::config::system_account_name).to_string();
    }

    sql_database::sql_database(const std::string &uri, uint32_t block_num_start, size_t pool_size, std::vector<string> filter_on, std::vector<string> filter_out) {
        new (this)sql_database( uri, block_num_start, pool_size );
        m_action_filter_on = filter_on;
        m_contract_filter_out = filter_out;
    }

    void sql_database::wipe() {
        chain::abi_def abi_def;
        abi_def = eosio_contract_abi(abi_def);
        m_accounts_table->add_eosio( m_session_pool->get_session(), system_account, fc::json::to_string( abi_def ));
    }

    bool sql_database::is_started() {
        return m_accounts_table->exist( m_session_pool->get_session(), system_account );
    }

    void sql_database::consume_block_state( const chain::block_state_ptr& bs) {
        // auto m_session = m_session_pool->get_session();
        auto block_id = bs->id.str();

        for(auto& receipt : bs->block->transactions) {
            string trx_id_str;
            if( receipt.trx.contains<chain::packed_transaction>() ){
                const auto& trx = fc::raw::unpack<chain::transaction>( receipt.trx.get<chain::packed_transaction>().get_raw_transaction() );

                if(trx.actions.size()==1 && trx.actions[0].name.to_string() == "onblock" ) continue ;

                for(auto actions : trx.actions){
                    m_actions_table->add( m_session_pool->get_session(), actions,trx.id(), bs->block->timestamp, m_action_filter_on);
                }

            }         

        }
    }

} // namespace
