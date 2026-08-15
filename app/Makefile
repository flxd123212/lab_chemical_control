#-----------------------------------------------------------------------
# 实验室危化品库房环境智能管控系统 - Makefile
# Target: GEC6818 (ARM Cortex-A53)
# Compiler: arm-linux-gnueabihf-gcc
#-----------------------------------------------------------------------

CROSS_COMPILE := arm-linux-gnueabihf-
CC            := $(CROSS_COMPILE)gcc
AR            := $(CROSS_COMPILE)ar
STRIP         := $(CROSS_COMPILE)strip

# Application name
TARGET := lab_chemical_control

# Source files
SRCS := main.c ui_lcd.c rfid_card.c gas_monitor.c weather.c water_sensor.c alarm.c

# Object files
OBJS := $(SRCS:.c=.o)

# Compiler flags
CFLAGS := -O2 -Wall -Wextra -Werror=implicit-function-declaration
CFLAGS += -I.

# Linker flags
LDFLAGS := -lm

#-----------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------

.PHONY: all clean distclean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@
	@echo "Build complete: $(TARGET)"
	@echo "File size:"
	ls -lh $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) *.o

distclean: clean
	rm -f $(TARGET)

# Install to target board (modify IP/path as needed)
# Usage: make install BOARD_IP=192.168.1.100
install: $(TARGET)
	@if [ -n "$(BOARD_IP)" ]; then \
		echo "Installing to board $(BOARD_IP)..."; \
		scp $(TARGET) root@$(BOARD_IP):/root/; \
		echo "Done. Run 'ssh root@$(BOARD_IP) ./$(TARGET)'"; \
	else \
		echo "Usage: make install BOARD_IP=<ip_address>"; \
	fi

# Show cross-compiler info
info:
	@echo "Cross compiler: $(CC)"
	@$(CC) --version | head -1

# Help
help:
	@echo "Build targets:"
	@echo "  make          - Build application"
	@echo "  make clean    - Remove object files"
	@echo "  make distclean - Remove all build artifacts"
	@echo "  make install BOARD_IP=<ip> - Build and copy to board"
	@echo "  make info     - Show compiler info"
