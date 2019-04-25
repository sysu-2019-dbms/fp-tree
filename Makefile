CC:=gcc
CXX:=g++
BUILD_DIR:=build
CFLAGS:=-g
PROJECT_ROOT:=$(shell pwd)
CXXFLAGS:=-g -Wall -std=c++11

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
