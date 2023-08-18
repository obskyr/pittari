/*
    Functions for comparing two pixels to determine whether the second pixel
    should be deemed to start a new row/column in the image at 1:1 scale.
*/
#ifndef COMPARE_H
#define COMPARE_H

#include <stdbool.h>

bool compare_pixel_exact(unsigned char** pixel_1, unsigned char** pixel_2);
bool compare_pixel_fuzzy(unsigned char** pixel_1, unsigned char** pixel_2);
extern bool (*compare_pixel)(unsigned char**, unsigned char**);

extern int compare_pixel_fuzzy_fuzziness;

#endif
