.PHONY:	all clean arm_kernel arm_user sd_kernels

all: arm_user arm_kernel bluubomb sd_kernels
	@echo All done!

clean:
	@$(MAKE) --no-print-directory -C arm_user clean
	@$(MAKE) --no-print-directory -C arm_kernel clean
	rm -f bluubomb
	@$(MAKE) -j1 --no-print-directory -C sd_kernels clean

arm_user:
	@$(MAKE) -j1 --no-print-directory -C arm_user

arm_kernel:
	@$(MAKE) -j1 --no-print-directory -C arm_kernel

bluubomb: bluubomb.c adapter.c bdaddr.c sdp.c
	gcc -std=gnu11 -Wall -o bluubomb bluubomb.c adapter.c bdaddr.c sdp.c -lbluetooth

sd_kernels:
	@echo Building SD kernels...
	@$(MAKE) -j1 --no-print-directory -C sd_kernels
