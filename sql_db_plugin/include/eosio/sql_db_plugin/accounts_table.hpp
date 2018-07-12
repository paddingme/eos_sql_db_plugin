#pragma once

#include <memory>
#include <soci/soci.h>

namespace eosio {

using std::string;

class accounts_table
{
    public:
        accounts_table(){};
        accounts_table(std::shared_ptr<soci::session> session);

        void drop();
        void create();
        void add(string name);
        bool exist(string name);
        void add_eosio(string name,string abi);

    private:
        std::shared_ptr<soci::session> m_session;
};

} // namespace

