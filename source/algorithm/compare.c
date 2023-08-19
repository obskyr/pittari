#include "algorithm/compare.h"

#include <stdbool.h>
#include <stdlib.h>

/*
    No fuzziness. Works for images that have been scaled with a nearest
    neighbor algorithm.
*/
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

/*
    Allow some leeway in the R, G, and B values of each pixel. This is strictly
    a per-channel affair – nothing along the lines of color distance – so its
    fanciness is limited. Useful for images that appear to have been scaled
    with a nearest neighbor algorithm, but apparently haven't – such as
    "tests/254x231 fuzzy.png".
*/
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

// Setting this determines the pixel comparison algorithm used in contrast.c.
bool (*compare_pixel)(unsigned char**, unsigned char**) = compare_pixel_exact;
