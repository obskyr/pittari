#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stddef.h>
#include <MagickWand/MagickWand.h>

int main(int argc, char **argv);

void determine_dimensions(
    size_t num_image_paths, char** image_paths,
    size_t* scaled_width, size_t* scaled_height,
    size_t* determined_width, size_t* determined_height
);

int update_contrasts_from_wand(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand);
int update_contrasts_from_image(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand);
void update_column_contrasts_from_pixels(size_t width, bool column_contrasts[], size_t height, unsigned char* pixels);
void update_row_contrasts_from_pixels(size_t width, size_t height, bool row_contrasts[], unsigned char* pixels);

size_t determine_dimension(size_t contrasts_size, bool contrasts[]);
static inline void register_run(size_t run_length, size_t* num_runs, size_t* thinnest, size_t* second_thinnest, size_t* third_thinnest);
size_t determine_dimension_by_certain_delineations(size_t contrasts_size, bool contrasts[], size_t thinnest);
static inline void count_run(size_t run_length, size_t thinnest, size_t* num_thinnest, size_t* num_second_thinnest);

void print_with_format(
    const char* format,
    size_t scaled_width, size_t scaled_height,
    size_t determined_width, size_t determined_height,
    double determined_x_scale, double determined_y_scale,
    double pixel_aspect_ratio
);

#endif
