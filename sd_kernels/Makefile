.PHONY:	all loadrpx wupserver regionfree

all: out loadrpx wupserver regionfree
	@zip -FSjr sd_kernels.zip out/*

clean:
	@rm -rf out sd_kernels.zip
	@$(MAKE) --no-print-directory -C loadrpx clean
	@$(MAKE) --no-print-directory -C wupserver clean
	@$(MAKE) --no-print-directory -C regionfree clean

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
