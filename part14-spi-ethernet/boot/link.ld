SECTIONS
{
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
        __bss_size = (__bss_end - __bss_start)>>3;
    }
    .cpu1Stack :
    {
        . = ALIGN(16);
        __stack_start  = .;
        . = . + 512;
        __cpu1_stack  = .;
    }
    .cpu2Stack :
    {
        . = . + 512;
        __cpu2_stack  = .;
    }
    .cpu3Stack :
    {
        . = . + 512;
        __cpu3_stack  = .;
    }
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
