.PHONY: all configure clean

BUILD_DIR := build

all: configure
	$(MAKE) -C $(BUILD_DIR)

configure:
	cmake -S . -B $(BUILD_DIR)

clean:
	$(MAKE) -C $(BUILD_DIR) clean
