# FP Tree

Sun Yat-sen University DBMS 2019 Course Project.

Group Members:

- Huang Yuhui ([@huanghongxun](https://github.com/huanghongxun))
- Chen Tailin ([@ctlchild](https://github.com/ctlchild))
- Liang Saibo ([@dasinlsb](https://github.com/dasinlsb))
- Liu Haohua ([@howardlau1999](https://github.com/howardlau1999))
- Gu Yuran ([@gyr5](https://github.com/gyr5))

## Getting Source Code

```bash
git clone https://github.com/sysu-2019-dbms/fp-tree.git
cd fp-tree
git submodule update --init --recursive
```

## Building & Running

### Setup NVM Simulation Environment 

Firstly make sure your system meets the requirement of running Ubuntu 18.04 (or higher) and follow the instructions in [Prerequisite](PREREQUISITE.md) section to set up an environment for NVM simulation.

### Benchmark

Run YCSB on level-db:

```bash
cd src && make LEVELDB_DB_PATH=/path/to/leveldb ./bin/lycsb
./bin/lycsb
```

Run YCSB on both LevelDB and FPTree:

```bash
cd src && make LEVELDB_DB_PATH=/path/to/leveldb ./bin/ycsb
./bin/ycsb
```

### Google Test

```shell
cd test && make all
./bin/utility_test
./bin/fptree_test
```

The command `make all` will build googletest library into directory `test`. You can try `make ./bin/utility_test` and `make ./bin/fptree_test` if you have global googletest installed.

## Test Result

### LYCSB

![LYCSB](./images/lycsb.png)

### YCSB

![YCSB](./image/YCSB.png)

### Google Test

#### Utility
![gtest_utility](./images/gtest_utility.png)

#### FP Tree
![gtest_fptree](./images/gtest_fptree.png)