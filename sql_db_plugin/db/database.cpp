// #include "database.hpp"
#include <eosio/sql_db_plugin/database.hpp>

namespace eosio
{

    database::database(const std::string &uri, uint32_t block_num_start) {
        m_session = std::make_shared<soci::session>(uri); 
        m_accounts_table = std::make_unique<accounts_table>(m_session);
        m_blocks_table = std::make_unique<blocks_table>(m_session);
        m_transactions_table = std::make_unique<transactions_table>(m_session);
        m_actions_table = std::make_unique<actions_table>(m_session);
        m_block_num_start = block_num_start;
        system_account = chain::name(chain::config::system_account_name).to_string();
    }

    void database::wipe() {
        m_actions_table->drop();
        // m_transactions_table->drop();
        //TODO
        // m_blocks_table->drop();
        m_accounts_table->drop();

        // m_transactions_table->create();
        m_actions_table->create();
        m_accounts_table->create();
        // m_blocks_table->create();
        chain::abi_def abi_def;
        abi_def = eosio_contract_abi(abi_def);
        m_accounts_table->add_eosio(system_account, fc::json::to_string( abi_def ));
    }

    bool database::is_started() {
        ilog(system_account);
        return m_accounts_table->exist(system_account);
    }

    void database::consume_block_state( const chain::block_state_ptr& bs) {
        //TODO
    }

    void database::consume_irreversible_block_state( const chain::block_state_ptr& bs){
        //TODO
    }

    void database::consume_transaction_metadata( const chain::transaction_metadata_ptr& tm ) {

        for(auto actions : tm->trx.actions){
            uint8_t seq = 0;
            m_actions_table->add(actions,tm->trx.id(), tm->trx.expiration, seq);
            seq++;
        }

    }

    void database::consume_transaction_trace( const chain::transaction_trace_ptr& tt) {
        // ilog("${data}",("data",fc::json::to_string(*tt)));
        // for(auto actions : tt->action_traces){
        //     uint8_t seq = 0;
        //     m_actions_table->add(actions.act,tt->id, tt->elapsed, seq);
        //     seq++;
        // }
    }

    const std::string database::block_states_col = "block_states";
    const std::string database::blocks_col = "blocks";
    const std::string database::trans_col = "transactions";
    const std::string database::trans_traces_col = "transaction_traces";
    const std::string database::actions_col = "actions";
    const std::string database::accounts_col = "accounts";


} // namespace
