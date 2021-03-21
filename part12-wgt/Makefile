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

bin/wgt2blk.o: bin/wgt2.blk
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

bin/lettersspr.o: bin/letters.spr
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

bin/breakspr.o: bin/break.spr
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

bin/spacespr.o: bin/space.spr
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

bin/invaderspr.o: bin/invader.spr
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

bin/mousespr.o: bin/mouse.spr
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

%.o: %.c
	$(LLVMPATH)/clang --target=aarch64-elf $(CLANGFLAGS) -c $< -o $@

kernel8.img: boot/boot.o $(OFILES) bin/wgt1pal.o bin/wgt1blk.o bin/wgt2blk.o bin/lettersspr.o bin/spacespr.o bin/invaderspr.o bin/mousespr.o bin/breakspr.o
	$(LLVMPATH)/ld.lld -m aarch64elf -nostdlib boot/boot.o $(OFILES) bin/wgt1pal.o bin/wgt1blk.o bin/wgt2blk.o bin/lettersspr.o bin/spacespr.o bin/invaderspr.o bin/mousespr.o bin/breakspr.o -T boot/link.ld -o kernel8.elf
	$(LLVMPATH)/llvm-objcopy -O binary kernel8.elf kernel8.img

clean:
	/bin/rm kernel8.elf *.o bin/*.o boot/*.o lib/*.o *.img > /dev/null 2> /dev/null || true
