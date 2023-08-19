#include "algorithm/contrast.h"

#include <stdbool.h>
#include <stddef.h>
#ifdef IMAGEMAGICK_7
#include <MagickWand/MagickWand.h>
#endif
#ifdef IMAGEMAGICK_6
#include <wand/MagickWand.h>
#endif
#include "algorithm/compare.h"

int update_contrasts_from_wand(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand)
{
    MagickResetIterator(wand);
    while (MagickNextImage(wand) != MagickFalse) {
        int error = update_contrasts_from_image(width, column_contrasts, height, row_contrasts, pixels, wand);
        if (error) {return error;}
    }
    return 0;
}

int update_contrasts_from_image(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand)
{
    size_t cur_width = MagickGetImageWidth(wand);
    size_t cur_height = MagickGetImageHeight(wand);
    if (cur_width != width || cur_height != height) {return 1;}

    MagickExportImagePixels(wand, 0, 0, width, height, "RGB", CharPixel, pixels);

    update_column_contrasts_from_pixels(width, column_contrasts, height, pixels);
    update_row_contrasts_from_pixels(width, height, row_contrasts, pixels);

    return 0;
}

void update_column_contrasts_from_pixels(size_t width, bool column_contrasts[], size_t height, unsigned char* pixels)
{
    unsigned char* left_pixel = pixels;
    unsigned char* cur_pixel = pixels + 3;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 1; x < width; x++) {
            if (column_contrasts[x]) {
                left_pixel += 3;
                cur_pixel += 3;
                continue;
            }

            if (compare_pixel(&left_pixel, &cur_pixel)) {
                column_contrasts[x] = true;
            }
        }
        // Currently, left_pixel is the rightmost pixel in the previous
        // column, and cur_pixel is the leftmost pixel in the current row –
        // so we gotta skip forward once.
        left_pixel += 3;
        cur_pixel += 3;
    }
}

void update_row_contrasts_from_pixels(size_t width, size_t height, bool row_contrasts[], unsigned char* pixels)
{
    unsigned char* above_pixel = pixels;
    unsigned char* cur_pixel = pixels + 3 * width;
    for (size_t y = 1; y < height; y++) {
        if (row_contrasts[y]) {
            above_pixel += 3 * width;
            cur_pixel += 3 * width;
            continue;
        }

        for (size_t x = 0; x < width; x++) {
            if (compare_pixel(&above_pixel, &cur_pixel)) {
                above_pixel = pixels + 3 * width * y;
                cur_pixel = pixels + 3 * width * (y + 1);
                row_contrasts[y] = true;
                break;
            }
        }
    }
}
