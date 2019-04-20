CC=gcc
BUILD_DIR=build
CFLAGS=-g
pmem_example: src/pmem_example.c
	@echo "Building $@"
	@mkdir -p build
	@gcc $(CFLAGS) $< -lpmem -o $(BUILD_DIR)/$@

pmem_test: pmem_example
	@./$(BUILD_DIR)/$<
	@cat /pmem-fs/myfile

.PHONY: clean

clean:
	rm -rf build
