CXX = g++

HEADER_DIR = ../include

FPTREE_HEADER_DIR = $(HEADER_DIR)/fptree

FPTREE_DB_PATH:=/pmem-fs/fptree/

FPTREE_FLAGS:=-DFPTREE_DB_PATH=\"$(FPTREE_DB_PATH)\"

CXXFLAGS = -g -O3 -std=c++11 -funroll-loops $(FPTREE_FLAGS)

CFLAGS = -fPIC -std=c99 -O3 -msse4.2 -mpclmul -march=native -funroll-loops -Wstrict-overflow -Wstrict-aliasing -Wall -Wextra -pedantic -Wshadow

MAIN = ./bin/main

YCSB = ./bin/ycsb

LYCSB = ./bin/lycsb

BIN = ./bin

OBJ = ../obj

FPTREE = $(OBJ)/fptree.o
UTILITY = $(OBJ)/utility.o
PMEM = $(OBJ)/pmem.o
PALLOCATOR = $(OBJ)/pallocator.o
CLHASH = $(OBJ)/clhash.o

all : $(OBJ) $(LYCSB) $(YCSB) $(MAIN) $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH) 

PROJECT_ROOT:=$(shell pwd)/..
LEVELDB_DB_PATH:=/pmem-fs/leveldb

LEVELDB_PATH:=$(PROJECT_ROOT)/leveldb
LEVELDB_INCLUDE_PATH:=$(LEVELDB_PATH)/include
LEVELDB_LIBRARY_PATH:=$(LEVELDB_PATH)/build
LEVELDB_FLAGS:=-DLEVELDB_DB_PATH=\"$(LEVELDB_DB_PATH)\"  -DPROJECT_ROOT=\"$(PROJECT_ROOT)\"  -I$(LEVELDB_INCLUDE_PATH)

LEVELDB:=$(PROJECT_ROOT)/leveldb/build/libleveldb.a

#============================TODO: all below============================
$(LEVELDB):	
	@ cd $(PROJECT_ROOT); \
	git submodule update --init --recursive; \
	cd $(PROJECT_ROOT)/leveldb; \
	echo "building leveldb"; \
	mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .; \
	cd $(PROJECT_ROOT)/src; \

$(PMEM) : pmem.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR) 

$(MAIN): main.cpp $(FPTREE) $(PALLOCATOR) $(UTILITY) $(PMEM) $(CLHASH)
	$(CXX) $(CXXFLAGS) -I$(HEADER_DIR) $^ -lpmem -lpthread -o $@

$(FPTREE) : fptree.cpp $(HEADER_DIR)/utility/pmem.h
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR) 

$(UTILITY) : utility.cpp $(HEADER_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR)

$(PALLOCATOR) : p_allocator.cpp $(HEADER_DIR)/utility/p_allocator.h

	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(HEADER_DIR)

$(CLHASH) : clhash.c $(HEADER_DIR)

	$(CC) $(CFLAGS) -c $< -o $@ -I $(HEADER_DIR)

$(YCSB): ycsb.cpp $(FPTREE) $(PALLOCATOR) $(UTILITY) $(PMEM) $(CLHASH) $(LEVELDB)
	@mkdir -p $(BIN)

	$(CXX) $(CXXFLAGS) $(LEVELDB_FLAGS) $^ -o $@ -I$(HEADER_DIR) -L$(LEVELDB_LIBRARY_PATH) -lleveldb -lpthread -lpmem

$(LYCSB): lycsb.cpp $(LEVELDB)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(LEVELDB_FLAGS) $< -o $@ -L$(LEVELDB_LIBRARY_PATH) -lleveldb -lpthread

$(OBJ):
	@mkdir -p $(OBJ)

$(BIN):
	@mkdir -p bin

all : $(ALL)

clean :
	rm -rf $(OBJ)/*.o ./bin/* ../data/*

cleand : 
	rm -rf ../data/* 
