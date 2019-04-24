# FP Tree

Sun Yat-sen University DBMS 2019 Course Project.

Group Members:

- Huang Yuhui ([@huanghongxun](https://github.com/huanghongxun))
- Chen Tailin ([@ctlchild](https://github.com/ctlchild))
- Liang Saibo ([@dasinlsb](https://github.com/dasinlsb))
- Liu Haohua ([@howardlau1999](https://github.com/howardlau1999))
- Gu Yuran ([@gyr5](https://github.com/gyr5))

## Building & Running

Firstly make sure your system meets the requirement of running Ubuntu 18.04 (or higher) and follow the instructions in [Prerequisite](PREREQUISITE.md) section to set up an environment for NVM simulation.

## Benchmark

Run YCSB on level-db:

```
make LEVELDB_DB_PATH=/path/to/leveldb lycsb
./build/lycsb
```