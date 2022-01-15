.PHONY:	all loadrpx wupserver regionfree install_wup

all: out loadrpx wupserver regionfree install_wup
	@zip -FSjr sd_kernels.zip out/*

clean:
	@rm -rf out sd_kernels.zip
	@$(MAKE) --no-print-directory -C loadrpx clean
	@$(MAKE) --no-print-directory -C wupserver clean
	@$(MAKE) --no-print-directory -C regionfree clean
	@$(MAKE) --no-print-directory -C install_wup clean

out:
	@mkdir $@

loadrpx:
	@$(MAKE) --no-print-directory -C $@
	@cp $@/$@.bin out/$@.bin

wupserver:
	@$(MAKE) --no-print-directory -C $@
	@cp $@/$@.bin out/$@.bin

regionfree:
	@$(MAKE) --no-print-directory -C $@
	@cp $@/$@.bin out/$@.bin

install_wup:
	@$(MAKE) --no-print-directory -C $@
	@cp $@/$@.bin out/$@.bin
