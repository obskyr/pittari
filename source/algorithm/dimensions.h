/*
    Determine a single dimension at a time based on a contrast array.
*/
#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#include <stdbool.h>
#include <stddef.h>

extern int nearest_neighbor_max_variation;

size_t determine_dimension(size_t contrasts_size, bool contrasts[]);
size_t determine_dimension_by_certain_delineations(size_t contrasts_size, bool contrasts[], size_t thinnest);

#endif
