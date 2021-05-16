OUTPUT_ARCH(arm)

SECTIONS
{
    . = (0x08135000);

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
    .bss : {
        *(.bss*)
        *(COMMON*)
    }

    /DISCARD/ : {
        *(*);
    }
}

