#include "main.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <MagickWand/MagickWand.h>

#define ThrowWandException(wand) \
{ \
  char* description; \
  ExceptionType severity; \
  description = MagickGetException(wand, &severity); \
  (void) fprintf(stderr, "%s %s %lu %s\n", GetMagickModule(), description); \
  description = (char*) MagickRelinquishMemory(description); \
  exit(-1); \
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        fprintf(
            stdout,
            "Usage: %s <screenshot images...>\n"
            "\n"
            "TODO: Description of the program's operation here.",
            argv[0]
        );
        exit(0);
    }

    MagickWandGenesis();

    MagickWand* wand = NewMagickWand();
    MagickBooleanType status = MagickReadImage(wand, argv[1]);
    if (status == MagickFalse) {ThrowWandException(wand);}
    MagickResetIterator(wand);
    MagickNextImage(wand);
    size_t width = MagickGetImageWidth(wand);
    size_t height = MagickGetImageHeight(wand);
    unsigned char* pixels = (unsigned char*) malloc(width * height * 3 * sizeof(unsigned char));
    bool* column_contrasts = (bool*) calloc(width, sizeof(bool));
    bool* row_contrasts = (bool*) calloc(height, sizeof(bool));
    // Since the leftmost / top pixel in a scaled image always starts a new
    // pixel in the source image.
    column_contrasts[0] = true;
    row_contrasts[0] = true;

    update_contrasts_from_wand(width, column_contrasts, height, row_contrasts, pixels, wand);

    wand = DestroyMagickWand(wand);

    for (size_t i = 2; i < argc; i++) {
        char* cur_path = argv[i];
        wand = NewMagickWand();
        MagickBooleanType status = MagickReadImage(wand, cur_path);
        if (status == MagickFalse) {ThrowWandException(wand);}

        int error = update_contrasts_from_wand(width, column_contrasts, height, row_contrasts, pixels, wand);
        if (error) {
            fprintf(stderr, "ERROR: Screenshots not of the same resolution (\"%s\" differs).", argv[1]);
            exit(-1);
        }

        wand = DestroyMagickWand(wand);
    }

    size_t calculated_width = calculate_dimension(width, column_contrasts);
    size_t calculated_height = calculate_dimension(height, row_contrasts);

    printf("%zux%zu", calculated_width, calculated_height);

    // Write the image then destroy it.
    // status = MagickWriteImages(wand,argv[2],MagickTrue);
    // if (status == MagickFalse)
    //     ThrowWandException(wand);
    MagickWandTerminus();
    return(0);
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

int update_contrasts_from_wand(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand)
{
    MagickResetIterator(wand);
    while (MagickNextImage(wand) != MagickFalse) {
        int error = update_contrasts_from_image(width, column_contrasts, height, row_contrasts, pixels, wand);
        if (error) {return error;}
    }
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

            for (int channel = 0; channel < 3; channel++) {
                if (*left_pixel++ != *cur_pixel++) {
                    left_pixel += 3 - channel - 1;
                    cur_pixel += 3 - channel - 1;
                    column_contrasts[x] = true;
                    break;
                }
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
            for (int channel = 0; channel < 3; channel++) {
                if (*above_pixel++ != *cur_pixel++) {
                    above_pixel = pixels + 3 * width * y;
                    cur_pixel = pixels + 3 * width * (y + 1);
                    row_contrasts[y] = true;
                    goto next_row;
                }
            }
        }
        next_row:
    }
}

size_t calculate_dimension(size_t contrasts_size, bool contrasts[])
{
    size_t num_runs = 0;
    size_t thinnest = SIZE_MAX;
    size_t second_thinnest = SIZE_MAX;
    size_t third_thinnest = SIZE_MAX;
    size_t run_start = 0;
    size_t i;
    for (i = 1; i < contrasts_size; i++) {
        bool contrast = contrasts[i];
        if (contrast) {
            if (i - run_start == 4) {printf("four at: %zu\n", i);}
            register_run(i - run_start, &num_runs, &thinnest, &second_thinnest, &third_thinnest);
            run_start = i;
        }
    }
    register_run(i - run_start, &num_runs, &thinnest, &second_thinnest, &third_thinnest);
    printf("Thinnest: %zu\nSecond thinnest: %zu\nThird thinnest: %zu\n", thinnest, second_thinnest, third_thinnest);

    if (second_thinnest == SIZE_MAX) {
        // The image has completely uniform pixels in this dimension.
        return num_runs;
    } else if (second_thinnest == thinnest + 1 && third_thinnest == SIZE_MAX) {
        // The detection has succeeded in identifying every single point where
        // the image switches to a new pixel in this dimension. Put another way,
        // there were no swaths of the same exact color that took up the whole
        // width/height.
        return num_runs;
    } else {
        // TODO: Implement this.
        fprintf(stderr, "Not implemented.\n");
        return 0;
    }
}

void register_run(size_t run_length, size_t* num_runs, size_t* thinnest, size_t* second_thinnest, size_t* third_thinnest)
{
    (*num_runs)++;
    if (run_length < *thinnest) {
        *third_thinnest = *second_thinnest;
        *second_thinnest = *thinnest;
        *thinnest = run_length;
    } else if (run_length > *thinnest && run_length < *second_thinnest) {
        *third_thinnest = *second_thinnest;
        *second_thinnest = run_length;
    } else if (run_length > *second_thinnest && run_length < *third_thinnest) {
        *third_thinnest = run_length;
    }
}
