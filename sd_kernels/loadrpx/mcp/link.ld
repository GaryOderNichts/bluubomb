OUTPUT_ARCH(arm)

SECTIONS {
    . = (0x05059900);
    .text : {
        _text_start = .;
        *(.text*);
        *(.rodata*);
        *(.data*);
    }
    _text_end = .;

    . = (0x050bc600);
    .bss : {
        _bss_start = .;
        *(.bss*);
    }
    _bss_end = .;

    /DISCARD/ : {
        *(*);
    }
}
