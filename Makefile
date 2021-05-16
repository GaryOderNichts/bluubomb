.PHONY:	all clean arm_kernel_loadfile arm_kernel_fw_launcher arm_kernel_region_free

all: bluubomb arm_kernel_loadfile arm_kernel_fw_launcher arm_kernel_region_free

clean:
	rm -f bluubomb
	@$(MAKE) --no-print-directory -C arm_kernel_loadfile clean
	@$(MAKE) --no-print-directory -C arm_kernel_fw_launcher clean
	@$(MAKE) --no-print-directory -C arm_kernel_region_free clean

bluubomb: bluubomb.c adapter.c bdaddr.c sdp.c
	gcc -std=gnu11 -Wall -o bluubomb bluubomb.c adapter.c bdaddr.c sdp.c -lbluetooth

arm_kernel_loadfile:
	@$(MAKE) -j1 --no-print-directory -C arm_kernel_loadfile

arm_kernel_fw_launcher:
	@$(MAKE) -j1 --no-print-directory -C arm_kernel_fw_launcher

arm_kernel_region_free:
	@$(MAKE) -j1 --no-print-directory -C arm_kernel_region_free
