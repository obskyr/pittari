/*
    The public interface for the unscaling algorithm.
*/
#ifndef INTERFACE_H
#define INTERFACE_H

#include <stddef.h>

void determine_dimensions(
    size_t num_image_paths, char** image_paths,
    size_t* scaled_width, size_t* scaled_height,
    size_t* determined_width, size_t* determined_height
);

#endif
