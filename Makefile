.PHONY: all clean

PROGRAM_FILENAME := how-many-pixels
BUILD_DIR := build/

SOURCE_FILES := source/main.c

DEFS := 

ifeq ($(OS), Windows_NT)
	PROGRAM_FILENAME := $(PROGRAM_FILENAME).exe
	DEFS := -DWIN64
endif
PROGRAM := $(BUILD_DIR)$(PROGRAM_FILENAME)

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM)

$(PROGRAM): $(SOURCE_FILES)
	@mkdir -p $(BUILD_DIR)
	gcc -Wall $(shell /mingw64/bin/MagickWand-config --cflags) $(DEFS) $(SOURCE_FILES) -o $(PROGRAM) $(shell /mingw64/bin/MagickWand-config --ldflags)
