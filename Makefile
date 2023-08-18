.PHONY: all clean

PROGRAM_FILENAME := how-many-pixels
SOURCE_DIR := source
BUILD_DIR := build

SOURCE_FILES := $(shell find "$(SOURCE_DIR)/" -type f -name "*.c")

DEFS := 
INCLUDE := -I/usr/include
LIBRARY_DIRS := 
LIBRARIES := 

# TODO: Find ImageMagick directories dynamically.
MAGICK_CODERS_PATH := /mingw64/lib/ImageMagick-7.1.1/modules-Q16HDRI/coders

ifeq ($(OS), Windows_NT)
	PROGRAM_FILENAME := $(PROGRAM_FILENAME).exe
	DEFS := -DWIN64
	INCLUDE := $(INCLUDE) -I./argp-standalone
	LIBRARY_DIRS := $(LIBRARY_DIRS) -L./argp-standalone/build
	LIBRARIES := $(LIBRARIES) -largp

	MAGICK_CODERS_DIR := $(BUILD_DIR)/coders
	MAGICK_CODERS_NEEDED := bmp gif jpeg png tiff webp
	MAGICK_CODERS_NEEDED := $(patsubst %, %.dll, $(MAGICK_CODERS_NEEDED)) $(patsubst %, %.la, $(MAGICK_CODERS_NEEDED))
	MAGICK_CODERS_NEEDED := $(patsubst %, $(MAGICK_CODERS_DIR)/%, $(MAGICK_CODERS_NEEDED))
endif
PROGRAM := $(BUILD_DIR)/$(PROGRAM_FILENAME)

all: $(PROGRAM) $(MAGICK_CODERS_NEEDED)

clean:
	rm -f $(PROGRAM)

$(PROGRAM): $(SOURCE_FILES)
	@mkdir -p $(BUILD_DIR)
	gcc -Wall $(shell MagickWand-config --cflags) -I$(SOURCE_DIR) $(INCLUDE) $(DEFS) $(SOURCE_FILES) -o $(PROGRAM) $(shell MagickWand-config --ldflags) $(LIBRARY_DIRS) $(LIBRARIES)

ifeq ($(OS), Windows_NT)
$(PROGRAM): ./argp-standalone/build/libargp.a

./argp-standalone/build/libargp.a:
	cd argp-standalone/ && \
	meson setup build/ && \
	cd build/ && \
	meson compile

$(MAGICK_CODERS_DIR)/%: $(MAGICK_CODERS_PATH)/%
	@mkdir -p $(@D)
	cp "$^" "$(@D)/"
endif
