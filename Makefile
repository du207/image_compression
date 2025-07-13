CC=gcc
CFLAGS=-I$(SRC_DIR) $(shell sdl2-config --cflags)
LDFLAGS=-lm $(shell sdl2-config --libs)

BUILD_DIR=./build
SRC_DIR=./src
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
TARGET=image_compression

ifdef DEBUG
	CFLAGS += -Wall -Wextra -g
endif

ifdef RELEASE
	CLFAGS += -O2
endif

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f sample/*.awi

run:
	$(BUILD_DIR)/$(TARGET)
