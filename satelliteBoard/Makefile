AVR_MCU := attiny13a
AVR_CC := avr-gcc
SRC_DIR := src
BUILD_DIR := build
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)
AVR_HEX := $(BUILD_DIR)/main.hex
AVR_ELF := $(BUILD_DIR)/main.elf

all: $(AVR_HEX) $(AVR_ELF)
	avr-objdump -Pmem-usage $(AVR_ELF)

flash: $(AVR_HEX)
	avrdude -c usbtiny -p $(AVR_MCU) -B 2 -U flash:w:$(AVR_HEX):i

$(AVR_HEX): $(AVR_ELF)
	avr-objcopy -j .text -j .data -O ihex $^ $@

$(AVR_ELF): $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	$(AVR_CC) -mmcu=$(AVR_MCU) $^ -o $@

%.o: %.c
	$(AVR_CC) -Wno-char-subscripts -Os -DF_CPU=8000000 -mmcu=$(AVR_MCU) -fstack-usage -c $^ -o $@

clean:
	rm -f $(wildcard $(SRC_DIR)/*.o) $(wildcard $(SRC_DIR)/*.su) $(AVR_ELF) $(AVR_HEX)


