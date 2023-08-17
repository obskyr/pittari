#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stddef.h>
#include <MagickWand/MagickWand.h>

int main(int argc, char **argv);

int update_contrasts_from_wand(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand);
int update_contrasts_from_image(size_t width, bool column_contrasts[], size_t height, bool row_contrasts[], unsigned char pixels[], MagickWand* wand);
void update_column_contrasts_from_pixels(size_t width, bool column_contrasts[], size_t height, unsigned char* pixels);
void update_row_contrasts_from_pixels(size_t width, size_t height, bool row_contrasts[], unsigned char* pixels);

size_t calculate_dimension(size_t contrasts_size, bool contrasts[]);
void register_run(size_t run_length, size_t* num_runs, size_t* thinnest, size_t* second_thinnest, size_t* third_thinnest);

#endif
