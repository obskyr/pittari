#include "algorithm/dimensions.h"

#include <stddef.h>
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif

static inline void register_run(size_t run_length, size_t* num_runs, size_t* thinnest, size_t *thickest);
static inline void count_run(size_t run_length, size_t thinnest, size_t* num_certain_pixels, size_t* certain_pixels_size);

int nearest_neighbor_max_variation = 1;

/*
    Determine one dimension (i.e. width or height) based on a previously
    determined array of contrasts.
*/
size_t determine_dimension(size_t contrasts_size, bool contrasts[])
{
    size_t num_runs = 0;
    size_t thinnest = SIZE_MAX;
    size_t thickest = 0;
    size_t run_start = 0;
    size_t i;
    for (i = 1; i < contrasts_size; i++) {
        if (contrasts[i]) {
            register_run(i - run_start, &num_runs, &thinnest, &thickest);
            run_start = i;
        }
    }
    register_run(i - run_start, &num_runs, &thinnest, &thickest);

#ifdef DEBUG
    printf("Thinnest: %zu\nThickest: %zu\n\n", thinnest, thickest);
#endif

    if (thickest - thinnest <= nearest_neighbor_max_variation) {
        // The detection has succeeded in identifying every single point where
        // the image switches to a new pixel in this dimension.
        // Put another way, there were no swaths of the same exact color
        // that took up the whole width/height.
        return num_runs;
    } else {
        return determine_dimension_by_certain_delineations(contrasts_size, contrasts, thinnest);
    }
}

static inline void register_run(size_t run_length, size_t* num_runs, size_t* thinnest, size_t *thickest)
{
    (*num_runs)++;
    if (run_length < *thinnest) {
        *thinnest = run_length;
    }
    if (run_length > *thickest) {
        *thickest = run_length;
    }
}

/*
    Determine one dimension (i.e. width or height) based on a previously
    determined array of contrasts. There may be swaths of uncertainty in the
    array (that is to say, longer runs of identical pixels, where it was not
    possible to determine where the rows/columns start and end).

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
    size_t num_certain_pixels = 0;
    size_t certain_pixels_size = 0;
    size_t run_start = 0;
    size_t i;
    for (i = 1; i < contrasts_size; i++) {
        if (contrasts[i]) {
            count_run(i - run_start, thinnest, &num_certain_pixels, &certain_pixels_size);
            run_start = i;
        }
    }
    count_run(i - run_start, thinnest, &num_certain_pixels, &certain_pixels_size);

    double determined_scale = (double) certain_pixels_size / (double) num_certain_pixels;
    return (size_t) (contrasts_size / determined_scale + 0.5);
}

static inline void count_run(size_t run_length, size_t thinnest, size_t* num_certain_pixels, size_t* certain_pixels_size)
{
    if (run_length - thinnest <= nearest_neighbor_max_variation) {
        (*num_certain_pixels)++;
        (*certain_pixels_size) += run_length;
    }
}
