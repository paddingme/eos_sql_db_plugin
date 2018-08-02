/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <boost/noncopyable.hpp>
#include <boost/chrono.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/transaction.hpp>
#include <fc/log/logger.hpp>
#include <eosio/sql_db_plugin/database.hpp>

// #include "database.hpp"

namespace eosio {

class consumer final : public boost::noncopyable {
    public:
        consumer(std::unique_ptr<database> db,std::unique_ptr<database> db2,std::unique_ptr<database> db3, size_t queue_size);
        ~consumer();
        void shutdown();

        template<typename Queue, typename Entry>
        void queue(boost::mutex&, boost::condition_variable&, Queue&, const Entry&, size_t );

        void push_transaction_metadata( const chain::transaction_metadata_ptr& );
        void push_transaction_trace( const chain::transaction_trace_ptr& );
        void push_block_state( const chain::block_state_ptr& );
        void push_irreversible_block_state( const chain::block_state_ptr& );
        void push_irreversible_block_for_traces_state( const tx_id_block_time& );
        void run_blocks();
        void run_traces();
        void run_irreversible();
        void run_irreversible_for_traces();

        std::deque<chain::block_state_ptr> block_state_queue;
        std::deque<chain::block_state_ptr> block_state_process_queue;
        std::deque<chain::block_state_ptr> irreversible_block_state_queue;
        std::deque<chain::block_state_ptr> irreversible_block_state_process_queue;
        std::deque<tx_id_block_time> irreversible_block_for_traces_state_queue;
        std::deque<tx_id_block_time> irreversible_block_for_traces_state_process_queue;
        std::deque<chain::transaction_metadata_ptr> transaction_metadata_queue;
        std::deque<chain::transaction_metadata_ptr> transaction_metadata_process_queue;
        std::deque<chain::transaction_trace_ptr> transaction_trace_queue;
        std::deque<chain::transaction_trace_ptr> transaction_trace_process_queue;

        std::unique_ptr<database> db;
        std::unique_ptr<database> db2;
        std::unique_ptr<database> db3;
        size_t queue_size;
        boost::atomic<bool> exit{false};
        boost::thread consume_blocks;
        boost::thread consume_thread_run_traces;
        boost::thread consume_thread_run_irreversible;
        boost::thread consume_thread_run_irreversible_for_traces;
        boost::mutex mtx_blocks;
        boost::mutex mtx_traces;
        boost::mutex mtx_irreversible;
        boost::mutex mtx_irreversible_for_traces;
        boost::mutex mtx_db;
        boost::condition_variable condition;

    };

    consumer::consumer(std::unique_ptr<database> db, std::unique_ptr<database> db2, std::unique_ptr<database> db3, size_t queue_size):
        db(std::move(db)),
        db2(std::move(db2)),
        db3(std::move(db3)),
        queue_size(queue_size),
        exit(false),
        // consume_thread_run_blocks(boost::thread([&]{this->run_blocks();})),
        consume_thread_run_traces(boost::thread([&]{this->run_traces();})),
        consume_thread_run_irreversible(boost::thread([&]{this->run_irreversible();})),
        consume_thread_run_irreversible_for_traces(boost::thread([&]{this->run_irreversible_for_traces();}))
        { }

    consumer::~consumer() {
        exit = true;
        condition.notify_all();
        consume_thread_run_irreversible_for_traces.join();
        consume_thread_run_traces.join();
        consume_thread_run_irreversible.join();
    }

    void consumer::shutdown() {
        exit = true;
        condition.notify_all();
        consume_thread_run_irreversible_for_traces.join();
        consume_thread_run_traces.join();
        consume_thread_run_irreversible.join();
    }

