// #include "actions_table.hpp"
#include <eosio/sql_db_plugin/actions_table.hpp>

namespace eosio {

    actions_table::actions_table(std::shared_ptr<soci::session> session):
        m_session(session) {

    }

    void actions_table::add(chain::action action, chain::transaction_id_type transaction_id, chain::block_timestamp_type block_time, std::vector<std::string> filter_out) {

        if(action.name.to_string() == "onblock") return ; //system contract abi haven't onblock, so we could get abi_data.

        chain::abi_def abi;
        std::string abi_def_account;
        chain::abi_serializer abis;
        soci::indicator ind;
        const auto transaction_id_str = transaction_id.str();
        const auto timestamp = std::chrono::seconds{block_time.operator fc::time_point().sec_since_epoch()}.count();

        string json = add_data(action);
        system_contract_arg dataJson = fc::json::from_string(json).as<system_contract_arg>();
        string json_auth = fc::json::to_string(action.authorization);
        // ilog("${to} , ${from} , ${receiver} , ${name}",("to",dataJson.to.to_string())("from",dataJson.from.to_string())("receiver",dataJson.receiver.to_string())("name",dataJson.name.to_string()) );

        if( std::find(filter_out.begin(), filter_out.end(), action.name.to_string())!=filter_out.end() ){
            try{
                *m_session << "INSERT INTO actions(account, created_at, name, data, authorization, transaction_id, eosto, eosfrom, receiver, payer, newaccount, sellram_account) "
                                "VALUES (:ac, FROM_UNIXTIME(:ca), :na, :da, :auth, :ti, :to, :form, :receiver, :payer, :newaccount, :sellram_account) ",
                    soci::use(action.account.to_string()),
                    soci::use(timestamp),
                    soci::use(action.name.to_string()),
                    soci::use(json),
                    soci::use(json_auth),
                    soci::use(transaction_id_str),
                    soci::use(dataJson.to.to_string()),
                    soci::use(dataJson.from.to_string()),
                    soci::use(dataJson.receiver.to_string()),
                    soci::use(dataJson.payer.to_string()),
                    soci::use(dataJson.name.to_string()),
                    soci::use(dataJson.account.to_string());
            } catch(...) {
                wlog("insert action failed in ${n}::${a}",("n",action.account.to_string())("a",action.name.to_string()));
                wlog("${data}",("data",fc::json::to_string(action)));
            }
        }

        try {
            parse_actions( action );
        } catch(std::exception& e){
            wlog(e.what());
        } catch(...){
            wlog("Unknown excpetion.");
        }
    }

    void actions_table::parse_actions( chain::action action ) {
        
        if(action.name == newaccount && action.account == chain::config::system_account_name) {
            auto action_data = action.data_as<chain::newaccount>();
            *m_session << "INSERT INTO accounts (name) VALUES (:name)",
                    soci::use(action_data.name.to_string());

            for (const auto& key_owner : action_data.owner.keys) {
                string permission_owner = "owner";
                string public_key_owner = static_cast<string>(key_owner.key);
                *m_session << "INSERT INTO accounts_keys(account, public_key, permission) VALUES (:ac, :ke, :pe) ",
                        soci::use(action_data.name.to_string()),
                        soci::use(public_key_owner),
                        soci::use(permission_owner);
            }

            for (const auto& key_active : action_data.active.keys) {
                string permission_active = "active";
                string public_key_active = static_cast<string>(key_active.key);
                *m_session << "INSERT INTO accounts_keys(account, public_key, permission) VALUES (:ac, :ke, :pe) ",
                        soci::use(action_data.name.to_string()),
                        soci::use(public_key_active),
                        soci::use(permission_active);
            }

        }
    }


    string actions_table::add_data(chain::action action){
        string json_str = "{}";

        if(action.data.size() ==0 ){
            ilog("data size is 0.");
             return json_str;
        }

        try{
            //当为set contract时 存储abi
            if( action.account == chain::config::system_account_name ){
                if( action.name == setabi ){
                    auto setabi = action.data_as<chain::setabi>();
                    try{
                        const chain::abi_def& abi_def = fc::raw::unpack<chain::abi_def>(setabi.abi);
                        json_str = fc::json::to_string( abi_def );

                        try{
                            *m_session << "UPDATE accounts SET abi = :abi, updated_at = NOW() WHERE name = :name",soci::use(json_str),soci::use(setabi.account.to_string());
                            // ilog("update abi ${n}",("n",action.account.to_string()));
                        }catch(...){
                            wlog("insert account abi failed");
                        }

                        return json_str;
                    }catch(fc::exception& e){
                        wlog("get setabi data wrong ${e}",("e",e.what()));
                    }
                }
            }

            chain::abi_def abi;
            std::string abi_def_account;
            chain::abi_serializer abis;
            soci::indicator ind;
            //get account abi
            *m_session << "SELECT abi FROM accounts WHERE name = :name", soci::into(abi_def_account, ind), soci::use(action.account.to_string());

            if(!abi_def_account.empty()){
                try {
                    abi = fc::json::from_string(abi_def_account).as<chain::abi_def>();
                    abis.set_abi( abi, max_serialization_time );
                    auto binary_data = abis.binary_to_variant( abis.get_action_type(action.name), action.data, max_serialization_time);
                    json_str = fc::json::to_string(binary_data);
                    return json_str;
                } catch(...) {
                    wlog("unable to convert account abi to abi_def for ${s}::${n} :${abi}",("s",action.account)("n",action.name)("abi",action.data));
                    wlog("analysis data failed");
                }
            }else{
                wlog("${n} abi is null.",("n",action.account));
            }

        }catch( std::exception& e ) {
            ilog( "Unable to convert action.data to ABI: ${s}::${n}, std what: ${e}",
                    ("s", action.account)( "n", action.name )( "e", e.what()));
        } catch( ... ) {
            ilog( "Unable to convert action.data to ABI: ${s}::${n}, unknown exception",
                    ("s", action.account)( "n", action.name ));
        }
        return json_str;
    }

    const chain::account_name actions_table::newaccount = chain::newaccount::get_name();
    const chain::account_name actions_table::setabi = chain::setabi::get_name();

} // namespace
