.PHONY: all clean

PROGRAM_FILENAME := how-many-pixels
BUILD_DIR := build/

SOURCE_FILES := source/main.c

DEFS := 
INCLUDE := -I/usr/include
LIBRARY_DIRS := 
LIBRARIES := 

ifeq ($(OS), Windows_NT)
	PROGRAM_FILENAME := $(PROGRAM_FILENAME).exe
	DEFS := -DWIN64
	INCLUDE := $(INCLUDE) -I./argp-standalone
	LIBRARY_DIRS := $(LIBRARY_DIRS) -L./argp-standalone/build
	LIBRARIES := $(LIBRARIES) -largp
endif
PROGRAM := $(BUILD_DIR)$(PROGRAM_FILENAME)

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM)

$(PROGRAM): $(SOURCE_FILES)
	@mkdir -p $(BUILD_DIR)
	gcc -Wall $(shell MagickWand-config --cflags) $(INCLUDE) $(DEFS) $(SOURCE_FILES) -o $(PROGRAM) $(shell MagickWand-config --ldflags) $(LIBRARY_DIRS) $(LIBRARIES)

ifeq ($(OS), Windows_NT)
$(PROGRAM): ./argp-standalone/build/libargp.a

./argp-standalone/build/libargp.a:
	cd argp-standalone/ && \
	meson setup build/ && \
	cd build/ && \
	meson compile
endif
