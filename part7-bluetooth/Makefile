CFILES = $(wildcard *.c)
OFILES = $(CFILES:.c=.o)
LLVMPATH = /opt/homebrew/opt/llvm/bin
GCCFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a72+nosimd

all: clean kernel8.img

boot.o: boot.S
	$(LLVMPATH)/clang --target=aarch64-elf $(GCCFLAGS) -c boot.S -o boot.o

BCM4345C0.o : BCM4345C0.hcd
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

%.o: %.c
	$(LLVMPATH)/clang --target=aarch64-elf $(GCCFLAGS) -c $< -o $@

kernel8.img: boot.o $(OFILES) BCM4345C0.o
	$(LLVMPATH)/ld.lld -m aarch64elf -nostdlib boot.o $(OFILES) BCM4345C0.o -T link.ld -o kernel8.elf
	$(LLVMPATH)/llvm-objcopy -O binary kernel8.elf kernel8.img

clean:
	/bin/rm kernel8.elf *.o *.img > /dev/null 2> /dev/null || true
