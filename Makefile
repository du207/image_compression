CC=gcc
CFLAGS=-I$(SRC_DIR)
LDFLAGS=-lm

BUILD_DIR=./build
SRC_DIR=./src
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
TARGET=image_compression

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -r $(BUILD_DIR)

run:
	$(BUILD_DIR)/$(TARGET)
