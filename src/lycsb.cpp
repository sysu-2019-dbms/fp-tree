#include <leveldb/db.h>
#include <cassert>
#include <string>
// We do not use iostream because of performance issue
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <cstdio>

#define KEY_LEN 8
#define VALUE_LEN 8

using namespace std;

const string workload = PROJECT_ROOT "/workloads/";

const string load =
    workload + "220w-rw-50-50-load.txt";  // the workload_load filename
const string run =
    workload + "220w-rw-50-50-run.txt";  // the workload_run filename

const string filePath = LEVELDB_DB_PATH;

const int READ_WRITE_NUM = 350000;  // how many operations

void read_ycsb(const string& fn, int n, uint64_t keys[], bool ifInsert[]) {
    FILE* fp = fopen(fn.c_str(), "r");
    char op[8];
    uint64_t key;
    for (int i = 0; i < n; ++i) {
        if (fscanf(fp, "%s %lu", op, &key) == EOF) break;
        keys[i] = key;
        ifInsert[i] = *op == 'I';
    }

    fclose(fp);
}

void operate_db(leveldb::DB* db, uint64_t keys[], bool ifInsert[], int n,
                const leveldb::ReadOptions& read_options,
                const leveldb::WriteOptions& write_options, uint64_t& inserted,
                uint64_t& queried) {
    char _key[9] = {0};
    string value;
    for (int i = 0; i < n; ++i) {
        memcpy(_key, keys + i, 8);
        if (ifInsert[i]) {
            ++inserted;
            db->Put(write_options, _key, _key);
        } else {
            ++queried;
            db->Get(read_options, _key, &value);
        }
    }
}

int main() {
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::WriteOptions write_options;
    leveldb::ReadOptions read_options;

    // open and initial the levelDB
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, filePath, &db);
    assert(status.ok());

    uint64_t inserted = 0, queried = 0, t = 0;
    uint64_t* key = new uint64_t[2200000];  // the key and value are same
    bool* ifInsert = new bool[2200000];     // the operation is insertion or not
    struct timespec start, finish;  // use to caculate the time
    double single_time;             // single operation time

    printf("Load phase begins \n");
    // read the ycsb_load and store
    read_ycsb(load, 2200000, key, ifInsert);

    clock_gettime(CLOCK_MONOTONIC, &start);

    // load the workload in LevelDB
    operate_db(db, key, ifInsert, 2200000, read_options, write_options,
               inserted, queried);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 +
                  (finish.tv_nsec - start.tv_nsec);

    printf("Load phase finishes: %d items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

    int operation_num = 0;
    inserted = 0;
    
    // read the ycsb_run and store
    read_ycsb(run, READ_WRITE_NUM, key, ifInsert);

    clock_gettime(CLOCK_MONOTONIC, &start);

    // operate the levelDB
    operate_db(db, key, ifInsert, READ_WRITE_NUM, read_options, write_options,
               inserted, queried);

    operation_num = inserted + queried;
    
    clock_gettime(CLOCK_MONOTONIC, &finish);
    single_time = (finish.tv_sec - start.tv_sec) +
                  (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %d/%d items are inserted/searched\n",
           operation_num - inserted, inserted);
    printf("Run phase throughput: %f operations per second \n",
           READ_WRITE_NUM / single_time);
    return 0;
}
