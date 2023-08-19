.PHONY: all clean

PROGRAM_FILENAME := how-many-pixels
SOURCE_DIR := source
BUILD_DIR := build

SOURCE_FILES := $(shell find "$(SOURCE_DIR)/" -type f -name "*.c")

DEFS := 
INCLUDE := -I/usr/include
LIBRARY_DIRS := 
LIBRARIES := 

MAGICK_CODERS_PATH := $(shell tools/find-imagemagick.sh coders)
MAGICKWAND_CONFIG := $(shell tools/find-imagemagick.sh MagickWand-config)

ifeq ($(OS), Windows_NT)
	PROGRAM_FILENAME := $(PROGRAM_FILENAME).exe
	DEFS := $(DEFS) -DWIN64 -DIMAGEMAGICK_7
	INCLUDE := $(INCLUDE) -I./argp-standalone
	LIBRARY_DIRS := $(LIBRARY_DIRS) -L./argp-standalone/build
	LIBRARIES := $(LIBRARIES) -largp

	MAGICK_CODERS_DIR := $(BUILD_DIR)/coders
	MAGICK_CODERS_NEEDED := bmp gif jpeg png tiff webp
	MAGICK_CODERS_NEEDED := $(patsubst %, %.dll, $(MAGICK_CODERS_NEEDED)) $(patsubst %, %.la, $(MAGICK_CODERS_NEEDED))
	MAGICK_CODERS_NEEDED := $(patsubst %, $(MAGICK_CODERS_DIR)/%, $(MAGICK_CODERS_NEEDED))
else
# Currently, it's hard-coded so that Linux uses ImageMagick 6.
# This can be improved to actually try to detect the version of ImageMagick
# installed… if anyone ever at all wants that.
	DEFS := $(DEFS) -DIMAGEMAGICK_6
endif
PROGRAM := $(BUILD_DIR)/$(PROGRAM_FILENAME)

all: $(PROGRAM) $(MAGICK_CODERS_NEEDED)

clean:
	rm -f $(PROGRAM)

$(PROGRAM): $(SOURCE_FILES)
	@mkdir -p $(BUILD_DIR)
	gcc -Wall $(shell $(MAGICKWAND_CONFIG) --cflags) -I$(SOURCE_DIR) $(INCLUDE) $(DEFS) $(SOURCE_FILES) -o $(PROGRAM) $(shell $(MAGICKWAND_CONFIG) --ldflags) $(LIBRARY_DIRS) $(LIBRARIES)

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
