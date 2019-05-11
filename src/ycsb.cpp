#include <leveldb/db.h>
#include <string>
#include "fptree/fptree.h"

#define KEY_LEN 8
#define VALUE_LEN 8
// The following should be defined during compile time
#ifndef PROJECT_ROOT
#define PROJECT_ROOT ".."
#endif

using namespace std;
const int n = 2200000;
const string workload = PROJECT_ROOT "/workloads/";

const string load =
    workload + "220w-rw-50-50-load.txt";  // the workload_load filename
const string run =
    workload + "220w-rw-50-50-run.txt";  // the workload_run filename

const string filePath = LEVELDB_DB_PATH;

const int READ_WRITE_NUM = 350000;  // TODO: amount of operations

void read_ycsb(const string& fn, int n, uint64_t keys[], bool ifInsert[]) {
    FILE*    fp = fopen(fn.c_str(), "r");
    char     op[8];
    uint64_t key;
    for (int i = 0; i < n; ++i) {
        if (fscanf(fp, "%s %lu", op, &key) == EOF) break;
        keys[i]     = key;
        ifInsert[i] = *op == 'I';
    }

    fclose(fp);
}

void operate_db(leveldb::DB* db, uint64_t keys[], bool ifInsert[], int n,
                const leveldb::ReadOptions&  read_options,
                const leveldb::WriteOptions& write_options, uint64_t& inserted,
                uint64_t& queried) {
    char   _key[9] = {0};
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

void operate_fptree(FPTree* fp, uint64_t keys[], bool ifInsert[], int n, uint64_t& inserted,
                    uint64_t& queried) {
    Key   _key;
    Value value;
    for (int i = 0; i < n; ++i) {
        _key = keys[i];
        if (ifInsert[i]) {
            ++inserted;
            fp->insert(_key, value);
        } else {
            ++queried;
            value = fp->find(_key);
        }
    }
}

void testFPTree() {
    FPTree          fptree(1028);
    uint64_t        inserted = 0, queried = 0, t = 0;
    uint64_t*       key      = new uint64_t[n];
    bool*           ifInsert = new bool[n];
    FILE *          ycsb, *ycsb_read;
    char*           buf = NULL;
    size_t          len = 0;
    struct timespec start, finish;
    double          single_time;

    printf("===================FPtreeDB===================\n");
    printf("FPTree DB Path: %s\n", FPTREE_DB_PATH);
    printf("Load phase begins \n");

    // TODO: read the ycsb_load
    read_ycsb(load, n, key, ifInsert);

    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: load the workload in the fptree
    operate_fptree(&fptree, key, ifInsert, n, inserted, queried);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 +
                  (finish.tv_nsec - start.tv_nsec);
    printf("Load phase finishes: %lu items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

    printf("Run phase begins\n");

    int operation_num = 0;
    inserted          = 0;
    // TODO: read the ycsb_run
    read_ycsb(run, READ_WRITE_NUM, key, ifInsert);

    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: operate the fptree
    operate_fptree(&fptree, key, ifInsert, READ_WRITE_NUM, inserted, queried);

    operation_num = inserted + queried;

    clock_gettime(CLOCK_MONOTONIC, &finish);
    single_time = (finish.tv_sec - start.tv_sec) +
                  (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %lu/%lu items are inserted/searched\n", inserted,
           operation_num - inserted);
    printf("Run phase throughput: %f operations per second \n",
           READ_WRITE_NUM / single_time);

    delete[] key;
    delete[] ifInsert;
}

void testLevelDB() {
    uint64_t        inserted = 0, queried = 0, t = 0;
    uint64_t*       key      = new uint64_t[n];
    bool*           ifInsert = new bool[n];
    FILE *          ycsb, *ycsb_read;
    char*           buf = NULL;
    size_t          len = 0;
    struct timespec start, finish;
    double          single_time;

    // LevelDB
    printf("===================LevelDB====================\n");
    printf("LevelDB DB Path: %s\n", filePath.c_str());
    memset(key, 0, 2200000);
    memset(ifInsert, 0, 2200000);

    leveldb::DB*          db;
    leveldb::Options      options;
    leveldb::ReadOptions  read_options;
    leveldb::WriteOptions write_options;
    options.create_if_missing = true;
    leveldb::Status status    = leveldb::DB::Open(options, filePath, &db);
    assert(status.ok());

    inserted = 0;
    printf("Load phase begins \n");
    read_ycsb(load, n, key, ifInsert);

    clock_gettime(CLOCK_MONOTONIC, &start);
    operate_db(db, key, ifInsert, n, read_options, write_options,
               inserted, queried);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 +
                  (finish.tv_nsec - start.tv_nsec);
    printf("Load phase finishes: %lu items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

    printf("Run phase begin\n");
    int operation_num = 0;
    inserted          = 0;
    queried           = 0;

    read_ycsb(run, READ_WRITE_NUM, key, ifInsert);

    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: run the workload_run in levelDB
    operate_db(db, key, ifInsert, READ_WRITE_NUM, read_options, write_options,
               inserted, queried);

    operation_num = inserted + queried;

    clock_gettime(CLOCK_MONOTONIC, &finish);

    single_time = (finish.tv_sec - start.tv_sec) +
                  (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %lu/%lu items are inserted/searched\n", inserted,
           operation_num - inserted);
    printf("Run phase throughput: %f operations per second \n",
           READ_WRITE_NUM / single_time);

    delete[] key;
    delete[] ifInsert;
}

int main() {
    testFPTree();
    testLevelDB();
    return 0;
}