    template<typename Queue, typename Entry>
    void consumer::queue(boost::mutex& mtx, boost::condition_variable& condition, Queue& queue, const Entry& e, size_t queue_size) {
        int sleep_time = 100;
        size_t last_queue_size = 0;
        boost::mutex::scoped_lock lock(mtx);
        if (queue.size() > queue_size) {
            lock.unlock();
            condition.notify_one();
            if (last_queue_size < queue.size()) {
                sleep_time += 100;
            } else {
                sleep_time -= 100;
                if (sleep_time < 0) sleep_time = 100;
            }
            last_queue_size = queue.size();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(sleep_time));
            lock.lock();
        }
        queue.emplace_back(e);
        lock.unlock();
        condition.notify_all();
    }

    void consumer::push_block_state( const chain::block_state_ptr& bs ){
        try {
            queue(mtx_blocks, condition, block_state_queue, bs, queue_size);
        } catch (fc::exception& e) {
            elog("FC Exception while accepted_block ${e}", ("e", e.to_string()));
        } catch (std::exception& e) {
            elog("STD Exception while accepted_block ${e}", ("e", e.what()));
        } catch (...) {
            elog("Unknown exception while accepted_block");
        }
    }

    void consumer::push_irreversible_block_state( const chain::block_state_ptr& bs ){
        try {
            queue(mtx_irreversible, condition, irreversible_block_state_queue, bs, queue_size);
        } catch (fc::exception& e) {
            elog("FC Exception while applied_irreversible_block ${e}", ("e", e.to_string()));
        } catch (std::exception& e) {
            elog("STD Exception while applied_irreversible_block ${e}", ("e", e.what()));
        } catch (...) {
            elog("Unknown exception while applied_irreversible_block");
        }
    }

    void consumer::push_irreversible_block_for_traces_state( const tx_id_block_time& traces_params ){
        try {
            queue(mtx_irreversible_for_traces, condition, irreversible_block_for_traces_state_queue, traces_params, queue_size);
        } catch (fc::exception& e) {
            elog("FC Exception while applied_irreversible_block ${e}", ("e", e.to_string()));
        } catch (std::exception& e) {
            elog("STD Exception while applied_irreversible_block ${e}", ("e", e.what()));
        } catch (...) {
            elog("Unknown exception while applied_irreversible_block");
        }
    }

    void consumer::push_transaction_metadata( const chain::transaction_metadata_ptr& tm){
        try {
            queue(mtx_traces, condition, transaction_metadata_queue, tm, queue_size);
        } catch (fc::exception& e) {
            elog("FC Exception while accepted_transaction ${e}", ("e", e.to_string()));
        } catch (std::exception& e) {
            elog("STD Exception while accepted_transaction ${e}", ("e", e.what()));
        } catch (...) {
            elog("Unknown exception while accepted_transaction");
        }
    }

    void consumer::push_transaction_trace( const chain::transaction_trace_ptr& tt){
        try {
            queue(mtx_traces, condition, transaction_trace_queue, tt, queue_size);
        } catch (fc::exception& e) {
            elog("FC Exception while applied_transaction ${e}", ("e", e.to_string()));
        } catch (std::exception& e) {
            elog("STD Exception while applied_transaction ${e}", ("e", e.what()));
        } catch (...) {
            elog("Unknown exception while applied_transaction");
        }
    }

    void consumer::run_blocks() {
        ilog("Consumer thread Start run_blocks");
        while (!exit) { 
            try{
                boost::mutex::scoped_lock lock(mtx_blocks);
                while(block_state_queue.empty() && !exit){
                    condition.wait(lock);
                }

                size_t block_state_size = block_state_queue.size();
                if( block_state_size > 0 ){
                    block_state_process_queue = std::move(block_state_queue);
                }

                lock.unlock();

                if( block_state_size > (queue_size * 0.75)) {
                    wlog("reversible queue size: ${q}", ("q", block_state_size));
                } else if (exit) {
                    ilog("reversible draining queue, size: ${q}", ("q", block_state_size));
                }          

                // process blocks
                while (!block_state_process_queue.empty()) {
                    const auto& bs = block_state_process_queue.front();
                    db->consume_block_state( bs );
                    block_state_process_queue.pop_front();
                }

                condition.notify_all();
            } catch (fc::exception& e) {
                elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
            } catch (std::exception& e) {
                elog("STD Exception while consuming block ${e}", ("e", e.what()));
            } catch (...) {
                elog("Unknown exception while consuming block");
            }  

        }
        
        ilog("Consumer thread End run_blocks");
    }  

    void consumer::run_traces() {
        ilog("Consumer thread Start run_traces");
        while (!exit) { 
            try{
                boost::mutex::scoped_lock lock(mtx_traces);
                while(transaction_trace_queue.empty()&& !exit){
                    condition.wait(lock);
                }

                // size_t transaction_metadata_size = transaction_metadata_queue.size();
                // if( transaction_metadata_size > 0 ){
                //     transaction_metadata_process_queue = std::move(transaction_metadata_queue);
                // }

                size_t transaction_trace_size = transaction_trace_queue.size();
                if( transaction_trace_size > 0 ){
                    transaction_trace_process_queue = std::move(transaction_trace_queue);
                }

                lock.unlock();

                if( transaction_trace_size> (queue_size * 0.75)) {
                    wlog("reversible queue size: ${q}", ("q", transaction_trace_size));
                } else if (exit) {
                    ilog("reversible draining queue, size: ${q}", ("q", transaction_trace_size));
                }


                //process trace
                while (!transaction_trace_process_queue.empty()) {
                    const auto& tt = transaction_trace_process_queue.front();
                    db2->consume_transaction_trace(tt);
                    transaction_trace_process_queue.pop_front();
                }

                // process transactions
                // while (!transaction_metadata_process_queue.empty()) {
                //     const auto& tm = transaction_metadata_process_queue.front();
                //     db->consume_transaction_metadata(tm);
                //     transaction_metadata_process_queue.pop_front();
                // }             

                condition.notify_all();
            } catch (fc::exception& e) {
                elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
            } catch (std::exception& e) {
                elog("STD Exception while consuming block ${e}", ("e", e.what()));
            } catch (...) {
                elog("Unknown exception while consuming block");
            }  

        }
        
        ilog("Consumer thread End run_traces");
    }

    void consumer::run_irreversible() {
        ilog("Consumer thread Start run_irreversible");
        while (!exit) { 
            try{
                boost::mutex::scoped_lock lock(mtx_irreversible);
                while(irreversible_block_state_queue.empty() && !exit){
                    condition.wait(lock);
                }


                size_t irreversible_block_state_size = irreversible_block_state_queue.size();
                if( irreversible_block_state_size > 0 ){
                    irreversible_block_state_process_queue = std::move(irreversible_block_state_queue);
                }

                lock.unlock();

                if( irreversible_block_state_size > (queue_size * 0.75) ) {
                    wlog("irreversible queue size: ${q}", ("q", irreversible_block_state_size));
                } else if (exit) {
                    ilog("irreversible draining queue, size: ${q}", ("q", irreversible_block_state_size));
                }

                boost::mutex::scoped_lock lock_db(mtx_db);
                
                // process irreversible blocks
                while (!irreversible_block_state_process_queue.empty()) {
                    const auto& bs = irreversible_block_state_process_queue.front();
                    db3->consume_irreversible_block_state(bs);
                    irreversible_block_state_process_queue.pop_front();
                }

            } catch (fc::exception& e) {
                elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
            } catch (std::exception& e) {
                elog("STD Exception while consuming block ${e}", ("e", e.what()));
            } catch (...) {
                elog("Unknown exception while consuming block");
            }  

        }
        
        ilog("Consumer thread End run_irreversible");
    }

    void consumer::run_irreversible_for_traces() {
        ilog("Consumer thread Start run_irreversible_for_traces");
        while (!exit) { 
            try{
                boost::mutex::scoped_lock lock(mtx_irreversible_for_traces);
                while(irreversible_block_state_queue.empty() && !exit){
                    condition.wait(lock);
                }


                size_t irreversible_block_for_traces_state_size = irreversible_block_for_traces_state_queue.size();
                if( irreversible_block_for_traces_state_size > 0 ){
                    irreversible_block_for_traces_state_process_queue = std::move(irreversible_block_for_traces_state_queue);
                }

                lock.unlock();

                if( irreversible_block_for_traces_state_size > (queue_size * 0.75) ) {
                    wlog("irreversible for traces queue size: ${q}", ("q", irreversible_block_for_traces_state_size));
                } else if (exit) {
                    ilog("irreversible for traces draining queue, size: ${q}", ("q", irreversible_block_for_traces_state_size));
                }

                // ilog("queue size ${size}",("size",irreversible_block_for_traces_state_size));

                boost::mutex::scoped_lock lock_db(mtx_db);
                
                // process irreversible blocks
                while (!irreversible_block_for_traces_state_process_queue.empty()) {
                    const auto& traces_params = irreversible_block_for_traces_state_process_queue.front();
                    db->consume_irreversible_block_for_traces_state(traces_params, lock_db, condition, exit);
                    irreversible_block_for_traces_state_process_queue.pop_front();
                }

            } catch (fc::exception& e) {
                elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
            } catch (std::exception& e) {
                elog("STD Exception while consuming block ${e}", ("e", e.what()));
            } catch (...) {
                elog("Unknown exception while consuming block");
            }  

        }
        
        ilog("Consumer thread end run_irreversible_for_traces");
    }

} // namespace

