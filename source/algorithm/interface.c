#include "algorithm/interface.h"

#include <stdbool.h>
#include <stddef.h>
#ifdef IMAGEMAGICK_7
#include <MagickWand/MagickWand.h>
#endif
#ifdef IMAGEMAGICK_6
#include <wand/MagickWand.h>
#endif
#include "algorithm/contrast.h"
#include "algorithm/dimensions.h"

#define ThrowWandException(wand) \
{ \
  char* description; \
  ExceptionType severity; \
  description = MagickGetException(wand, &severity); \
  (void) fprintf(stderr, "%s %s %lu %s\n", GetMagickModule(), description); \
  description = (char*) MagickRelinquishMemory(description); \
  exit(-1); \
}

void determine_dimensions(
    size_t num_image_paths, char** image_paths,
    size_t* scaled_width, size_t* scaled_height,
    size_t* determined_width, size_t* determined_height
)
{
    MagickWandGenesis();

    MagickWand* wand = NewMagickWand();
    MagickBooleanType status = MagickReadImage(wand, image_paths[0]);
    if (status == MagickFalse) {ThrowWandException(wand);}
    MagickResetIterator(wand);
    MagickNextImage(wand);
    *scaled_width = MagickGetImageWidth(wand);
    *scaled_height = MagickGetImageHeight(wand);
    unsigned char* pixels = (unsigned char*) malloc(*scaled_width * *scaled_height * 3 * sizeof(unsigned char));
    bool* column_contrasts = (bool*) calloc(*scaled_width, sizeof(bool));
    bool* row_contrasts = (bool*) calloc(*scaled_height, sizeof(bool));
    // Since the leftmost / top pixel in a scaled image always starts
    // a new pixel in the source image.
    column_contrasts[0] = true;
    row_contrasts[0] = true;

    update_contrasts_from_wand(*scaled_width, column_contrasts, *scaled_height, row_contrasts, pixels, wand);

    wand = DestroyMagickWand(wand);

    for (size_t i = 1; i < num_image_paths; i++) {
        char* cur_path = image_paths[i];
        wand = NewMagickWand();
        MagickBooleanType status = MagickReadImage(wand, cur_path);
        if (status == MagickFalse) {ThrowWandException(wand);}

        int error = update_contrasts_from_wand(*scaled_width, column_contrasts, *scaled_height, row_contrasts, pixels, wand);
        if (error) {
            fprintf(stderr, "ERROR: Screenshots not of the same resolution (\"%s\" differs).", image_paths[i]);
            exit(-1);
        }

        wand = DestroyMagickWand(wand);
    }

#ifdef DEBUG
    printf("== COLUMNS (width) ==\n\n");
#endif

    *determined_width = determine_dimension(*scaled_width, column_contrasts);

#ifdef DEBUG
    printf("== ROWS (height) ==\n\n");
#endif

    *determined_height = determine_dimension(*scaled_height, row_contrasts);

    free(pixels);
    free(column_contrasts);
    free(row_contrasts);

    MagickWandTerminus();
}
