#pragma once

#include <memory>
#include <soci/soci.h>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/time.hpp>

#include <eosio/sql_db_plugin/sql_db_plugin.hpp>

namespace eosio{

class mysql_table{
    public:

        void reconnect(std::shared_ptr<soci::session> m_session){
            try{
                *m_session << "select 1;";
            } catch(std::exception& e) {
                wlog( "${e}",("e",e.what()) );
                wlog( "数据库timeout 进行重连 " );
                m_session->reconnect();
            } catch(...) {
                wlog("未知异常");
                wlog( "进行重连 " );
                m_session->reconnect();
            }
        }

        fc::microseconds max_serialization_time = fc::microseconds(150*1000);

};


}