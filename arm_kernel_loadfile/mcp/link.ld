OUTPUT_ARCH(arm)

SECTIONS
{
    .text 0x05059900 : {
        _text_start = .;
        *(.text*)
        *(.rodata*)
    }
    _text_end = .;

    /DISCARD/ : {
        *(*);
    }
}
