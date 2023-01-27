#include "db/db_factory.h"

#include <string>
#include <iostream>
#include "db/cabindb_db.h"

using namespace std;

namespace ycsbc {
//using ycsbc::DB;
//using ycsbc::DBFactory;

DB* DBFactory::CreateDB(utils::Properties &props) {
  if (props["dbname"] == "cabindb") {
    std::string dbpath = props.GetProperty("dbpath","/tmp/test-rocksdb");
    return new CabinDB(dbpath.c_str(), props);
  } else return nullptr;
}

}
