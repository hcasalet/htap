#include "core/core_workload.h"
#include "cabindb_db.h"
#include "lib/coding.h"

using namespace std;

namespace ycsbc {

    CabinDB::CabinDB(const char *dbfilename, utils::Properties &props) {
        rocksdb::Options options;
        SetOptions(&options, props);

        int num_cfshards = stoi(props.GetProperty("columnfamilycount", "0"));
        for (int i = 0; i < num_cfshards; i++) {
            cfshards_.push_back("field"+std::to_string(i));
        }

        dstore_ = new cabindb::Dstore(dbfilename, options, cfshards_);
    }

    void CabinDB::SetOptions(rocksdb::Options *options, utils::Properties &props) {

        options->create_if_missing = true;
        options->compression = rocksdb::kNoCompression;
        //options->enable_pipelined_write = true;

        options->max_background_jobs = 2;
        options->max_bytes_for_level_base = 32ul * 1024 * 1024;
        options->write_buffer_size = 32ul * 1024 * 1024;
        options->max_write_buffer_number = 2;
        options->target_file_size_base = 4ul * 1024 * 1024;

        options->level0_file_num_compaction_trigger = 4;
        options->level0_slowdown_writes_trigger = 8;     
        options->level0_stop_writes_trigger = 12;

        options->use_direct_reads = true;
        options->use_direct_io_for_flush_and_compaction = true;

        uint64_t nums = stoi(props.GetProperty(CoreWorkload::RECORD_COUNT_PROPERTY));
        uint32_t key_len = stoi(props.GetProperty(CoreWorkload::KEY_LENGTH));
        uint32_t value_len = stoi(props.GetProperty(CoreWorkload::FIELD_LENGTH_PROPERTY));
        uint32_t cache_size = nums * (key_len + value_len) * 10 / 100;
        if(cache_size < 8 << 20) {
            cache_size = 8 << 20;
        }
        cache_ = rocksdb::NewLRUCache(cache_size);

        bool statistics = utils::StrToBool(props["dbstatistics"]);
        if(statistics){
            dbstats_ = rocksdb::CreateDBStatistics();
            options->statistics = dbstats_;
        }
    }

    int CabinDB::Read(const std::string &table, const std::string &key, const std::vector<std::string> *fields,
                      std::vector<KVPair> &result) 
    {
        std::string value;
        cabindb::Status s = dstore_->Read(table, key, value);
        if (s == cabindb::Status::kOK) {
            DeSerializeValue(value, result);
            return 0;
        }

        if (s == cabindb::Status::kNotFound) {
            noResultsInDefaultColumnFamily++;
            std::vector<std::string> values;
            for (auto field : *fields) {
                value.clear();
                cabindb::Status ss = dstore_->Read(field, key, value);
                if (ss == cabindb::Status::kOK) {
                    values.push_back(value);
                    continue;
                }
                if (ss == cabindb::Status::kNotFound) {
                    noResults++;
                    return 0;
                }
                std::cerr<<"read error"<<std::endl;
            }
            StitchColumns(values, result);
            return 0;
        }

        noResults++;
        std::cerr<<"read error"<<std::endl;
        return 1;
    }

    int CabinDB::Scan(const std::string &table, const std::string &key, int len,
                      const std::vector<std::string> *fields,
                      std::vector<std::vector<KVPair>> &result) 
    {
        std::vector<std::string> values;
        cabindb::Status s = dstore_->Scan(table, key, len, values);
        result.clear();
        if (s == cabindb::Status::kOK) {
            DeSerializeValues(values, result);
            noResultsInDefaultColumnFamily += (len - values.size());
            return 0;
        }

        if (s == cabindb::Status::kNotFound) {
            noResultsInDefaultColumnFamily += len;
            return 0;
        }
        
        return 1;
    }

    int CabinDB::Insert(const std::string &table, const std::string &key, std::vector<KVPair> &values)
    {
        std::string value;
        SerializeValue(values, value);

        cabindb::Status s = dstore_->Insert(table, key, value);
        if (s == cabindb::Status::kOK) {
            return 0;
        }

        return 1;
    }

    int CabinDB::Update(const std::string &table, const std::string &key, std::vector<KVPair> &values)
    {
        return Insert(table, key, values);
    }

    int CabinDB::Delete(const std::string &table, const std::string &key)
    {
        cabindb::Status s = dstore_->Delete(table, key);
        if (s == cabindb::Status::kOK) {
            return 0;
        }
        return 1;
    }

    void CabinDB::SerializeValue(std::vector<KVPair> &kvs, std::string &value) {
        value.clear();
        PutFixed64(&value, kvs.size());
        for(unsigned int i=0; i < kvs.size(); i++){
            PutFixed64(&value, kvs[i].first.size());
            value.append(kvs[i].first);
            PutFixed64(&value, kvs[i].second.size());
            value.append(kvs[i].second);
        }
    }

    void CabinDB::DeSerializeValue(std::string &value, std::vector<KVPair> &kvs){
        uint64_t offset = 0;
        uint64_t kv_num = 0;
        uint64_t key_size = 0;
        uint64_t value_size = 0;

        kv_num = DecodeFixed64(value.c_str());
        offset += 8;
        for( unsigned int i = 0; i < kv_num; i++){
            ycsbc::DB::KVPair pair;
            key_size = DecodeFixed64(value.c_str() + offset);
            offset += 8;

            pair.first.assign(value.c_str() + offset, key_size);
            offset += key_size;

            value_size = DecodeFixed64(value.c_str() + offset);
            offset += 8;

            pair.second.assign(value.c_str() + offset, value_size);
            offset += value_size;
            kvs.push_back(pair);
        }
    }

    void CabinDB::DeSerializeValues(std::vector<std::string> &values, std::vector<std::vector<KVPair>> &kvs_vec)
    {
        kvs_vec.clear();
        for (unsigned int i = 0; i < values.size(); i++) {
            std::vector<KVPair> kvs;
            DeSerializeValue(values[i], kvs);
            kvs_vec.push_back(kvs);
        }
    }

    void CabinDB::StitchColumns(std::vector<std::string> &values, std::vector<KVPair> &kvs)
    {
        kvs.clear();
        uint64_t offset = 0;
        for (unsigned int i = 0; i < values.size(); i++) {
            ycsbc::DB::KVPair pair;
            uint64_t kv_num = DecodeFixed64(values[i].c_str());
            offset += 8;

            uint64_t key_size = DecodeFixed64(values[i].c_str() + offset);
            offset += 8;

            pair.first.assign(values[i].c_str() + offset, key_size);
            offset += key_size;

            uint64_t value_size = DecodeFixed64(values[i].c_str() + offset);
            offset += 8;

            pair.second.assign(values[i].c_str() + offset, value_size);

            kvs.push_back(pair);
        }
    }

}