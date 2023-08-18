#include "algorithm/compare.h"

#include <math.h>
#include <stdbool.h>

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

int compare_pixel_fuzzy_fuzziness;
bool compare_pixel_fuzzy(unsigned char** pixel_1, unsigned char** pixel_2)
{
    for (int channel = 0; channel < 3; channel++) {
        if (abs((int) *(*pixel_1)++ - (int) *(*pixel_2)++) > compare_pixel_fuzzy_fuzziness) {
            *pixel_1 += 3 - channel - 1;
            *pixel_2 += 3 - channel - 1;
            return true;
        }
    }
    return false;
}

bool (*compare_pixel)(unsigned char**, unsigned char**) = compare_pixel_exact;
