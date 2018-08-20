// #include "transactions_table.hpp"
#include <eosio/sql_db_plugin/transactions_table.hpp>

#include <chrono>
#include <fc/log/logger.hpp>

namespace eosio {

    transactions_table::transactions_table(std::shared_ptr<soci_session_pool> session_pool):
        m_session_pool(session_pool)
    {

    }

    void transactions_table::add( chain::transaction transaction ) {
        auto m_session = m_session_pool->get_session();

        const auto transaction_id_str = transaction.id().str();
        const auto expiration = std::chrono::seconds{transaction.expiration.sec_since_epoch()}.count();
        try{
            *m_session << "INSERT INTO transactions(id, ref_block_num, ref_block_prefix, expiration, pending, created_at, num_actions) "
                        "VALUES (:id, :rbi, :rb, FROM_UNIXTIME(:ex), :pe, FROM_UNIXTIME(:ca), :na)",
                soci::use(transaction_id_str),
                soci::use(transaction.ref_block_num),
                soci::use(transaction.ref_block_prefix),
                soci::use(expiration),
                soci::use(0),
                soci::use(expiration),
                soci::use(transaction.total_actions());
        } catch(soci::mysql_soci_error& e) {
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch (std::exception& e) {
            wlog("insert transaction failed. ${id}",("id",transaction_id_str));
            wlog("${e}",("e",e.what()));
        } catch(...){
            wlog("insert transaction failed. ${id}",("id",transaction_id_str));
        }
    }

    void transactions_table::add( chain::transaction transaction, chain::block_timestamp_type block_time, uint32_t block_num ) {
        auto m_session = m_session_pool->get_session();

        const auto transaction_id_str = transaction.id().str();
        const auto expiration = std::chrono::seconds{transaction.expiration.sec_since_epoch()}.count();
        const auto blocktime = std::chrono::seconds{block_time.operator fc::time_point().sec_since_epoch()}.count();
        try{
            *m_session << "INSERT INTO transactions(id, block_num, ref_block_num, ref_block_prefix, expiration, pending, created_at, num_actions) "
                        "VALUES (:id, :bn, :rbi, :rb, FROM_UNIXTIME(:ex), :pe, FROM_UNIXTIME(:ca), :na)",
                soci::use(transaction_id_str),
                soci::use(block_num),
                soci::use(transaction.ref_block_num),
                soci::use(transaction.ref_block_prefix),
                soci::use(expiration),
                soci::use(0),
                soci::use(blocktime),
                soci::use(transaction.total_actions());
        } catch(soci::mysql_soci_error& e) {
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch (std::exception& e) {
            wlog("insert transaction failed. ${id}",("id",transaction_id_str));
            wlog("${e}",("e",e.what()));
        } catch(...){
            wlog("insert transaction failed. ${id}",("id",transaction_id_str));
        }
    }

    void transactions_table::add_scheduled_or_except( string transaction_id_str,  chain::block_timestamp_type block_time, uint32_t block_num ) {
        auto m_session = m_session_pool->get_session();

        const auto blocktime = std::chrono::seconds{block_time.operator fc::time_point().sec_since_epoch()}.count();

        try{
            *m_session << "REPLACE INTO transactions(id, block_num) VALUES (:id, :bn)",
                soci::use(transaction_id_str),
                soci::use(block_num);
        } catch(soci::mysql_soci_error& e) {
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch (std::exception& e) {
            wlog("insert transaction failed. ${id}",("id",transaction_id_str));
            wlog("${e}",("e",e.what()));
        } catch(...){
            wlog("insert transaction failed. ${id}",("id",transaction_id_str));
        }
    }

    void transactions_table::irreversible_set( std::string block_id, bool irreversible, std::string transaction_id_str) {
        auto m_session = m_session_pool->get_session();

        try{
            *m_session << "UPDATE transactions SET irreversible = :irreversible WHERE id = :id ",
                soci::use(irreversible?1:0),
                soci::use(transaction_id_str);
        } catch(soci::mysql_soci_error& e) {
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch (std::exception& e) {
            wlog("update transaction failed ${id}",("id",transaction_id_str));
            wlog("${e}",("e",e.what()));
        } catch(...) {
            wlog("update transaction failed ${id}",("id",transaction_id_str));
        }
    }

    bool transactions_table::irreversible_set( bool irreversible, std::string transaction_id_str) {
        auto m_session = m_session_pool->get_session();

        int amount = 0;
        try{
            soci::statement st = ( m_session->prepare << "UPDATE transactions SET irreversible = :irreversible WHERE id = :id ",
                                        soci::use(irreversible?1:0),
                                        soci::use(transaction_id_str) );
            st.execute(true);
            amount = st.get_affected_rows();

            //sometime soci will error, amount always is 0;
            if(amount==0){

            }

        } catch(soci::mysql_soci_error& e) {
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch (std::exception& e) {
            wlog("update transaction failed ${id}",("id",transaction_id_str));
            wlog("${e}",("e",e.what()));
        } catch(...) {
            wlog("update transaction failed ${id}",("id",transaction_id_str));
        }
        return amount > 0;
    }

    bool transactions_table::find_transaction( std::string transaction_id_str) {
        auto m_session = m_session_pool->get_session();
        
        int amount;
        try{
            *m_session << "SELECT COUNT(*) FROM transactions WHERE id = :id",
                soci::into(amount),
                soci::use(transaction_id_str);
        } catch(soci::mysql_soci_error& e) {
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch(...) {
            amount = 0;
            wlog("find transaction failed. ${id}",("id",transaction_id_str));
        }
        return amount > 0;
    }

    void transactions_table::delete_transaction(string transaction_id_str){
        auto m_session = m_session_pool->get_session();
        
        try{
            *m_session << "DELETE FROM transactions WHERE id = :id", soci::use(transaction_id_str);
        } catch(soci::mysql_soci_error& e) {
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch(...) {
            wlog("delete_transaction failed. ${id}",("id",transaction_id_str));
        }
    }

    uint32_t transactions_table::get_transaction_block_num(string transaction_id_str){
        auto m_session = m_session_pool->get_session();
        
        uint32_t block_num;
        try{
            *m_session << "SELECT block_num FROM transactions WHERE id = :id",
                soci::into(block_num),
                soci::use(transaction_id_str);
        } catch(soci::mysql_soci_error& e) {
            block_num = 0;
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch(...) {
            block_num = 0;
            wlog("get_transaction_block_num failed. ${id}",("id",transaction_id_str));
        }
        return block_num;
    }

    void transactions_table::get_irreversible_transaction(string transaction_id_str,string& tx_id, int64_t& timestamp){
        auto m_session = m_session_pool->get_session();

        try{
            *m_session << " SELECT id, UNIX_TIMESTAMP(created_at) FROM transactions where id = :tx_id and irreversible = 1",
                soci::into(tx_id),
                soci::into(timestamp),
                soci::use(transaction_id_str);
                
        } catch(soci::mysql_soci_error& e) {
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch (std::exception& e) {
            wlog( "${e} ${id} ${data}",("e",e.what())("id",transaction_id_str)("data",tx_id) );
        }catch(...){
            wlog("insert scheduled transaction id failed. ${id}",("id",transaction_id_str));
        }

        if(tx_id.empty()){
            // wlog( "scheduled transaction id is null. ${id}",("id",transaction_id_str) );
            return ;
        }

        return ;
    }

    uint32_t transactions_table::get_max_irreversible_block_num(){
        auto m_session = m_session_pool->get_session();
        
        uint32_t block_num;
        try{
            *m_session << "SELECT max(block_num) FROM transactions where irreversible = 1 ",
                soci::into(block_num);
        } catch(soci::mysql_soci_error& e) {
            block_num = 0;
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch(...) {
            block_num = 0;
            wlog("get_scheduled_transaction_max_block_num failed.");
        }
        return block_num;
    }

    bool transactions_table::is_max_block_num_in_current_state( string transaction_id_str ){
        auto m_session = m_session_pool->get_session();

        int amount = 0;
        try{
            *m_session << "SELECT ( CASE WHEN ISNULL( block_num ) THEN 0 WHEN irreversible = 1 THEN 0 WHEN ISNULL( (SELECT max(block_num) FROM transactions WHERE irreversible = 1) ) THEN 0 WHEN block_num >= (SELECT max(block_num) from transactions where irreversible = 1 ) THEN 0 ELSE 1 END )from transactions where id = :id ",
                soci::into(amount),
                soci::use(transaction_id_str);
        } catch(soci::mysql_soci_error& e) {
            amount = 0;
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch(...) {
            amount = 0;
            wlog("is_max_block_num_in_current_state failed.");
        }
        return amount !=0;
    }

    bool transactions_table::is_current_transaction(string transaction_id_str, uint32_t block_num){

        auto m_session = m_session_pool->get_session();

        int amount = 0;
        try{
            *m_session << "SELECT (CASE WHEN (SELECT count(*) FROM transactions WHERE id = :id)!=0 THEN 0 WHEN ISNULL((select max(block_num) FROM transactions)) THEN 0 WHEN (select max(block_num) FROM transactions) <= :bn THEN 0 ELSE 1 END) ",
                soci::into(amount),
                soci::use(transaction_id_str),
                soci::use(block_num);
        } catch(soci::mysql_soci_error& e) {
            amount = 0;
            wlog("soci::error: ${e}",("e",e.what()) );
        } catch(...) {
            amount = 0;
            wlog("is_current_transaction failed.");
        }
        return amount ==0;
    }

} // namespace
