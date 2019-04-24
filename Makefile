CC:=gcc
CXX:=g++
BUILD_DIR:=build
CFLAGS:=-g
PROJECT_ROOT:=$(shell pwd)
LEVELDB_DB_PATH:=/tmp/leveldb
LEVELDB_PATH:=$(PROJECT_ROOT)/leveldb
LEVELDB_INCLUDE_PATH:=$(LEVELDB_PATH)/include
LEVELDB_LIBRARY_PATH:=$(LEVELDB_PATH)/build

CXXFLAGS:=-g -Wall -std=c++11 -DPROJECT_ROOT=\"$(PROJECT_ROOT)\" 
LEVELDB_FLAGS:=-DLEVELDB_DB_PATH=\"$(LEVELDB_DB_PATH)\" -I$(LEVELDB_INCLUDE_PATH)
LIBRARY:=-lleveldb -lpthread

lycsb: src/lycsb.cpp
	@echo "Building $@"
	@if ! [ -d "$(PROJECT_ROOT)/leveldb" ] || ! [ -f "$(PROJECT_ROOT)/leveldb/build/libleveldb.a" ]; then \
    echo "leveldb not cloned"; \
		git submodule init; \
		git submodule update; \
		cd leveldb; \
		echo "building leveldb"; \
		mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .; \
		cd $(PROJECT_ROOT); \
	fi

	@mkdir -p build
	
	$(CXX) $(CXXFLAGS) $(LEVELDB_FLAGS) $< -o $(BUILD_DIR)/$@ -L$(LEVELDB_LIBRARY_PATH) $(LIBRARY)

pmem_example: src/pmem_example.c
	@echo "Building $@"
	@mkdir -p build
	@$(CC) $(CFLAGS) $< -lpmem -o $(BUILD_DIR)/$@

pmem_test: pmem_example
	@./$(BUILD_DIR)/$<
	@cat /pmem-fs/myfile

.PHONY: clean

clean:
	rm -rf build
