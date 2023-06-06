// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <stdint.h>

#include "rocksdb/slice.h"
#include <mutex>
#include <shared_mutex>
#include <memory>

namespace ROCKSDB_NAMESPACE {

// Define all public custom types here.

using ColumnFamilyId = uint32_t;

// Represents a sequence number in a WAL file.
using SequenceNumber = uint64_t;

const SequenceNumber kMinUnCommittedSeq = 1;  // 0 is always committed

enum class TableFileCreationReason {
  kFlush,
  kCompaction,
  kRecovery,
  kMisc,
};

enum class BlobFileCreationReason {
  kFlush,
  kCompaction,
  kRecovery,
};

// The types of files RocksDB uses in a DB directory. (Available for
// advanced options.)
enum FileType {
  kWalFile,
  kDBLockFile,
  kTableFile,
  kDescriptorFile,
  kCurrentFile,
  kTempFile,
  kInfoLogFile,  // Either the current one, or an old one
  kMetaDatabase,
  kIdentityFile,
  kOptionsFile,
  kBlobFile
};

// User-oriented representation of internal key types.
// Ordering of this enum entries should not change.
enum EntryType {
  kEntryPut,
  kEntryDelete,
  kEntrySingleDelete,
  kEntryMerge,
  kEntryRangeDeletion,
  kEntryBlobIndex,
  kEntryDeleteWithTimestamp,
  kEntryWideColumnEntity,
  kEntryOther,
};

enum class WriteStallCause {
  // Beginning of CF-scope write stall causes
  //
  // Always keep `kMemtableLimit` as the first stat in this section
  kMemtableLimit,
  kL0FileCountLimit,
  kPendingCompactionBytes,
  kCFScopeWriteStallCauseEnumMax,
  // End of CF-scope write stall causes

  // Beginning of DB-scope write stall causes
  //
  // Always keep `kWriteBufferManagerLimit` as the first stat in this section
  kWriteBufferManagerLimit,
  kDBScopeWriteStallCauseEnumMax,
  // End of DB-scope write stall causes

  // Always add new WriteStallCause before `kNone`
  kNone,
};

enum class WriteStallCondition {
  kDelayed,
  kStopped,
  // Always add new WriteStallCondition before `kNormal`
  kNormal,
};

template<typename T>
class atomic_shared_ptr {
public:
    atomic_shared_ptr() : data(nullptr) {}

    explicit atomic_shared_ptr(std::shared_ptr<T> ptr) : data(ptr) {}

    atomic_shared_ptr(const atomic_shared_ptr& other) {
        std::lock_guard<std::mutex> lock(other.mutex);
        data = other.data;
    }

    atomic_shared_ptr& operator=(const atomic_shared_ptr& other) {
        if (this != &other) {
            std::lock_guard<std::mutex> this_lock(mutex);
            std::lock_guard<std::mutex> other_lock(other.mutex);
            data = other.data;
        }
        return *this;
    }
    atomic_shared_ptr& operator=(std::shared_ptr<T> ptr) {
        std::lock_guard<std::mutex> lock(mutex);
        data = ptr;
        return *this;
    }
    void store(std::shared_ptr<T> ptr) {
        std::lock_guard<std::mutex> lock(mutex);
        data = ptr;
    }

    std::shared_ptr<T> load() const {
        std::lock_guard<std::mutex> lock(mutex);
        return data;
    }
    // special load function for ReleaseSnapshot, in order to delete the SnapshotList Node if the last_snapshot_ is the only pointer 
    std::shared_ptr<T> LoadSnap() const {
        std::lock_guard<std::mutex> lock(mutex);
        if (data.use_count() == 2)
          data = nullptr;
        return data;
    }

    operator std::shared_ptr<T>() const {
        std::lock_guard<std::mutex> lock(mutex);
        return data;
    }

private:
    mutable std::mutex mutex;
    mutable std::shared_ptr<T> data;
};

}  // namespace ROCKSDB_NAMESPACE
