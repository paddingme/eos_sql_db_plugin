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

namespace {
const char* BLOCK_START_OPTION = "sql_db-block-start";
const char* BUFFER_SIZE_OPTION = "sql_db-queue-size";
const char* SQL_DB_URI_OPTION = "sql_db-uri";
const char* REBUILD_DATABASE = "rebuild-database";
}

namespace fc { class variant; }

namespace eosio {

    static appbase::abstract_plugin& _sql_db_plugin = app().register_plugin<sql_db_plugin>();

    class sql_db_plugin_impl  {
        public:
            sql_db_plugin_impl(){};
            ~sql_db_plugin_impl(){};

            std::unique_ptr<consumer> handler;

            fc::optional<boost::signals2::scoped_connection> accepted_block_connection;
            fc::optional<boost::signals2::scoped_connection> irreversible_block_connection;
            fc::optional<boost::signals2::scoped_connection> accepted_transaction_connection;
            fc::optional<boost::signals2::scoped_connection> applied_transaction_connection;

            void accepted_block( const chain::block_state_ptr& );
            void applied_irreversible_block( const chain::block_state_ptr& );
            void accepted_transaction( const chain::transaction_metadata_ptr& );
            void applied_transaction( const chain::transaction_trace_ptr& );

    };

    void sql_db_plugin_impl::accepted_block( const chain::block_state_ptr& bs ) {
        // if(bs->trxs.size()!=0){
        //     for(auto& trx : bs->trxs){
        //         ilog("${result}",("result",trx->trx));
        //     }
        // }
        handler->push_block_state(bs);
    }

    void sql_db_plugin_impl::applied_irreversible_block( const chain::block_state_ptr& bs) {
        // for(auto& transaction : bs->block->transactions){
        //     ilog("${result}",("result",fc::json::to_string(fc::raw::unpack<chain::transaction>(transaction.trx.get<chain::packed_transaction>().get_raw_transaction()))));
        // }
        handler->push_irreversible_block_state(bs);
    }

    void sql_db_plugin_impl::accepted_transaction( const chain::transaction_metadata_ptr& tm ) {
        handler->push_transaction_metadata(tm);
    }

    void sql_db_plugin_impl::applied_transaction( const chain::transaction_trace_ptr& tt ) {
        
        // if(!(tt->action_traces.size()==1&&tt->action_traces[0].act.name.to_string()=="onblock")){
        //     ilog("${result}",("result",tt));
        // }

        handler->push_transaction_trace(tt);
    }

    sql_db_plugin::sql_db_plugin():my(new sql_db_plugin_impl ){}

    sql_db_plugin::~sql_db_plugin(){}

    void sql_db_plugin::set_program_options(options_description& cli, options_description& cfg) {
        dlog("set_program_options");

        cfg.add_options()
                (BUFFER_SIZE_OPTION, bpo::value<uint>()->default_value(256),
                "The queue size between nodeos and SQL DB plugin thread.")
                (BLOCK_START_OPTION, bpo::value<uint32_t>()->default_value(0),
                "The block to start sync.")
                (SQL_DB_URI_OPTION, bpo::value<std::string>(),
                "Sql DB URI connection string"
                " If not specified then plugin is disabled. Default database 'EOS' is used if not specified in URI.")
                (REBUILD_DATABASE,bpo::bool_switch()->default_value(false),"")
                ;
    }

    void sql_db_plugin::plugin_initialize(const variables_map& options) {
        ilog("initialize");

        std::string uri_str = options.at(SQL_DB_URI_OPTION).as<std::string>();
        if (uri_str.empty()){
            wlog("db URI not specified => eosio::sql_db_plugin disabled.");
            return;
        }

        ilog("connecting to ${u}", ("u", uri_str));
        uint32_t block_num_start = options.at(BLOCK_START_OPTION).as<uint32_t>();
        auto queue_size = options.at(BUFFER_SIZE_OPTION).as<uint32_t>();

        auto db = std::make_unique<database>(uri_str, block_num_start);

        if (block_num_start == 0) {
            ilog("Resync requested: wiping database");
            db->wipe();
        }
        if (!db->is_started()) {
            if (block_num_start == 0) {
                ilog("Resync requested: wiping database");
                db->wipe();
            }
        }

        my->handler = std::make_unique<consumer>(std::move(db),queue_size);
        chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
        FC_ASSERT(chain_plug);
        auto& chain = chain_plug->chain();

        my->accepted_block_connection.emplace(chain.accepted_block.connect([this]( const chain::block_state_ptr& bs){
            my->accepted_block(bs);
        } ));

        my->irreversible_block_connection.emplace(chain.irreversible_block.connect([this]( const chain::block_state_ptr& bs){
            my->applied_irreversible_block(bs);
        } ));

        my->accepted_transaction_connection.emplace(chain.accepted_transaction.connect([this](const chain::transaction_metadata_ptr& tm){
            // my->accepted_transaction(tm);
        } ));

        my->applied_transaction_connection.emplace(chain.applied_transaction.connect([this](const chain::transaction_trace_ptr& tt){
            my->applied_transaction(tt);
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
