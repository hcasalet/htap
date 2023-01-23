#pragma once

#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"

using rocksdb::ColumnFamilyMetaData;
using rocksdb::CompactionOptions;
using rocksdb::DB;
using rocksdb::EventListener;
using rocksdb::FlushJobInfo;
using rocksdb::Options;
using rocksdb::ReadOptions;
using rocksdb::Status;
using rocksdb::WriteOptions;

namespace CABINDB_NAMESPACE {
struct CompactionTask;

class Compactor : public EventListener {
 public:
    virtual CompactionTask* PickCompaction(DB* db, const std::string& cf_name) = 0;

    // Schedule and run the specified compaction task in background.
    virtual void ScheduleCompaction(CompactionTask* task) = 0;
};

struct CompactionTask {
  CompactionTask(DB* _db, Compactor* _compactor,
                 const std::string& _column_family_name,
                 const std::vector<std::string>& _input_file_names,
                 const int _output_level,
                 const CompactionOptions& _compact_options, bool _retry_on_fail)
      : db(_db),
        compactor(_compactor),
        column_family_name(_column_family_name),
        input_file_names(_input_file_names),
        output_level(_output_level),
        compact_options(_compact_options),
        retry_on_fail(_retry_on_fail) {}
  DB* db;
  Compactor* compactor;
  const std::string& column_family_name;
  std::vector<std::string> input_file_names;
  int output_level;
  CompactionOptions compact_options;
  bool retry_on_fail;
};

class CabinCompactor : public Compactor {
 public:
    CabinCompactor(const Options options): options_(options) {
      compact_options_.compression = options_.compression;
      compact_options_.output_file_size_limit = options_.target_file_size_base;
    };

    void OnFlushCompleted(DB* db, const FlushJobInfo& info) override {
      CompactionTask* task = PickCompaction(db, info.cf_name);
      if (task != nullptr) {
         if (info.triggered_writes_stop) {
            task->retry_on_fail = true;
         }
         // Schedule compaction in a different thread.
         ScheduleCompaction(task);
      }
    }

    CompactionTask* PickCompaction(DB* db, const std::string& cf_name) override {
      ColumnFamilyMetaData cf_meta;
      db->GetColumnFamilyMetaData(&cf_meta);

      std::vector<std::string> input_file_names;
      for (auto level : cf_meta.levels) {
         for (auto file : level.files) {
            if (file.being_compacted) {
               return nullptr;
            }
         input_file_names.push_back(file.name);
         }
      }
      return new CompactionTask(db, this, cf_name, input_file_names,
                              options_.num_levels - 1, compact_options_, false);
    }

    void ScheduleCompaction(CompactionTask* task) override {
      options_.env->Schedule(&CabinCompactor::CompactFiles, task);
    }
    
    static void CompactFiles(void* arg) {
      std::unique_ptr<CompactionTask> task(
        reinterpret_cast<CompactionTask*>(arg));
      assert(task);
      assert(task->db);
      Status s = task->db->CompactFiles(
        task->compact_options, task->input_file_names, task->output_level);
      printf("CompactFiles() finished with status %s\n", s.ToString().c_str());
      if (!s.ok() && !s.IsIOError() && task->retry_on_fail) {
         // If a compaction task with its retry_on_fail=true failed,
         // try to schedule another compaction in case the reason
         // is not an IO error.
         CompactionTask* new_task =
            task->compactor->PickCompaction(task->db, task->column_family_name);
         task->compactor->ScheduleCompaction(new_task);
      }
    }

    ~CabinCompactor() {};
 private:
    Options options_;
    CompactionOptions compact_options_;
};

}