
SRC_DIR := src
VENDOR_DIR := vendor
VENDOR_DIR := vendor
INCLUDE_DIR := include
BUILD_DIR := build
LIB_DIR := $(BUILD_DIR)/libs
ASSET_DIR := assets

EXE_NAME := main

SERVER ?= true

SRC_WEB_DIR = $(SRC_DIR)/website
SRC_FILES := $(shell find $(SRC_DIR) -type f -name '*.c')
SRC_FILES += $(shell find $(VENDOR_DIR) -type f -name '*.c')

# Remove both entry points
SRC_FILES := $(filter-out $(SRC_DIR)/cli.c, $(SRC_FILES))
SRC_FILES := $(filter-out $(SRC_DIR)/server.c, $(SRC_FILES))

# Website folder (Needs to be compiled with EMCC)
SRC_FILES := $(filter-out $(SRC_WEB_DIR)/%, $(SRC_FILES))

ifeq ($(SERVER),true)
	SRC_FILES += $(SRC_DIR)/server.c
else 
	SRC_FILES += $(SRC_DIR)/cli.c 
endif

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/obj/%.o,$(SRC_FILES))

EMCC := emcc 
MAKE := make

RAYLIB_REPO := https://github.com/raysan5/raylib.git
RAYLIB_COMMIT := 180c3c13ba3ab6ca10b43ced8464aed0db7df566
RAYLIB_WEB_LIB := libraylib.web.a
RAYLIB_DIR := $(BUILD_DIR)/raylib

INCLUDE_PATHS := -I$(INCLUDE_DIR) -I$(VENDOR_DIR)
OPTIMISATIONS := -Wall -Wextra -pedantic -O2 -fsanitize=address
DEFINIES := -DBUILD_DIR=\"$(BUILD_DIR)\" -DASSETS_DIR=\"$(ASSET_DIR)\"

EMCC_SRC_FILES := $(shell find $(SRC_WEB_DIR) -type f -name '*.c')
EMCC_RAYLIB_INCLUDE_PATH := $(RAYLIB_DIR)/src
EMCC_CFLAGS := $(OPTIMISATIONS) $(INCLUDE_PATHS) -I$(EMCC_RAYLIB_INCLUDE_PATH) $(DEFINIES) -s TOTAL_STACK=16777216 -s USE_GLFW=3 -s ASYNCIFY -s FORCE_FILESYSTEM=1 -s INITIAL_MEMORY=134217728 -s ALLOW_MEMORY_GROWTH=1
EMCC_LDFLAGS := -L $(LIB_DIR) -lraylib.web
EMCC_TEMPLATE := $(SRC_WEB_DIR)/template.html

CC := gcc
CFLAGS := $(OPTIMISATIONS) $(INCLUDE_PATHS) $(DEFINIES)
LDFLAGS := -lraylib -lmupdf -lleptonica -ltesseract -lcurl -lm

.PHONY: app clean website

all: folders website app

folders:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/obj
	mkdir -p $(BUILD_DIR)/obj/containers
	mkdir -p $(BUILD_DIR)/website
	mkdir -p $(LIB_DIR)

$(RAYLIB_WEB_LIB): 
	if [ ! -d "$(RAYLIB_DIR)" ]; then \
		git clone --no-checkout $(RAYLIB_REPO) $(RAYLIB_DIR); \
	fi
	cd $(RAYLIB_DIR) && git fetch --depth 1 origin $(RAYLIB_COMMIT)
	cd $(RAYLIB_DIR) && git checkout $(RAYLIB_COMMIT)
	$(MAKE) -C $(RAYLIB_DIR)/src PLATFORM=PLATFORM_WEB
	cp $(RAYLIB_DIR)/src/$(RAYLIB_WEB_LIB) $(LIB_DIR)/$(RAYLIB_WEB_LIB)

website: $(RAYLIB_WEB_LIB)
	$(EMCC) $(EMCC_SRC_FILES) \
		-o $(BUILD_DIR)/website/index.html \
		$(EMCC_CFLAGS) \
		$(EMCC_LDFLAGS) \
		--shell-file $(EMCC_TEMPLATE) \
		--preload-file $(ASSET_DIR)

app: $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/website/$(EXE_NAME) $(LDFLAGS)

run:
	cd $(BUILD_DIR)/website 
	./$(EXE_NAME)

$(BUILD_DIR)/obj/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)


