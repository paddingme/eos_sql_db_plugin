/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @author Alessandro Siniscalchi <asiniscalchi@gmail.com>
 */
#include <eosio/sql_db_plugin/sql_db_plugin.hpp>
#include <eosio/sql_db_plugin/database.hpp>
// #include "database.hpp"
#include "consumer.hpp"

#include <fc/io/json.hpp>
#include <fc/utf8.hpp>
#include <fc/variant.hpp>

#include <boost/algorithm/string.hpp>

namespace {
const char* BLOCK_START_OPTION = "sql_db-block-start";
const char* BUFFER_SIZE_OPTION = "sql_db-queue-size";
const char* SQL_DB_URI_OPTION = "sql_db-uri";
const char* SQL_DB_ACTION_FILTER_ON = "sql_db-action-filter-on";
const char* SQL_DB_CONTRACT_FILTER_OUT = "sql_db-contract-filter-out";
}

namespace fc { class variant; }

namespace eosio {

    static appbase::abstract_plugin& _sql_db_plugin = app().register_plugin<sql_db_plugin>();

    class sql_db_plugin_impl  {
        public:
            sql_db_plugin_impl(){};
            ~sql_db_plugin_impl(){};

            std::unique_ptr<consumer> handler;
            std::vector<std::string> contract_filter_out;

            fc::optional<boost::signals2::scoped_connection> accepted_block_connection;
            fc::optional<boost::signals2::scoped_connection> irreversible_block_connection;
            fc::optional<boost::signals2::scoped_connection> accepted_transaction_connection;
            fc::optional<boost::signals2::scoped_connection> applied_transaction_connection;

            void accepted_block( const chain::block_state_ptr& );
            void applied_irreversible_block( const chain::block_state_ptr& );
            void accepted_transaction( const chain::transaction_metadata_ptr& );
            void applied_transaction( const chain::transaction_trace_ptr& );

            bool filter_out_contract( std::string contract) {
                if( std::find(contract_filter_out.begin(),contract_filter_out.end(),contract) != contract_filter_out.end() ){
                    return true;
                }
                return false;
            }

    };

    void sql_db_plugin_impl::accepted_block( const chain::block_state_ptr& bs ) {
        handler->push_block_state(bs);
    }

    void sql_db_plugin_impl::applied_irreversible_block( const chain::block_state_ptr& bs) {
        handler->push_irreversible_block_state(bs);
    }

    void sql_db_plugin_impl::accepted_transaction( const chain::transaction_metadata_ptr& tm ) {
        handler->push_transaction_metadata(tm);
    }

    void sql_db_plugin_impl::applied_transaction( const chain::transaction_trace_ptr& tt ) {
        if(!tt->except){
            //filter out system timer action
            if(tt->action_traces.size()==1&&tt->action_traces[0].act.name.to_string()=="onblock"){
                return ;
            }

            //filter out attack contract
            bool attack_check = true;
            for(auto& action_trace : tt->action_traces ){
                if(!filter_out_contract(action_trace.act.account.to_string()) ){
                    attack_check = false;
                }
            }
            if(attack_check) return; 
        }  

        handler->push_transaction_trace(tt);
    }

    sql_db_plugin::sql_db_plugin():my(new sql_db_plugin_impl ){}

    sql_db_plugin::~sql_db_plugin(){}

    void sql_db_plugin::set_program_options(options_description& cli, options_description& cfg) {
        dlog("set_program_options");

        cfg.add_options()
                (BUFFER_SIZE_OPTION, bpo::value<uint>()->default_value(5000),
                "The queue size between nodeos and SQL DB plugin thread.")
                (BLOCK_START_OPTION, bpo::value<uint32_t>()->default_value(0),
                "The block to start sync.")
                (SQL_DB_URI_OPTION, bpo::value<std::string>(),
                "Sql DB URI connection string"
                " If not specified then plugin is disabled. Default database 'EOS' is used if not specified in URI.")
                (SQL_DB_ACTION_FILTER_ON,bpo::value<std::string>(),
                "saved action with filter on")
                (SQL_DB_CONTRACT_FILTER_OUT,bpo::value<std::string>(),
                "saved action without filter out")
                ;
    }

