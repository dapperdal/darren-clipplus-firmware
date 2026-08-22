CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic -O2

BUILD := build
SOURCES := firmware/src/clipplus_ui.c firmware/src/clipplus_boot.c firmware/src/clipplus_runtime.c firmware/src/clipplus_memory.c firmware/src/clipplus_alarm.c firmware/src/clipplus_startup.c firmware/src/clipplus_board_stub.c firmware/src/clipplus_playback.c simulator/main.c
TARGET_BUILD := $(BUILD)/target
TARGET_SOURCES := $(filter-out simulator/% firmware/src/clipplus_playback.c, $(SOURCES))
TARGET_OBJECTS := $(patsubst firmware/src/%.c,$(TARGET_BUILD)/%.o,$(TARGET_SOURCES))
TARGET_ENTRY := $(TARGET_BUILD)/entry.o
TARGET_LINKER := target/as3525v2/linker.ld.in
TARGET_CFLAGS := -std=c99 -mcpu=arm926ej-s -marm -mthumb-interwork -mfloat-abi=soft -ffreestanding -fno-builtin -fno-common -ffunction-sections -fdata-sections -Os -Wall -Wextra -Werror -pedantic -Ifirmware/include

.PHONY: all clean test preflight target-audit target-compile target-link target-validate

all: $(BUILD)/clipplus-sim

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/clipplus-sim: $(SOURCES) firmware/include/clipplus_ui.h | $(BUILD)
	$(CC) $(CFLAGS) -Ifirmware/include $(SOURCES) -o $@

test: $(BUILD)/clipplus-sim
	$(BUILD)/clipplus-sim --self-test

preflight:
	@test -n "$(ORIGINAL)" || (echo "usage: make preflight ORIGINAL=/path/to/image" && exit 2)
	python3 tools/image_preflight.py "$(ORIGINAL)"

target-audit:
	python3 tools/target_build_audit.py

target-compile: $(TARGET_OBJECTS)
	@echo "target object compile: complete (not linked; not bootable)"

target-link: $(TARGET_OBJECTS) $(TARGET_ENTRY)
	arm-none-eabi-gcc -nostdlib -Wl,-Map=$(TARGET_BUILD)/clipplus-layout.map -T$(TARGET_LINKER) $(TARGET_ENTRY) $(TARGET_OBJECTS) -lgcc -o $(TARGET_BUILD)/clipplus-layout.elf
	arm-none-eabi-size $(TARGET_BUILD)/clipplus-layout.elf
	arm-none-eabi-readelf -h -S $(TARGET_BUILD)/clipplus-layout.elf
	@echo "target layout link: complete (inert entry; not bootable or flashable)"

target-validate: target-link
	python3 tools/validate_target_elf.py $(TARGET_BUILD)/clipplus-layout.elf

$(TARGET_BUILD):
	mkdir -p $(TARGET_BUILD)

$(TARGET_BUILD)/%.o: firmware/src/%.c | $(TARGET_BUILD)
	arm-none-eabi-gcc $(TARGET_CFLAGS) -c $< -o $@

$(TARGET_ENTRY): target/as3525v2/entry.S | $(TARGET_BUILD)
	arm-none-eabi-gcc $(TARGET_CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
