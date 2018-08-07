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
        if(bs->trxs.size()!=0){
            ilog("trx.size ${size}",("size",bs->trxs.size()));
            // for(auto& trx : bs->trxs){
            //     ilog("${result}",("result",trx->trx));
            // }
        }
        handler->push_block_state(bs);
    }

    void sql_db_plugin_impl::applied_irreversible_block( const chain::block_state_ptr& bs) {
        // for(auto& transaction : bs->block->transactions){
        //     ilog("${result}",("result",fc::json::to_string(fc::raw::unpack<chain::transaction>(transaction.trx.get<chain::packed_transaction>().get_raw_transaction()))));
        // }
        // if(bs->trxs.size()!=0){
        //     ilog("trx.size ${size}",("size",bs->trxs.size()));
        // }
        handler->push_irreversible_block_state(bs);
    }

    // void sql_db_plugin_impl::applied_irreversible_block_for_traces( const chain::block_state_ptr& bs) {
    //     for(auto& receipt : bs->block->transactions){
    //         string trx_id_str;
    //         if( receipt.trx.contains<chain::packed_transaction>() ){
    //             const auto& trx = fc::raw::unpack<chain::transaction>( receipt.trx.get<chain::packed_transaction>().get_raw_transaction() );
                
    //             //filter out system timer action
    //             if(trx.actions.size()==1 && trx.actions[0].name.to_string() == "onblock" ) continue ;

    //             if( trx.actions[0].name.to_string()=="refund" ){
    //                 ilog("irr 222");
    //                 ilog(trx.id().str());
    //             }
    //             //filter out attack contract
    //             bool attack_check = true;
    //             for(auto& action : trx.actions ){
    //                 if(!filter_out_contract(action.account.to_string()) ){
    //                     attack_check = false;
    //                 }
    //             }
                
    //             if(attack_check) continue;
    //             if( trx.actions[0].name.to_string()=="refund" ){
    //                 ilog("irr 222");
    //                 ilog(trx.id().str());
    //             }
    //             trx_id_str = trx.id().str();
    //             tx_id_block_time traces_params{trx_id_str,bs->block->timestamp};
    //             handler->push_irreversible_block_for_traces_state( traces_params );
    //         }
    //     }

    // }

    void sql_db_plugin_impl::accepted_transaction( const chain::transaction_metadata_ptr& tm ) {
        handler->push_transaction_metadata(tm);
    }

    void sql_db_plugin_impl::applied_transaction( const chain::transaction_trace_ptr& tt ) {
        //filter out system timer action
        if(tt->action_traces.size()==1&&tt->action_traces[0].act.name.to_string()=="onblock"){
            return ;
        }

        // if( tt->action_traces[0].act.name.to_string()=="refund" ) ilog("看我看我啊啊啊啊啊啊");

        //filter out attack contract
        // bool attack_check = true;
        // for(auto& action_trace : tt->action_traces ){
        //     if(!filter_out_contract(action_trace.act.account.to_string()) ){
        //         attack_check = false;
        //     }
        // }
        // if(attack_check) return;

        if( tt->action_traces.size()==1 && filter_out_contract(tt->action_traces[0].act.account.to_string()) ){
            return ;
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
        auto db_blocks = std::make_unique<database>(uri_str, block_num_start);
        auto db_traces = std::make_unique<database>(uri_str, block_num_start);
        auto db_irreversible = std::make_unique<database>(uri_str, block_num_start, action_filter_on, my->contract_filter_out);

        if (!db_blocks->is_started()) {
            if (block_num_start == 0) {
                ilog("Resync requested: wiping database");
                db_blocks->wipe();
            }
        }

        my->handler = std::make_unique<consumer>(std::move(db_blocks),std::move(db_traces),std::move(db_irreversible),queue_size);
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
