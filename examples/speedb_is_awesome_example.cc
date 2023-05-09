// Copyright (C) 2023 Speedb Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>

#include "rocksdb/db.h"
#include "rocksdb/options.h"

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteOptions;
struct lockfree_stack {
  std::atomic_uint64_t counter = 0;
  std::atomic_uint64_t current_item = 0;
  std::vector<rocksdb::Snapshot*> snapshots;
  lockfree_stack(uint64_t size) {
    snapshots.resize(size);
  }
  void insert (const rocksdb::Snapshot* snapshot) {
    uint64_t item = current_item.fetch_add(1);
    snapshots[item] = const_cast<rocksdb::Snapshot*>(snapshot);
    counter.fetch_add(1);
  }
  const rocksdb::Snapshot* fetch() {
    uint64_t item = current_item.fetch_sub(1);
    counter.fetch_sub(1);
    std::cout << "counter: " << counter.load() << std::endl;
    std::cout << "item: " << item << std::endl;
    return snapshots[item];
  }
};
void getrelease(DB* db);
void writer(DB* db);
void releaser(DB* db, lockfree_stack* snapper);
void getter(DB* db, lockfree_stack* snapper);
#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\speedb_is_awesome_example";
#else
std::string kDBPath = "/tmp/speedb_is_awesome_example";
#endif
using namespace std::chrono_literals;
int main() {
  // Open the storage
  DB* db = nullptr;
  Options options;
  // create the DB if it's not already present
  lockfree_stack* ds = new lockfree_stack(1000000000);
  options.create_if_missing = true;
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());
  std::vector<std::thread> threads(10);
  std::thread write(writer, db);
  for(size_t i = 0;i < threads.size();++i) {
    threads[i] = std::thread(getrelease, db);
  }
  

  write.join();
  threads[10].join();
  // close DB
  s = db->Close();
  delete ds;
  assert(s.ok());
  return 0;
}

void getrelease(DB* db) {
  const rocksdb::Snapshot* snap;
  while (true) {
    snap = db->GetSnapshot();
    std::this_thread::sleep_for(1us);
    db->ReleaseSnapshot(snap);
  }
}

void writer(DB* db) {
  uint64_t i = 0;
  auto wopts = WriteOptions();
  while(true) {
    //std::this_thread::sleep_for(1ms);
    db->Put(wopts, "abc" + std::to_string(i), "abcd");
  }
}

void releaser(DB* db, lockfree_stack* snapper) {
  while (true) {
    std::this_thread::sleep_for(11ms);
    const rocksdb::Snapshot* snap = snapper->fetch();
    db->ReleaseSnapshot(snap);
  }
}

void getter(DB* db, lockfree_stack* snapper) {
  while(true) {
    std::this_thread::sleep_for(10ms);
    snapper->insert(db->GetSnapshot());
  }
}