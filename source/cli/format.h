#ifndef FORMAT_H
#define FORMAT_H

#include <stddef.h>

void print_with_format(
    const char* format,
    size_t scaled_width, size_t scaled_height,
    size_t determined_width, size_t determined_height,
    double determined_x_scale, double determined_y_scale,
    double pixel_aspect_ratio
);

#endif
