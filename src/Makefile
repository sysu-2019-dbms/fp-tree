CXX = g++

HEADER_DIR = ../include

FPTREE_HEADER_DIR = $(HEADER_DIR)/fptree

CXXFLAGS = -std=c++11

CFLAGS = -fPIC -std=c99 -O3 -msse4.2 -mpclmul -march=native -funroll-loops -Wstrict-overflow -Wstrict-aliasing -Wall -Wextra -pedantic -Wshadow

MAIN = ./bin/main

YCSB = ./bin/ycsb

LYCSB = ./bin/lycsb

BIN = bin

FPTREE = fptree.o

UTILITY = utility.o

PALLOCATOR = pallocator.o

CLHASH = clhash.o

ALL := $(LYCSB) $(YCSB) $(MAIN) $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH)

PROJECT_ROOT:=$(shell pwd)/..
LEVELDB_DB_PATH:=/tmp/leveldb
LEVELDB_PATH:=$(PROJECT_ROOT)/leveldb
LEVELDB_INCLUDE_PATH:=$(LEVELDB_PATH)/include
LEVELDB_LIBRARY_PATH:=$(LEVELDB_PATH)/build
LEVELDB_FLAGS:=-DLEVELDB_DB_PATH=\"$(LEVELDB_DB_PATH)\"  -DPROJECT_ROOT=\"$(PROJECT_ROOT)\"  -I$(LEVELDB_INCLUDE_PATH)

#============================TODO: all below============================
$(MAIN) :

$(FPTREE) : 

$(UTILITY) :

$(PALLOCATOR) : 

$(LYCSB): lycsb.cpp
	@echo "Building $@"
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

	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(LEVELDB_FLAGS) $< -o $@ -L$(LEVELDB_LIBRARY_PATH) -lleveldb -lpthread

$(CLHASH) : clhash.c $(HEADER_DIR)/utility/clhash.h
	$(CC) $(CFLAGS) -c clhash.c -I $(HEADER_DIR) -g -o $@

$(BIN):
	mkdir -p bin

all : $(ALL)

clean :
	rm -rf *.o ./bin/* ../data/*

cleand : 
	rm -rf ../data/*