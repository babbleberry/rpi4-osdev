CFILES = $(wildcard *.c lib/*.c)
OFILES = $(CFILES:.c=.o)
LLVMPATH = /opt/homebrew/opt/llvm/bin
CLANGFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a72+nosimd

all: clean kernel8.img

boot/boot.o: boot/boot.S
	$(LLVMPATH)/clang --target=aarch64-elf $(CLANGFLAGS) -c $< -o $@

bin/wgt1pal.o: bin/wgt1.pal
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

bin/wgt1blk.o: bin/wgt1.blk
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

%.o: %.c
	$(LLVMPATH)/clang --target=aarch64-elf $(CLANGFLAGS) -c $< -o $@

kernel8.img: boot/boot.o $(OFILES) bin/wgt1pal.o bin/wgt1blk.o
	$(LLVMPATH)/ld.lld -m aarch64elf -nostdlib boot/boot.o $(OFILES) bin/wgt1pal.o bin/wgt1blk.o -T boot/link.ld -o kernel8.elf
	$(LLVMPATH)/llvm-objcopy -O binary kernel8.elf kernel8.img

clean:
	/bin/rm kernel8.elf *.o bin/*.o boot/*.o lib/*.o *.img > /dev/null 2> /dev/null || true
