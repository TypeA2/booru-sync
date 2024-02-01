#include "database.hpp"

database::database() {

}

pqxx::work database::work() {
    return pqxx::work { _conn };
}

database::operator pqxx::connection& () {
    return _conn;
}

pqxx::connection* database::operator->() {
    return &_conn;
}
