CXX = g++

HEADER_DIR = ../include

FPTREE_HEADER_DIR = $(HEADER_DIR)/fptree

CXXFLAGS = -g -std=c++11

CFLAGS = -fPIC -std=c99 -O3 -msse4.2 -mpclmul -march=native -funroll-loops -Wstrict-overflow -Wstrict-aliasing -Wall -Wextra -pedantic -Wshadow

MAIN = ./bin/main

YCSB = ./bin/ycsb

LYCSB = ./bin/lycsb

BIN = bin

OBJ = ./obj

FPTREE = fptree.o
UTILITY = utility.o
PMEM = pmem.o
PALLOCATOR = pallocator.o
CLHASH = clhash.o

all : $(LYCSB) $(Y/home/u/Desktop/fp-tree/src/ycsb.cppCSB) $(MAIN) $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH)

PROJECT_ROOT:=$(shell pwd)/..
LEVELDB_DB_PATH:=/tmp/leveldb
LEVELDB_PATH:=$(PROJECT_ROOT)/leveldb
LEVELDB_INCLUDE_PATH:=$(LEVELDB_PATH)/include
LEVELDB_LIBRARY_PATH:=$(LEVELDB_PATH)/build
LEVELDB_FLAGS:=-DLEVELDB_DB_PATH=\"$(LEVELDB_DB_PATH)\"  -DPROJECT_ROOT=\"$(PROJECT_ROOT)\"  -I$(LEVELDB_INCLUDE_PATH)

LEVELDB:=\
	@echo "Building $@" \
	@if ! [ -d "$(PROJECT_ROOT)/leveldb" ] || ! [ -f "$(PROJECT_ROOT)/leveldb/build/libleveldb.a" ]; then \
    	echo "leveldb not cloned"; \
		cd $(PROJECT_ROOT); \
		git submodule init; \
		git submodule update; \
		cd leveldb; \
		echo "building leveldb"; \
		mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .; \
		cd $(PROJECT_ROOT)/src; \
	fi
#============================TODO: all below============================
$(PMEM) : pmem.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR) 

main.o : main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR)

$(MAIN): main.o $(FPTREE) $(PALLOCATOR) $(UTILITY) $(PMEM) $(CLHASH)
	$(CXX) $(CXXFLAGS) $^ -lpmem -lpthread -o $@

$(FPTREE) : fptree.cpp $(HEADER_DIR)/utility/pmem.h
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR) 

$(UTILITY) : utility.cpp $(HEADER_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR)

$(PALLOCATOR) : p_allocator.cpp $(HEADER_DIR)/utility/p_allocator.h

	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR)

$(CLHASH) : clhash.c $(HEADER_DIR)

	$(CC) $(CFLAGS) -c $< -o $@ -I $(HEADER_DIR)

$(YCSB): ycsb.cpp $(FPTREE) $(PALLOCATOR) $(UTILITY) $(PMEM) $(CLHASH)

	$(LEVELDB)

	@mkdir -p $(BIN)

	$(CXX) $(CXXFLAGS) $(LEVELDB_FLAGS) -c $< -o ycsb.o -I $(HEADER_DIR) -L$(LEVELDB_LIBRARY_PATH)
	$(CXX) $(CXXFLAGS) $(LEVELDB_FLAGS) ycsb.o $(FPTREE) $(PALLOCATOR) $(UTILITY) $(PMEM) $(CLHASH) -o $@ -I $(HEADER_DIR) -L$(LEVELDB_LIBRARY_PATH) -lleveldb -lpthread -lpmem

$(LYCSB): lycsb.cpp 

	$(LEVELDB)

	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(LEVELDB_FLAGS) $< -o $@ -L$(LEVELDB_LIBRARY_PATH) -lleveldb -lpthread

$(BIN):
	mkdir -p bin

all : $(ALL)

clean :
	rm -rf *.o ./bin/* ../data/*

cleand : 
	rm -rf ../data/* 
