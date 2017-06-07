#
# Makefile 
# Copyright (c) 2017 João Baptista de Paula e Silva
# este código está sob a licença MIT
#

#
# toolchain
#
CC      := avr-gcc
CXX     := avr-g++
AS      := avr-gcc
LD      := avr-g++
OBJCOPY := avr-objcopy
OBJDUMP := avr-objdump
AVRDUDE := avrdude

#
# platform configs
#
DEVICE     := atmega328p
CLOCK      := 16000000
PROGRAMMER := stk500v1
BAUD       := 19200
LFUSE      := 0xD7
HFUSE      := 0xD1
EFUSE      := 0xFC

#
# compiling configs
#
REGISTERS := r3 r4 r5 r6 r7
OPTRULE   := -O3 -fweb -frename-registers -flto -fno-fat-lto-objects
COMFLAGS  := -MMD -mmcu=$(DEVICE) -DF_CPU=$(CLOCK)UL $(addprefix --fixed-,$(REGISTERS))
CCPPFLAGS := $(COMFLAGS) $(OPTRULE) -Wall -ffunction-sections -fdata-sections -Wno-main -Wno-volatile-register-var
CFLAGS    := $(CCPPFLAGS) -std=gnu11
CPPFLAGS  := $(CCPPFLAGS) -std=gnu++1z -fpermissive -fno-exceptions -fno-threadsafe-statics
ASMFLAGS  := $(COMFLAGS) -x assembler-with-cpp
LDFLAGS   := $(OPTRULE) -fuse-linker-plugin $(addprefix --fixed-,$(REGISTERS)) -Wall -Wl,--gc-sections -mmcu=$(DEVICE)

#
# files to be compiled
#
SOURCES  := $(wildcard *.c *.cpp *.S)
OBJECTS  := $(SOURCES:=.o)
DEPENDS  := $(OBJECTS:.o=.d)

out.hex: out.elf

#
# compilation rules (finally!)
# Makefile is included as a requisite for convenience - modifying
# the Makefile will make all sources rebuild
#
-include $(DEPENDS)

%.c.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

%.cpp.o: %.cpp Makefile
	$(CXX) $(CPPFLAGS) -c -o $@ $<

%.S.o: %.S Makefile
	$(AS) $(ASMFLAGS) -c -o $@ $<

#
# linking rule
#
out.elf: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^
	@./size.py

out.hex: out.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

.PHONY: dump upload setfuses clean

#
# dump rule
#
dump: out.elf
	$(OBJDUMP) -D -m avr5 out.elf > avrdisasm.txt

#
# upload and setfuses rules
#
upload: out.hex
	$(AVRDUDE) -p$(DEVICE) -c$(PROGRAMMER) -b$(BAUD) -P$(PORT) -Uflash:w:out.hex:i

setfuses:
	$(AVRDUDE) -p$(DEVICE) -c$(PROGRAMMER) -b$(BAUD) -P$(PORT) -Ulfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -Uefuse:w:$(EFUSE):m -Ulock:w:0x3F:m 

#
# clean rule
#
clean:
	rm -rf *.o *.d out.elf out.hex


