// #include "database.hpp"
#include <eosio/sql_db_plugin/database.hpp>

namespace eosio
{

    database::database(const std::string &uri, uint32_t block_num_start) {
        m_session = std::make_shared<soci::session>(uri); 
        m_accounts_table = std::make_unique<accounts_table>(m_session);
        m_blocks_table = std::make_unique<blocks_table>(m_session);
        m_traces_table = std::make_unique<traces_table>(m_session);
        m_transactions_table = std::make_unique<transactions_table>(m_session);
        m_actions_table = std::make_unique<actions_table>(m_session);
        m_block_num_start = block_num_start;
        system_account = chain::name(chain::config::system_account_name).to_string();
    }

    database::database(const std::string &uri, uint32_t block_num_start, std::vector<string> filter_out) {
        new (this)database(uri,block_num_start);
        m_action_filter_on = filter_out;
    }

    void database::wipe() {
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
        m_blocks_table->add(bs);
    }

    void database::consume_irreversible_block_state( const chain::block_state_ptr& bs ){
        //TODO
        // ilog("run consume irreversible block");
        auto block_id = bs->id.str();
        // do{
        //     bool update_irreversible = m_blocks_table->irreversible_set(block_id, true);
        //     if(update_irreversible || exit) break;
        //     else condition.timed_wait(lock_db, boost::posix_time::milliseconds(10));
        // }while(!exit);

        for(auto& receipt : bs->block->transactions) {
            string trx_id_str;
            if( receipt.trx.contains<chain::packed_transaction>() ){
                const auto& trx = fc::raw::unpack<chain::transaction>( receipt.trx.get<chain::packed_transaction>().get_raw_transaction() );

                if(trx.actions.size()==1 && trx.actions[0].name.to_string() == "onblock" ) continue ;

                // ilog("run irreversible ${result}",("result",m_action_filter_on.size()));
                // m_transactions_table->add(trx);
                for(auto actions : trx.actions){
                    m_actions_table->add(actions,trx.id(), bs->block->timestamp, m_action_filter_on);
                }  

            }else{
                trx_id_str = receipt.trx.get<chain::transaction_id_type>().str();
            }

        }

    }

    void database::consume_irreversible_block_for_traces_state( const tx_id_block_time& traces_params, boost::mutex::scoped_lock& lock_db, boost::condition_variable& condition, boost::atomic<bool>& exit ){
        bool trace_result;
        do{
            trace_result = m_traces_table->list(traces_params.tx_id, traces_params.block_time);
            if(trace_result || exit) break;
            else condition.timed_wait(lock_db, boost::posix_time::milliseconds(10));     
        }while((!exit));
    }

    void database::consume_transaction_metadata( const chain::transaction_metadata_ptr& tm ) {

        // if(tm->trx.actions.size()==1 && tm->trx.actions[0].name.to_string() == "onblock" ) return ;

        m_transactions_table->add(tm->trx);
        for(auto actions : tm->trx.actions){
            m_actions_table->add(actions,tm->trx.id(), tm->trx.expiration, m_action_filter_on);
        }

    }

    void database::consume_transaction_trace( const chain::transaction_trace_ptr& tt) {
        m_traces_table->add(tt);
    }

    const std::string database::block_states_col = "block_states";
    const std::string database::blocks_col = "blocks";
    const std::string database::trans_col = "transactions";
    const std::string database::trans_traces_col = "transaction_traces";
    const std::string database::actions_col = "actions";
    const std::string database::accounts_col = "accounts";


} // namespace
