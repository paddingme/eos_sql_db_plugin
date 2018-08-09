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
    //    string str ="{\"id\":\"0002e445f404a8000f46f99180aa603f739ffee1f048198c858dd9d6e0e11d80\",\"block_num\":189509,\"header\":{\"timestamp\":\"2018-06-11T13:17:23.500\",\"producer\":\"genesisblock\",\"confirmed\":0,\"previous\":\"0002e44461e972fa55d1379c454cd3bb611245051662bda1dea4f4cd5a280a8c\",\"transaction_mroot\":\"39720cec2e14c75a72d05eb408ba2f58c9e37f73b8a85ee96dfc7b215f9968e4\",\"action_mroot\":\"5f3432fcd82ac56e12f4ae8bc113dada3364fe1da9cf08f4fce784d189d65a8b\",\"schedule_version\":1,\"header_extensions\":[],\"producer_signature\":\"SIG_K1_KkxNojxCKFUWDzvPhypNVXTtAYGThJogLLRGPgyVcVG3nh8ZkpbSpLxo9wb5yKsg7ZZLwcM2jtz74ysQaiP4mj7fhBHHh2\"},\"dpos_proposed_irreversible_blocknum\":189509,\"dpos_irreversible_blocknum\":189508,\"bft_irreversible_blocknum\":0,\"pending_schedule_lib_num\":12149,\"pending_schedule_hash\":\"c43882d5411af19d8596d5d835b3f4bd6a7fd36cc4c7fb55942ef11f8d1473b6\",\"pending_schedule\":{\"version\":1,\"producers\":[]},\"active_schedule\":{\"version\":1,\"producers\":[{\"producer_name\":\"genesisblock\",\"block_signing_key\":\"EOS8Yid3mE5bwWMvGGKYEDxFRGHostu5xCzFanyJP1UdgZ5mpPdwZ\"}]},\"blockroot_merkle\":{\"_active_nodes\":[\"2aa864d054eb9820db3e7d5eddb78022f385a963ba83be29f064e001f9c1339c\",\"e11c0abb64f0932b7b723c31764d308f32b7ddd090c61dfed141151bdc2d77df\",\"500096b184c6539387233b549bf46f045a810bf331e301bb4669d47700a68561\",\"f20243e9c77f41b0336f7d85f177446ec2b55c50b8b0fe7a0935194481e015af\",\"4b3d3bc2cf56b95fed65682df162175f3215f1bba9791b3d0fea5bfbaabb8281\",\"d4cc4e3c4635a50dcb7f9e091da8555deaba4ea2707cbebec5246cb2939df967\",\"827270b90af501d41051054f541ec063dfbab4e4a2eb4d6f85ffac575df777f0\",\"f094a95bd103d8e247452fc50b76d83f56cf4fac9d003a3f3580a8eec369705b\"],\"_node_count\":189508},\"producer_to_last_produced\":[[\"eosio\",12150],[\"genesisblock\",189509]],\"producer_to_last_implied_irb\":[[\"genesisblock\",189508]],\"block_signing_key\":\"EOS8Yid3mE5bwWMvGGKYEDxFRGHostu5xCzFanyJP1UdgZ5mpPdwZ\",\"confirm_count\":[],\"confirmations\":[],\"block\":{\"timestamp\":\"2018-06-11T13:17:23.500\",\"producer\":\"genesisblock\",\"confirmed\":0,\"previous\":\"0002e44461e972fa55d1379c454cd3bb611245051662bda1dea4f4cd5a280a8c\",\"transaction_mroot\":\"39720cec2e14c75a72d05eb408ba2f58c9e37f73b8a85ee96dfc7b215f9968e4\",\"action_mroot\":\"5f3432fcd82ac56e12f4ae8bc113dada3364fe1da9cf08f4fce784d189d65a8b\",\"schedule_version\":1,\"header_extensions\":[],\"producer_signature\":\"SIG_K1_KkxNojxCKFUWDzvPhypNVXTtAYGThJogLLRGPgyVcVG3nh8ZkpbSpLxo9wb5yKsg7ZZLwcM2jtz74ysQaiP4mj7fhBHHh2\",\"transactions\":[{\"status\":\"executed\",\"cpu_usage_us\":566,\"net_usage_words\":15,\"trx\":[1,{\"signatures\":[\"SIG_K1_K273pLxLGCwtY5ioeZhb1ym4CYg6ukYTnV6YgquftjxvjRYXYcNQThxQ4fGcEyQTkjpiRzCjYMhffZq6pphEyFrvd5GAB9\"],\"compression\":\"none\",\"packed_context_free_data\":\"00\",\"packed_trx\":\"81761e5b42e42284cb3e00000000010000000000ea30550000000000a032dd01a01861fa4c93896200000000a8ed323219a01861fa4c938962000000000000000001e0b3dbe632ec305500\"}]}],\"block_extensions\":[]},\"validated\":false,\"in_current_chain\":false}";
    //    auto bs = fc::json::from_string(str).as<chain::block_state>();
    //     for(auto& receipt : bs.block->transactions) {
    //         string trx_id_str;
    //         if( receipt.trx.contains<chain::packed_transaction>() ){
    //             const auto& tm = chain::transaction_metadata(receipt.trx.get<chain::packed_transaction>());

    //             trx_id_str = tm.trx.id().str();   
    //             for(auto atc : tm.trx.actions){
    //                 db->m_actions_table->add(atc, trx_id_str,bs.block->timestamp , action_filter_on);
    //             }
    //         }

    //     }
    //     auto traces = fc::json::from_string("{\"id\":\"37f6a03c467e3e53c80075fe4b6db51eec28b68c2ddb6fde579f6457e2ed695e\",\"receipt\":{\"status\":\"executed\",\"cpu_usage_us\":566,\"net_usage_words\":15},\"elapsed\":348,\"net_usage\":120,\"scheduled\":false,\"action_traces\":[{\"receipt\":{\"receiver\":\"eosio\",\"act_digest\":\"ab084b17c4118773578eb25349c404e9eb697c4594f9a6266a0b85ca856358f8\",\"global_sequence\":2735883,\"recv_sequence\":1362926,\"auth_sequence\":[[\"ge4tanbug4ge\",2]],\"code_sequence\":4,\"abi_sequence\":5},\"act\":{\"account\":\"eosio\",\"name\":\"vote\",\"authorization\":[{\"actor\":\"ge4tanbug4ge\",\"permission\":\"active\"}],\"data\":\"a01861fa4c938962000000000000000001e0b3dbe632ec3055\"},\"elapsed\":307,\"cpu_usage\":0,\"console\":\"\",\"total_cpu_usage\":0,\"trx_id\":\"37f6a03c467e3e53c80075fe4b6db51eec28b68c2ddb6fde579f6457e2ed695e\",\"inline_traces\":[]}],\"failed_dtrx_trace\":null}").as<chain::transaction_trace>();
    //     db->m_traces_table->parse_traces(traces);
           

        my->handler = std::make_unique<consumer>(std::move(db),queue_size);
        chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
        FC_ASSERT(chain_plug);
        auto& chain = chain_plug->chain();

        my->applied_transaction_connection.emplace(chain.applied_transaction.connect([this](const chain::transaction_trace_ptr& tt){
            my->applied_transaction(tt);
        } ));

        // my->accepted_transaction_connection.emplace(chain.accepted_transaction.connect([this](const chain::transaction_metadata_ptr& tm){
            // my->accepted_transaction(tm);
        // } ));

        // my->accepted_block_connection.emplace(chain.accepted_block.connect([this]( const chain::block_state_ptr& bs){
        //     my->accepted_block(bs);
        // } ));   

        my->irreversible_block_connection.emplace(chain.irreversible_block.connect([this]( const chain::block_state_ptr& bs){
            my->applied_irreversible_block(bs);
        } ));     
        
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
