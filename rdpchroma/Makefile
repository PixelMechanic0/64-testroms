BUILD_DIR = build
include $(N64_INST)/include/n64.mk

PROJECT_NAME = rdpchroma

SRC = $(wildcard src/*.c)
OBJS = $(SRC:%.c=$(BUILD_DIR)/%.o)

N64_CFLAGS += -Wall -Wextra -Werror -O2 -Isrc

all: $(PROJECT_NAME).z64

$(PROJECT_NAME).z64: N64_ROM_TITLE = "RDP Chroma Key 3"

$(BUILD_DIR)/$(PROJECT_NAME).elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) $(PROJECT_NAME).z64

-include $(wildcard $(BUILD_DIR)/*.d)
-include $(wildcard $(BUILD_DIR)/src/*.d)

.PHONY: all clean
