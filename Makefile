pmem_example: src/pmem_example.c
	@gcc $< -lpmem -o $@

pmem_test: pmem_example
	@./$<
	@cat /pmem-fs/myfile
