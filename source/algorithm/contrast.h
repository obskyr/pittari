/*
    Functions that condense the rows and columns in a scaled image (or a series
    of images that were scaled identically) into an array each that signifies
    at which row/column in the scaled image a new row/column starts in the
    original 1:1 image.
*/
#ifndef CONTRAST_H
#define CONTRAST_H

#include <stdbool.h>
#include <stddef.h>
#include <MagickWand/MagickWand.h>

int update_contrasts_from_wand(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand);
int update_contrasts_from_image(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand);
void update_column_contrasts_from_pixels(size_t width, bool column_contrasts[], size_t height, unsigned char* pixels);
void update_row_contrasts_from_pixels(size_t width, size_t height, bool row_contrasts[], unsigned char* pixels);

#endif
