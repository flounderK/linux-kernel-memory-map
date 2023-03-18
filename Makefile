

all: kernel_module test_code


clean: clean_kernel_module clean_test_code


test: all


kernel_module:
	cd src; $(MAKE)


test_code:
	cd testsrc; $(MAKE)


clean_kernel_module:
	cd src; $(MAKE) clean


clean_test_code:
	cd testsrc; $(MAKE) clean


.PHONY: all clean test kernel_module clean_kernel_module test_code clean_test_code
