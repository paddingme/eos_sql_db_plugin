// #include "database.hpp"
#include <eosio/sql_db_plugin/database.hpp>

namespace eosio
{

    database::database(const std::string &uri, uint32_t block_num_start, size_t pool_size) {
        // m_session = std::make_shared<soci::session>(uri); 
        m_session_pool = std::make_shared<soci_session_pool>(pool_size,uri);
        m_accounts_table = std::make_unique<accounts_table>(m_session_pool);
        m_blocks_table = std::make_unique<blocks_table>(m_session_pool);
        m_traces_table = std::make_unique<traces_table>(m_session_pool);
        m_transactions_table = std::make_unique<transactions_table>(m_session_pool);
        m_actions_table = std::make_unique<actions_table>(m_session_pool);
        m_block_num_start = block_num_start;
        system_account = chain::name(chain::config::system_account_name).to_string();
    }

    database::database(const std::string &uri, uint32_t block_num_start, size_t pool_size, std::vector<string> filter_on, std::vector<string> filter_out) {
        new (this)database(uri,block_num_start,pool_size);
        m_action_filter_on = filter_on;
        m_contract_filter_out = filter_out;
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
        m_blocks_table->add(bs);
    }

    void database::consume_irreversible_block_state( const chain::block_state_ptr& bs ,boost::mutex::scoped_lock& lock_db, boost::condition_variable& condition, boost::atomic<bool>& exit){
        // ilog("run consume irreversible block");
        auto block_id = bs->id.str();

        // sleep(1);
        // do{
        //     bool update_irreversible = m_blocks_table->irreversible_set(block_id, true);
        //     if(update_irreversible || exit) break;
        //     else condition.timed_wait(lock_db, boost::posix_time::milliseconds(10));
        // }while(!exit);

        for(auto& receipt : bs->block->transactions) {
            string trx_id_str;
            if( receipt.trx.contains<chain::packed_transaction>() ){
                const auto& tm = chain::transaction_metadata(receipt.trx.get<chain::packed_transaction>());

                //filter out system timer
                if(tm.trx.actions.size()==1 && tm.trx.actions[0].name.to_string() == "onblock" ) continue ;

                //filter out attack contract
                bool attack_check = true;
                for(auto& act : tm.trx.actions ){
                    if( std::find(m_contract_filter_out.begin(),m_contract_filter_out.end(),act.account.to_string()) == m_contract_filter_out.end() ){
                        attack_check = false;
                    }
                }
                if(attack_check) return;

                trx_id_str = tm.trx.id().str();   

                // string trace_result;
                // do{
                //     trace_result = m_traces_table->list(trx_id_str, bs->block->timestamp);
                //     if( !trace_result.empty() ){
                //         auto traces = fc::json::from_string(trace_result).as<chain::transaction_trace>();
                //         for(auto atc : traces.action_traces){
                //             if( atc.receipt.receiver == atc.act.account ){
                //                 const auto timestamp = std::chrono::seconds{bs->block->timestamp.operator fc::time_point().sec_since_epoch()}.count();
                //                 m_actions_table->add(atc.act, trx_id_str, timestamp, m_action_filter_on);
                //             }
                //         }
                //         // ilog("${result}",("result",traces));
                //         m_traces_table->parse_traces(traces);
                //         break;
                //     } else if(exit) {
                //         break;
                //     }else condition.timed_wait(lock_db, boost::posix_time::milliseconds(10));     

                // }while((!exit));

            } else {
                trx_id_str = receipt.trx.get<chain::transaction_id_type>().str();
            }       
            m_traces_table->add_scheduled_transaction(trx_id_str,bs->block->timestamp);
        }

    }

    void database::consume_transaction_metadata( const chain::transaction_metadata_ptr& tm ) {
        m_transactions_table->add(tm->trx);
        // for(auto actions : tm->trx.actions){
        //     m_actions_table->add(actions,tm->trx.id(), tm->trx.expiration, m_action_filter_on);
        // }
    }

    void database::consume_transaction_trace( const chain::transaction_trace_ptr& tt, boost::mutex::scoped_lock& lock_db, boost::condition_variable& condition, boost::atomic<bool>& exit ) {
        do{
            string tx_id;
            int64_t timestamp;

            auto trx_id_str = tt->id.str();
            m_traces_table->get_scheduled_transaction(trx_id_str,tx_id,timestamp);
            if( !tx_id.empty() ){
                
                if(!tt->except) {
                    for(auto atc : tt->action_traces){
                        if( atc.receipt.receiver == atc.act.account ){
                            m_actions_table->add(atc.act, trx_id_str, timestamp, m_action_filter_on);
                        }
                    }
                    m_traces_table->parse_traces(*tt,timestamp);
                }//else ilog("wrong transaction");
                break;
            } else if(exit) {
                break;
            }else condition.timed_wait(lock_db, boost::posix_time::milliseconds(10));     

        }while((!exit));
    }

    const std::string database::block_states_col = "block_states";
    const std::string database::blocks_col = "blocks";
    const std::string database::trans_col = "transactions";
    const std::string database::trans_traces_col = "transaction_traces";
    const std::string database::actions_col = "actions";
    const std::string database::accounts_col = "accounts";

} // namespace
