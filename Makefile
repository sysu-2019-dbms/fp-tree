CC=gcc
BUILD_DIR=build
pmem_example: src/pmem_example.c
	@mkdir -p build
	@gcc $< -lpmem -o $(BUILD_DIR)/$@

pmem_test: pmem_example
	@./$(BUILD_DIR)/$<
	@cat /pmem-fs/myfile

.PHONY: clean

clean:
	rm -rf build
