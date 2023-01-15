#pragma once

#include <string>
#include <vector>
#include "cabindb_namespace.h"
#include <rocksdb/options.h>

namespace rocksdb{
  class DB;
  class Env;
  class Cache;
  class FilterPolicy;
  class Snapshot;
  class Slice;
  class WriteBatch;
  class Iterator;
  class Logger;
  class ColumnFamilyHandle;
  class Status;
  struct Options;
  struct BlockBasedTableOptions;
  struct DBOptions;
  struct ColumnFamilyOptions;
}

namespace CABINDB_NAMESPACE {

enum Status {
  kOK = 0,
  kError,
  kNotFound,
  kNotImplemented
};

typedef std::pair<std::string, std::string> KVPair;

class Dstore {
  public:
    Dstore(const char *dbfilename, rocksdb::Options& options, std::vector<std::string> &cfshards);

    Status Read(const std::string &table, const std::string &key, std::string &value);

    Status Scan(const std::string &table, const std::string &key, int len, std::vector<std::string> &values);

    Status Insert(const std::string &table, const std::string &key, std::string &value);

    Status Delete(const std::string &table, const std::string &key);

    ~Dstore() {}

  private:
    rocksdb::DB *db_;
    std::string dbpath_;
    rocksdb::Options options_;
    std::vector<rocksdb::ColumnFamilyHandle*> cfhandles_;

};

}
