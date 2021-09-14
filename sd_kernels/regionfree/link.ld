OUTPUT_ARCH(arm)

SECTIONS {
    . = (0x08135400);

    .text : {
        crt0.o(.init);
        *(.text*);
    }
    .rodata : {
        *(.rodata*);
    }
    .data : {
        *(.data*);
    }
    PROVIDE(__bss_start = .);
    .bss : {
        *(.bss*);
    }
    PROVIDE(__bss_end = .);

    /DISCARD/ : {
        *(*);
    }
}

