OUTPUT_ARCH(arm)

SECTIONS {
    . = (0x11f85800);

    .text : {
        crt0.o(.init)
        *(.text*);
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
    /* .bss : {
        *(.bss*)
    } */

    /DISCARD/ : {
        *(*);
    }
}
