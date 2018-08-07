// #include "accounts_table.hpp"
#include <eosio/sql_db_plugin/accounts_table.hpp>
#include <fc/log/logger.hpp>

namespace eosio {

accounts_table::accounts_table(std::shared_ptr<soci_session_pool> session_pool):
    m_session_pool(session_pool)
{

}

void accounts_table::add(string name) {
    // reconnect(m_session);
    auto m_session = m_session_pool->get_session();
    try {
        *m_session << "INSERT INTO accounts (name) VALUES (:name)",
            soci::use(name);
    } catch(soci::mysql_soci_error e) {
        wlog("soci::error: ${e}",("e",e.what()) );
    } catch (std::exception const & e) {
        wlog( "exception: ${e}",("e",e.what()) );
    } catch (...){
        wlog( "unknow exception" );
    }
    
}

void accounts_table::add_eosio(string name,string abi) {
    auto m_session = m_session_pool->get_session();
    try {
        *m_session << "INSERT INTO accounts (name,abi) VALUES (:name,:abi)",
            soci::use(name),soci::use(abi);
    } catch(soci::mysql_soci_error e) {
        wlog("soci::error: ${e}",("e",e.what()) );
    } catch (std::exception const & e) {
        wlog( "exception: ${e}",("e",e.what()) );
    } catch (...){
        wlog( "unknow exception" );
    } 
}

bool accounts_table::exist(string name)
{
    auto m_session = m_session_pool->get_session();
    int amount;
    try {
        *m_session << "SELECT COUNT(*) FROM accounts WHERE name = :name", soci::into(amount), soci::use(name);
    } catch(soci::mysql_soci_error e) {
        wlog("soci::error: ${e}",("e",e.what()) );
    } catch (std::exception const & e) {
        amount = 0;
    }
    return amount > 0;
}

} // namespace
