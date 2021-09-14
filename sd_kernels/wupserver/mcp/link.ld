OUTPUT_ARCH(arm)

SECTIONS {
    . = (0x05116000);
    .text : {
        _text_start = .;
        *(.text*);
        *(.rodata*);
        *(.data*);
    }
    _text_end = .;

    . = (0x050bd000);
    .bss : {
        _bss_start = .;
        *(.bss*);
    }
    _bss_end = .;

    /DISCARD/ : {
        *(*);
    }
}