    void sql_db_plugin::plugin_initialize(const variables_map& options) {
        ilog("initialize");

        std::vector<std::string> action_filter_on;
        if( options.count( SQL_DB_ACTION_FILTER_ON ) ){
            auto fo = options.at(SQL_DB_ACTION_FILTER_ON).as<std::string>();
            boost::replace_all(fo," ","");
            boost::split(action_filter_on, fo,  boost::is_any_of( "," ));
            ilog("${string} ${size}",("string",fo)("size",action_filter_on.size()));
        }

        if( options.count( SQL_DB_CONTRACT_FILTER_OUT ) ){
            auto fo = options.at(SQL_DB_CONTRACT_FILTER_OUT).as<std::string>();
            boost::replace_all(fo," ","");
            boost::split(my->contract_filter_out, fo,  boost::is_any_of( "," ));
            ilog("${string} ${size}",("string",fo)("size",my->contract_filter_out.size()));
        }

        std::string uri_str = options.at(SQL_DB_URI_OPTION).as<std::string>();
        if (uri_str.empty()){
            wlog("db URI not specified => eosio::sql_db_plugin disabled.");
            return;
        }

        ilog("connecting to ${u}", ("u", uri_str));
        uint32_t block_num_start = options.at(BLOCK_START_OPTION).as<uint32_t>();
        auto queue_size = options.at(BUFFER_SIZE_OPTION).as<uint32_t>();

        ilog("queue size ${size}",("size",queue_size));

        //for three thread。 TODO: change to thread db pool
        auto db = std::make_unique<database>(uri_str, block_num_start, 5, action_filter_on, my->contract_filter_out);

        if (!db->is_started()) {
            if (block_num_start == 0) {
                ilog("Resync requested: wiping database");
                db->wipe();
            }
        }
        //test demo
    //    string str ="{\"id\":\"0003ba1262ddc6e8c069a1fb7c0a68ec41eb9c2c28abe6eb47f4f671acf74bd5\",\"block_num\":244242,\"header\":{\"timestamp\":\"2018-06-11T20:53:30.000\",\"producer\":\"genesisblock\",\"confirmed\":0,\"previous\":\"0003ba11e0c8e58ba964ccb97d680591499dd09c59992f37107b63f33995112b\",\"transaction_mroot\":\"17aef3d165474c062b4eff3c5847c6b5c91b829c3ca3712f9ebef2f9bbd59ad4\",\"action_mroot\":\"b5a168162fd47baeb47184d27d72f76820be356ba92d11419951b77b5e4d7ce9\",\"schedule_version\":1,\"header_extensions\":[],\"producer_signature\":\"SIG_K1_K3VRaUmuDM1t7dUa6g86Z7NuyAcrVRAhDkx2xRV5eRpGcTAMMHvdR7b5Hn62ejQ22SKrt3XT9rHTKEz6SBy2yxjveTaooB\"},\"dpos_proposed_irreversible_blocknum\":244242,\"dpos_irreversible_blocknum\":244241,\"bft_irreversible_blocknum\":0,\"pending_schedule_lib_num\":12149,\"pending_schedule_hash\":\"c43882d5411af19d8596d5d835b3f4bd6a7fd36cc4c7fb55942ef11f8d1473b6\",\"pending_schedule\":{\"version\":1,\"producers\":[]},\"active_schedule\":{\"version\":1,\"producers\":[{\"producer_name\":\"genesisblock\",\"block_signing_key\":\"EOS8Yid3mE5bwWMvGGKYEDxFRGHostu5xCzFanyJP1UdgZ5mpPdwZ\"}]},\"blockroot_merkle\":{\"_active_nodes\":[\"0003ba11e0c8e58ba964ccb97d680591499dd09c59992f37107b63f33995112b\",\"8f7b1f835d0f37e9f887150ebb4a2148e5d27dcfe5f9e5542bacba2ce0998ce8\",\"4535480bf05d7f3fed3f1994b4780680d42b328f81b2454f689ddabe531555b0\",\"2b1fbdec5d7b61706024ff3fb2181ebdfb2b2ba30bf59a4ab84279aef701ad03\",\"92e3fa53c75325fac2d829aa4e3756dd41220c1788d9c5a75b7365056ce74435\",\"94bafbe9812b967f55caea3318eaee06baad9020088f089ca57299e163eb4e2e\",\"946e0d935c6b452f2530997b42ec1989c5f2e0b7f9f0be7f1cdda23e9ebf2c5b\",\"3a1ebfd3d001cd66c9f9e17a32476a99d02a75cf82b7167881f7d4d0890e4614\",\"827270b90af501d41051054f541ec063dfbab4e4a2eb4d6f85ffac575df777f0\",\"10bb567e0218334f9815a403305267557ccc7f1f2c7ed35e4bbfcefa2fff3aeb\"],\"_node_count\":244241},\"producer_to_last_produced\":[[\"eosio\",12150],[\"genesisblock\",244242]],\"producer_to_last_implied_irb\":[[\"genesisblock\",244241]],\"block_signing_key\":\"EOS8Yid3mE5bwWMvGGKYEDxFRGHostu5xCzFanyJP1UdgZ5mpPdwZ\",\"confirm_count\":[],\"confirmations\":[],\"block\":{\"timestamp\":\"2018-06-11T20:53:30.000\",\"producer\":\"genesisblock\",\"confirmed\":0,\"previous\":\"0003ba11e0c8e58ba964ccb97d680591499dd09c59992f37107b63f33995112b\",\"transaction_mroot\":\"17aef3d165474c062b4eff3c5847c6b5c91b829c3ca3712f9ebef2f9bbd59ad4\",\"action_mroot\":\"b5a168162fd47baeb47184d27d72f76820be356ba92d11419951b77b5e4d7ce9\",\"schedule_version\":1,\"header_extensions\":[],\"producer_signature\":\"SIG_K1_K3VRaUmuDM1t7dUa6g86Z7NuyAcrVRAhDkx2xRV5eRpGcTAMMHvdR7b5Hn62ejQ22SKrt3XT9rHTKEz6SBy2yxjveTaooB\",\"transactions\":[{\"status\":\"executed\",\"cpu_usage_us\":3581,\"net_usage_words\":19,\"trx\":[1,{\"signatures\":[\"SIG_K1_K7THPjtD5WYdq2WEoEmg1w2m8aqT8TxcYeSTBQYWzJACpxafzWBNTJ2AbF6mMFeGomrduAL9TDfaJWVPECqki1mcwHWFs6\"],\"compression\":\"none\",\"packed_context_free_data\":\"\",\"packed_trx\":\"7de11e5b00bab3d8bc2700000000010000000000ea30557015d289deaa32dd01a09867fb5099846600000000a8ed323239a09867fb50998466000000000000000005204dba2a63693055e0b3bbb4656d3055202932c94c83305580a94a4e5b17315540196594a988cca500\"}]}],\"block_extensions\":[]},\"validated\":true,\"in_current_chain\":true}";
    //    auto bs = fc::json::from_string(str).as<chain::block_state>();ilog("???");
    //     for(auto& receipt : bs.block->transactions) {
    //         ilog("???");
    //         string trx_id_str;
    //         if( receipt.trx.contains<chain::packed_transaction>() ){
    //             const auto& tm = chain::transaction_metadata(receipt.trx.get<chain::packed_transaction>());
    //             ilog("???");
    //             trx_id_str = tm.trx.id().str();   
    //             for(auto atc : tm.trx.actions){
    //                 db->m_actions_table->add(atc, trx_id_str,std::chrono::seconds{bs.block->timestamp.operator fc::time_point().sec_since_epoch()}.count() , action_filter_on);
    //             }
    //         }else trx_id_str = receipt.trx.get<chain::transaction_id_type>().str();
    //         ilog("${trx_id_str}",("trx_id_str",trx_id_str));
    //     }
        // auto traces = fc::json::from_string("{\"id\":\"7b69b1e053d4edc6b3b7632283f1eca5e160845314b2ae6f298f47c196a14864\",\"receipt\":{\"status\":\"executed\",\"cpu_usage_us\":1597,\"net_usage_words\":0},\"elapsed\":1794,\"net_usage\":0,\"scheduled\":true,\"action_traces\":[{\"receipt\":{\"receiver\":\"eosio\",\"act_digest\":\"a5358e257888c404f47f3ed18448d266213dbea5ceca00a1984648d6d3795cb2\",\"global_sequence\":4180016,\"recv_sequence\":2472726,\"auth_sequence\":[[\"gm4tgmrxgyge\",30]],\"code_sequence\":4,\"abi_sequence\":5},\"act\":{\"account\":\"eosio\",\"name\":\"refund\",\"authorization\":[{\"actor\":\"gm4tgmrxgyge\",\"permission\":\"active\"}],\"data\":\"a09867fd4a968964\"},\"elapsed\":1191,\"cpu_usage\":0,\"console\":\"\",\"total_cpu_usage\":0,\"trx_id\":\"7b69b1e053d4edc6b3b7632283f1eca5e160845314b2ae6f298f47c196a14864\",\"inline_traces\":[{\"receipt\":{\"receiver\":\"eosio.token\",\"act_digest\":\"6b69f9e0c78a06fafeda46df1a27472d0ff2dabd738928d4fc63152fa6821622\",\"global_sequence\":4180017,\"recv_sequence\":727836,\"auth_sequence\":[[\"eosio.stake\",3]],\"code_sequence\":1,\"abi_sequence\":1},\"act\":{\"account\":\"eosio.token\",\"name\":\"transfer\",\"authorization\":[{\"actor\":\"eosio.stake\",\"permission\":\"active\"}],\"data\":\"0014341903ea3055a09867fd4a968964605af4050000000004454f530000000007756e7374616b65\"},\"elapsed\":544,\"cpu_usage\":0,\"console\":\"\",\"total_cpu_usage\":0,\"trx_id\":\"7b69b1e053d4edc6b3b7632283f1eca5e160845314b2ae6f298f47c196a14864\",\"inline_traces\":[{\"receipt\":{\"receiver\":\"eosio.stake\",\"act_digest\":\"6b69f9e0c78a06fafeda46df1a27472d0ff2dabd738928d4fc63152fa6821622\",\"global_sequence\":4180018,\"recv_sequence\":177276,\"auth_sequence\":[[\"eosio.stake\",4]],\"code_sequence\":1,\"abi_sequence\":1},\"act\":{\"account\":\"eosio.token\",\"name\":\"transfer\",\"authorization\":[{\"actor\":\"eosio.stake\",\"permission\":\"active\"}],\"data\":\"0014341903ea3055a09867fd4a968964605af4050000000004454f530000000007756e7374616b65\"},\"elapsed\":5,\"cpu_usage\":0,\"console\":\"\",\"total_cpu_usage\":0,\"trx_id\":\"7b69b1e053d4edc6b3b7632283f1eca5e160845314b2ae6f298f47c196a14864\",\"inline_traces\":[]},{\"receipt\":{\"receiver\":\"gm4tgmrxgyge\",\"act_digest\":\"6b69f9e0c78a06fafeda46df1a27472d0ff2dabd738928d4fc63152fa6821622\",\"global_sequence\":4180019,\"recv_sequence\":10,\"auth_sequence\":[[\"eosio.stake\",5]],\"code_sequence\":1,\"abi_sequence\":1},\"act\":{\"account\":\"eosio.token\",\"name\":\"transfer\",\"authorization\":[{\"actor\":\"eosio.stake\",\"permission\":\"active\"}],\"data\":\"0014341903ea3055a09867fd4a968964605af4050000000004454f530000000007756e7374616b65\"},\"elapsed\":3,\"cpu_usage\":0,\"console\":\"\",\"total_cpu_usage\":0,\"trx_id\":\"7b69b1e053d4edc6b3b7632283f1eca5e160845314b2ae6f298f47c196a14864\",\"inline_traces\":[]}]}]}],\"failed_dtrx_trace\":null}\"hostname\":\"\",\"thread_name\":\"thread-0\",\"timestamp\":\"2018-08-06T10:19:05.128\"},\"format\":\"assertion failure with message: ${s}\",\"data\":{\"s\":\"refund is not available yet\"}},{\"context\":{\"level\":\"warn\",\"file\":\"apply_context.cpp\",\"line\":61,\"method\":\"exec_one\",\"hostname\":\"\",\"thread_name\":\"thread-0\",\"timestamp\":\"2018-08-06T10:19:05.129\"},\"format\":\"pending console output: ${console}\",\"data\":{\"console\":\"\"}}]}}\"code_sequence\":1,\"abi_sequence\":1},\"act\":{\"account\":\"eosio.token\",\"name\":\"transfer\",\"authorization\":[{\"actor\":\"eosio.stake\",\"permission\":\"active\"}],\"data\":\"0014341903ea3055a09867fd4a968964605af4050000000004454f530000000007756e7374616b65\"},\"elapsed\":5,\"cpu_usage\":0,\"console\":\"\",\"total_cpu_usage\":0,\"trx_id\":\"7b69b1e053d4edc6b3b7632283f1eca5e160845314b2ae6f298f47c196a14864\",\"inline_traces\":[]},{\"receipt\":{\"receiver\":\"gm4tgmrxgyge\",\"act_digest\":\"6b69f9e0c78a06fafeda46df1a27472d0ff2dabd738928d4fc63152fa6821622\",\"global_sequence\":4180019,\"recv_sequence\":10,\"auth_sequence\":[[\"eosio.stake\",5]],\"code_sequence\":1,\"abi_sequence\":1},\"act\":{\"account\":\"eosio.token\",\"name\":\"transfer\",\"authorization\":[{\"actor\":\"eosio.stake\",\"permission\":\"active\"}],\"data\":\"0014341903ea3055a09867fd4a968964605af4050000000004454f530000000007756e7374616b65\"},\"elapsed\":3,\"cpu_usage\":0,\"console\":\"\",\"total_cpu_usage\":0,\"trx_id\":\"7b69b1e053d4edc6b3b7632283f1eca5e160845314b2ae6f298f47c196a14864\",\"inline_traces\":[]}]}]}],\"failed_dtrx_trace\":null}").as<chain::transaction_trace>();
        // // db->m_traces_table->parse_traces(traces);
        // ilog("what");

        // string tx_id;
        // int64_t timestamp;

        // auto trx_id_str = traces.id.str();
        // db->m_traces_table->get_scheduled_transaction(trx_id_str,tx_id,timestamp);
        // ilog("${tx_id}:${timestamp}",("tx_id",tx_id)("timestamp",timestamp));
        // if( !tx_id.empty() ){
            
        //     if(!traces.except) {
        //         for(auto atc : traces.action_traces){
        //             if( atc.receipt.receiver == atc.act.account ){
        //                 db->m_actions_table->add(atc.act, trx_id_str, timestamp, action_filter_on);
        //             }
        //         }
        //         db->m_traces_table->parse_traces(traces);
        //     }else ilog("wrong transaction");
        // } 

        //test end
       
        my->handler = std::make_unique<consumer>(std::move(db),queue_size);
        chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
        FC_ASSERT(chain_plug);
        auto& chain = chain_plug->chain();

        my->accepted_block_connection.emplace(chain.accepted_block.connect([this]( const chain::block_state_ptr& bs){
            // my->accepted_block(bs);
        } ));   

        my->irreversible_block_connection.emplace(chain.irreversible_block.connect([this]( const chain::block_state_ptr& bs){
            // my->applied_irreversible_block(bs);
        } ));  

        my->applied_transaction_connection.emplace(chain.applied_transaction.connect([this](const chain::transaction_trace_ptr& tt){
            // if(!(tt->action_traces.size()==1&&tt->action_traces[0].act.name.to_string()=="onblock"))
                // ilog("${result}",("result",fc::json::to_string(tt)));
            // my->applied_transaction(tt);
        } ));

        // my->accepted_transaction_connection.emplace(chain.accepted_transaction.connect([this](const chain::transaction_metadata_ptr& tm){
            // my->accepted_transaction(tm);
        // } ));
        
    }

    void sql_db_plugin::plugin_startup() {
        ilog("startup");
    }

    void sql_db_plugin::plugin_shutdown() {
        ilog("shutdown");
        my->handler->shutdown();
        my->accepted_block_connection.reset();
        my->irreversible_block_connection.reset();
        my->accepted_transaction_connection.reset();
        my->applied_transaction_connection.reset();
    }

} // namespace eosio
