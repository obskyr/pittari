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

bool compare_pixel_exact(unsigned char** pixel_1, unsigned char** pixel_2)
{
    for (int channel = 0; channel < 3; channel++) {
        if (*(*pixel_1)++ != *(*pixel_2)++) {
            *pixel_1 += 3 - channel - 1;
            *pixel_2 += 3 - channel - 1;
            return true;
        }
    }
    return false;
}

int fuzziness = 10;
bool compare_pixel_fuzzy(unsigned char** pixel_1, unsigned char** pixel_2)
{
    for (int channel = 0; channel < 3; channel++) {
        if (abs((int) *(*pixel_1)++ - (int) *(*pixel_2)++) > fuzziness) {
            *pixel_1 += 3 - channel - 1;
            *pixel_2 += 3 - channel - 1;
            return true;
        }
    }
    return false;
}

bool (*compare_pixel)(unsigned char**, unsigned char**) = compare_pixel_exact;

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

#ifdef DEBUG
    printf("== COLUMNS (width) ==\n\n");
#endif

    size_t calculated_width = determine_dimension(width, column_contrasts);

#ifdef DEBUG
    printf("== ROWS (height) ==\n\n");
#endif

    size_t calculated_height = determine_dimension(height, row_contrasts);

    double calculated_x_scale = (double) width / (double) calculated_width;
    double calculated_y_scale = (double) height / (double) calculated_height;
    double pixel_aspect_ratio = calculated_x_scale / calculated_y_scale;

#ifdef DEBUG
    printf("== RESULT (height) ==\n\n");
#endif

    printf("Original resolution: %zu x %zu\n", calculated_width, calculated_height);
    printf("Scale:               %lg x %lg\n", calculated_x_scale, calculated_y_scale);
    printf("Pixel aspect ratio:  %lg\n", pixel_aspect_ratio);

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

size_t determine_dimension(size_t contrasts_size, bool contrasts[])
{
    size_t num_runs = 0;
    size_t thinnest = SIZE_MAX;
    size_t second_thinnest = SIZE_MAX;
    size_t third_thinnest = SIZE_MAX;
    size_t run_start = 0;
    size_t i;
    for (i = 1; i < contrasts_size; i++) {
        if (contrasts[i]) {
            register_run(i - run_start, &num_runs, &thinnest, &second_thinnest, &third_thinnest);
            run_start = i;
        }
    }
    register_run(i - run_start, &num_runs, &thinnest, &second_thinnest, &third_thinnest);

#ifdef DEBUG
    printf("Thinnest: %zu\nSecond thinnest: %zu\nThird thinnest: %zu\n\n", thinnest, second_thinnest, third_thinnest);
#endif

    if (second_thinnest == SIZE_MAX) {
        // The image has completely uniform pixels in this dimension.
        return num_runs;
    } else if (second_thinnest == thinnest + 1 && third_thinnest == SIZE_MAX) {
        // The detection has succeeded in identifying every single point where
        // the image switches to a new pixel in this dimension.
        // Put another way, there were no swaths of the same exact color
        // that took up the whole width/height.
        return num_runs;
    } else {
        return determine_dimension_by_certain_delineations(contrasts_size, contrasts, thinnest);
    }
}

static inline void register_run(size_t run_length, size_t* num_runs, size_t* thinnest, size_t* second_thinnest, size_t* third_thinnest)
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

/*
    The way this algorithm currently works is by counting the number of known
    pixels and dividing that by how many pixels they take up in the scaled
    image, thus averaging out to an estimated scale based on what we do know.

    What this ignores is that a nearest-neighbor algorithm will always place
    duplicated pixels at an equal (or, y'know ± 1 since it's rounded) distance
    from each other. The algorithm could takethis into account, and attempt
    to get the exact parameters for the original nearest-neigbor scaling
    process (i.e. find the origin [center rounding up like ImageMagick {the
    sane approach}, bottom right like Paint.net, or perhaps center rounding
    down or top left {though I haven't found software that uses those yet}],
    and calculate the scale based on where known extra pixels are positioned).
    I'd imagine this would particularly help images at a non- integer scale
    that's between 1× and 2×, since that's the only case when a 2-pixel-wide
    run could either be two pixels or a widened single pixel – but that's a
    very particular case, that I'm not sure will come up much IRL. If it does,
    it may be time to improve this algorithm.
*/
size_t determine_dimension_by_certain_delineations(size_t contrasts_size, bool contrasts[], size_t thinnest)
{
    size_t num_thinnest = 0;
    size_t num_second_thinnest = 0;
    size_t run_start = 0;
    size_t i;
    for (i = 1; i < contrasts_size; i++) {
        if (contrasts[i]) {
            count_run(i - run_start, thinnest, &num_thinnest, &num_second_thinnest);
            run_start = i;
        }
    }
    count_run(i - run_start, thinnest, &num_thinnest, &num_second_thinnest);

    // We could have tallied this up in the first place instead of counting
    // the thinnest and second-thinnest pixels separately, but this is
    // easier to change to a different approach later (should it be needed)
    // and should only constitute a negligible performance hit.
    size_t certain_pixels_size = num_thinnest * thinnest + num_second_thinnest * (thinnest + 1);
    size_t num_certain_pixels = num_thinnest + num_second_thinnest;

    double determined_scale = (double) certain_pixels_size / (double) num_certain_pixels;
    return (size_t) (contrasts_size / determined_scale + 0.5);
}

static inline void count_run(size_t run_length, size_t thinnest, size_t* num_thinnest, size_t* num_second_thinnest)
{
    if (run_length == thinnest) {
        (*num_thinnest)++;
    } else if (run_length == thinnest + 1) {
        (*num_second_thinnest)++;
    }
}
