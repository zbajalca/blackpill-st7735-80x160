TARGET = st7735_dual_panel
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
LCD_PANEL ?= 1

CFLAGS = -mcpu=cortex-m4 -mthumb -Os -ffreestanding -fno-builtin \
         -ffunction-sections -fdata-sections -Wall -Wextra \
         -DLCD_PANEL=$(LCD_PANEL)
LDFLAGS = -mcpu=cortex-m4 -mthumb -nostdlib -T stm32f411xe_flash.ld \
          -Wl,--gc-sections -Wl,-Map=$(TARGET).map

SRCS = main.c startup_stm32f411xe.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET).elf $(TARGET).bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -lgcc -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).elf
	openocd -f openocd.cfg -c "program $(TARGET).elf verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map

.PHONY: all clean flash
