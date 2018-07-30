// #include "transactions_table.hpp"
#include <eosio/sql_db_plugin/transactions_table.hpp>

#include <chrono>
#include <fc/log/logger.hpp>

namespace eosio {

    transactions_table::transactions_table(std::shared_ptr<soci::session> session):
        m_session(session) {

    }

    void transactions_table::drop() {
        try {
            *m_session << "DROP TABLE IF EXISTS transactions";
        }
        catch(std::exception& e){
            wlog(e.what());
        }
    }

    void transactions_table::create() {
        *m_session << "CREATE TABLE `transactions` ("
            "`tx_id` bigint(20) NOT NULL AUTO_INCREMENT,"
            "`id` varchar(64) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '',"
            "`block_id` varchar(64) NOT NULL DEFAULT '0',"
            "`ref_block_num` bigint(20) NOT NULL DEFAULT '0',"
            "`ref_block_prefix` bigint(20) NOT NULL DEFAULT '0',"
            "`expiration` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "`pending` tinyint(1) NOT NULL DEFAULT '0',"
            "`created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "`num_actions` bigint(20) DEFAULT '0',"
            "`updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "`irreversible` tinyint(1) NOT NULL DEFAULT '0',"
            "PRIMARY KEY (`tx_id`),"
            "UNIQUE INDEX `idx_transactions_id` (`id`),"
            "KEY `transactions_block_id` (`block_id`)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;";

        // *m_session << "CREATE INDEX transactions_block_id ON transactions (block_id);";

    }

    void transactions_table::add( chain::transaction transaction ) {
        const auto transaction_id_str = transaction.id().str();
        const auto expiration = std::chrono::seconds{transaction.expiration.sec_since_epoch()}.count();
        try{
            *m_session << "INSERT INTO transactions(id, ref_block_num, ref_block_prefix, expiration, pending, created_at, updated_at, num_actions) "
                        "VALUES (:id, :rbi, :rb, FROM_UNIXTIME(:ex), :pe, FROM_UNIXTIME(:ca), FROM_UNIXTIME(:ua), :na)",
                soci::use(transaction_id_str),
                soci::use(transaction.ref_block_num),
                soci::use(transaction.ref_block_prefix),
                soci::use(expiration),
                soci::use(0),
                soci::use(expiration),
                soci::use(expiration),
                soci::use(transaction.total_actions());
        } catch (std::exception e) {
            wlog("insert transaction failed. ${id}",("id",transaction_id_str));
            wlog("${e}",("e",e.what()));
        } catch(...){
            wlog("insert transaction failed. ${id}",("id",transaction_id_str));
        }
    }

    void transactions_table::irreversible_set( std::string block_id, bool irreversible, std::string transaction_id_str) {
        try{
            *m_session << "UPDATE transactions SET block_id = :block_id, irreversible = :irreversible WHERE id = :id ",
                soci::use(block_id),
                soci::use(irreversible?1:0),
                soci::use(transaction_id_str);
        } catch (std::exception e) {
            wlog("update transaction failed ${id}",("id",transaction_id_str));
            wlog("${e}",("e",e.what()));
        } catch(...) {
            wlog("update transaction failed ${id}",("id",transaction_id_str));
        }
    }

    bool transactions_table::find_transaction( std::string transaction_id_str) {
        int amount;
        try{
            *m_session << "SELECT COUNT(*) FROM transactions WHERE id = :id",
                soci::into(amount),
                soci::use(transaction_id_str);
        } catch(...) {
            amount = 0;
            wlog("find transaction failed. ${id}",("id",transaction_id_str));
        }
        return amount > 0;
    }

} // namespace
