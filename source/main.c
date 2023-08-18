#include "main.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN64
#include <Windows.h>
#endif

#include <argp.h>
#include <MagickWand/MagickWand.h>

#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

// When --inexact is used. By default, no fuzziness is used.
#define DEFAULT_FUZZINESS 10

const char* argp_program_version = "TODO v0.1-alpha";
const char* argp_program_bug_address = "https://github.com/obskyr/TODO/issues";
static char doc[] = "\nScans a series of screenshots that have been scaled up to determine what their original resolution (with square 1x1 pixels) were. Combine with a program to scale them down and then up to your desired resolution to re-scale screenshots that were taken at a wonky scale! Multiple screenshots can be supplied (as long as they're of the same resolution and were taken in the same program) to make the result more likely to be accurate (though most likely, one will suffice).";
static char args_doc[] = "<SCREENSHOT IMAGE...>";
static struct argp_option options[] = {
    {0, 0, 0, 0, "Algorithm options:"},
    {"inexact", 'i', 0, 0, "Allow for some leeway when scanning for differing pixels. Useful for, for example, PlayStation 1 screenshots."},
    {"leeway", 'l', "leeway", 0, "How much an R, G, or B value can differ when using --inexact (0-255). " STRINGIFY(DEFAULT_FUZZINESS) " by default."},

    {0, 0, 0, 0, "Output options:"},
    {"custom", 'c', "format", 0, "Print the data in a custom format you supply and exit. Available variables are {width}, {height}, {scaled_width}, {scaled_height}, {x_scale}, {y_scale}, and {par}."},
    {"par", 'p', 0, 0, "Print the determined pixel aspect ratio in the format \"{par}\" and exit."},
    {"resolution", 'r', 0, 0, "Print the determined resolution in the format \"{width}x{height}\" and exit."},
    {"scale", 's', 0, 0, "Print the determined scale in the format \"{x_scale}x{y_scale}\" and exit."},

    {0, 0, 0, 0, "Help:", -1},
    {"help", 'h', 0, 0, "Print this help page and exit."},
    {"version", 'v', 0, 0, "Print the program name and version and exit."},
    {0, 'V', 0, OPTION_ALIAS},

    {0}
};

struct options {
    bool inexact;
    int leeway;

    bool format_specified;
    char* format;

    size_t num_image_paths;
    char** image_paths;
};

static int parse_options(int key, char *arg, struct argp_state *state) {
    struct options* options = state->input;
    switch (key) {
        case 'i': options->inexact = true; break;
        case 'l':
            options->leeway = atoi(arg);
            if (options->leeway == 0 && strcmp(arg, "0") != 0) {
                fprintf(stderr, "ERROR: Invalid --leeway argument: \"%s\"\n", arg);
                exit(-1);
            }
            break;

        case 'c':
            options->format_specified = true;
            options->format = arg;
            break;
        case 'p':
            options->format_specified = true;
            options->format = "{par}";
            break;
        case 'r':
            options->format_specified = true;
            options->format = "{width}x{height}";
            break;
        case 's':
            options->format_specified = true;
            options->format = "{x_scale}x{y_scale}";
            break;

        case 'h':
            argp_state_help(state, stdout, ARGP_HELP_SHORT_USAGE | ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_BUG_ADDR | ARGP_HELP_EXIT_OK);
            break;
        
        case 'v':
            printf("%s\n", argp_program_version);
            exit(0);
            break;

        case ARGP_KEY_ARGS:
            options->num_image_paths = state->argc - state->next;
            options->image_paths = state->argv + state->next;
            break;
        
        case ARGP_KEY_NO_ARGS:
            argp_state_help(state, stdout, ARGP_HELP_SHORT_USAGE | ARGP_HELP_PRE_DOC | ARGP_HELP_EXIT_ERR);
            break;

        case ARGP_KEY_ARG: return ARGP_ERR_UNKNOWN; break;
        default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}

static struct argp argp = {options, parse_options, args_doc, doc, 0, 0, 0};

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

int fuzziness;
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
    struct options options;

    options.inexact = false;
    options.leeway = DEFAULT_FUZZINESS;
    options.format_specified = false;
    options.format = 0;

    argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &options);

    if (options.inexact) {compare_pixel = compare_pixel_fuzzy;}
    fuzziness = options.leeway;

#ifdef WIN64
    // Here's the skinny: ImageMagick is not at all friendly to portable
    // programs. It packages the bits that handle specific file formats in
    // separate DLLs, and those DLLs are located at runtime via an environment
    // variable called "MAGICK_CODER_MODULE_PATH". We don't want our users to
    // have to worry about that, so we pack the DLLs with the distributed
    // executable and set the environment variable programmatically here.
    // …I don't know why I chose C for this project, man.
    char putenv_directive[MAX_PATH + 25 + 1] = "MAGICK_CODER_MODULE_PATH=";
    DWORD length = GetModuleFileName(NULL, putenv_directive + 25, MAX_PATH + 1);
    for (size_t i = 25 + length; i >= 25; i--) {
        if (putenv_directive[i] == '\\') {
            putenv_directive[i] = 0;
            break;
        }
    }
    strcat(putenv_directive, "\\coders");
    putenv(putenv_directive);
#endif

    size_t scaled_width;
    size_t scaled_height;
    size_t determined_width;
    size_t determined_height;

    determine_dimensions(
        options.num_image_paths, options.image_paths,
        &scaled_width, &scaled_height,
        &determined_width, &determined_height
    );

    double determined_x_scale = (double) scaled_width / (double) determined_width;
    double determined_y_scale = (double) scaled_height / (double) determined_height;
    double pixel_aspect_ratio = determined_x_scale / determined_y_scale;

#ifdef DEBUG
    printf("== RESULT ==\n\n");
#endif

    if (!options.format_specified) {
        printf("Original resolution: %zu x %zu\n", determined_width, determined_height);
        printf("Scale:               %lg x %lg\n", determined_x_scale, determined_y_scale);
        printf("Pixel aspect ratio:  %lg\n", pixel_aspect_ratio);
    } else {
        print_with_format(
            options.format,
            scaled_width, scaled_height,
            determined_width, determined_height,
            determined_x_scale, determined_y_scale,
            pixel_aspect_ratio
        );
    }

    return 0;
}

void determine_dimensions(
    size_t num_image_paths, char** image_paths,
    size_t* scaled_width, size_t* scaled_height,
    size_t* determined_width, size_t* determined_height
)
{
    MagickWandGenesis();

    MagickWand* wand = NewMagickWand();
    MagickBooleanType status = MagickReadImage(wand, image_paths[0]);
    if (status == MagickFalse) {ThrowWandException(wand);}
    MagickResetIterator(wand);
    MagickNextImage(wand);
    *scaled_width = MagickGetImageWidth(wand);
    *scaled_height = MagickGetImageHeight(wand);
    unsigned char* pixels = (unsigned char*) malloc(*scaled_width * *scaled_height * 3 * sizeof(unsigned char));
    bool* column_contrasts = (bool*) calloc(*scaled_width, sizeof(bool));
    bool* row_contrasts = (bool*) calloc(*scaled_height, sizeof(bool));
    // Since the leftmost / top pixel in a scaled image always starts
    // a new pixel in the source image.
    column_contrasts[0] = true;
    row_contrasts[0] = true;

    update_contrasts_from_wand(*scaled_width, column_contrasts, *scaled_height, row_contrasts, pixels, wand);

    wand = DestroyMagickWand(wand);

    for (size_t i = 1; i < num_image_paths; i++) {
        char* cur_path = image_paths[i];
        wand = NewMagickWand();
        MagickBooleanType status = MagickReadImage(wand, cur_path);
        if (status == MagickFalse) {ThrowWandException(wand);}

        int error = update_contrasts_from_wand(*scaled_width, column_contrasts, *scaled_height, row_contrasts, pixels, wand);
        if (error) {
            fprintf(stderr, "ERROR: Screenshots not of the same resolution (\"%s\" differs).", image_paths[i]);
            exit(-1);
        }

        wand = DestroyMagickWand(wand);
    }

#ifdef DEBUG
    printf("== COLUMNS (width) ==\n\n");
#endif

    *determined_width = determine_dimension(*scaled_width, column_contrasts);

#ifdef DEBUG
    printf("== ROWS (height) ==\n\n");
#endif

    *determined_height = determine_dimension(*scaled_height, row_contrasts);

    free(pixels);
    free(column_contrasts);
    free(row_contrasts);

    MagickWandTerminus();
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

void print_with_format(
    const char* format,
    size_t scaled_width, size_t scaled_height,
    size_t determined_width, size_t determined_height,
    double determined_x_scale, double determined_y_scale,
    double pixel_aspect_ratio
)
{
    size_t format_length = strlen(format);
    char* modifiable_format = (char*) malloc(format_length * sizeof(char));
    memcpy(modifiable_format, format, format_length * sizeof(char));

    char out_str[1024] = {0};
    char* cur_out_char = out_str;
    bool in_variable = false;
    char* variable_start;
    for (char* cur_char = modifiable_format; cur_char < modifiable_format + format_length; cur_char++) {
        // cur_out_char jumps at most 2 characters forward during one iteration.
        // That is, except for when printing a variable,
        // but that has its own error handling.
        if (cur_out_char - out_str > 1024 - 1 - 2) {
            fprintf(stderr, "ERROR: Format too long.\n");
            exit(1);
        }

        if (!in_variable) {
            switch (*cur_char) {
                case '{':
                    if (*(cur_char + 1) != '{') {
                        in_variable = true;
                        variable_start = cur_char + 1;
                    } else {
                        cur_char++;
                        *cur_out_char++ = '{';
                    }
                    break;

                case '}':
                    if (*(cur_char + 1) == '}') {
                        cur_char++;
                    }
                    *cur_out_char++ = '}';
                    break;

                case '\\':
                    if (*(cur_char + 1) == 'n') {
                        cur_char++;
                        *cur_out_char++ = '\n';
                    } else {
                        *cur_out_char++ = '\\';
                    }
                    break;
                
                default: *cur_out_char++ = *cur_char;
            }
        } else {
            if (*cur_char == '}') {
                *cur_char = 0;
                in_variable = false;
                int bytes_left = 1024 - 1 - (cur_out_char - out_str);
                int written;
                if (strcmp(variable_start, "scaled_width") == 0) {
                    written = snprintf(cur_out_char, bytes_left, "%zu", scaled_width);
                } else if (strcmp(variable_start, "scaled_height") == 0) {
                    written = snprintf(cur_out_char, bytes_left, "%zu", scaled_height);
                } else if (strcmp(variable_start, "width") == 0) {
                    written = snprintf(cur_out_char, bytes_left, "%zu", determined_width);
                } else if (strcmp(variable_start, "height") == 0) {
                    written = snprintf(cur_out_char, bytes_left, "%zu", determined_height);
                } else if (strcmp(variable_start, "x_scale") == 0) {
                    written = snprintf(cur_out_char, bytes_left, "%lg", determined_x_scale);
                } else if (strcmp(variable_start, "y_scale") == 0) {
                    written = snprintf(cur_out_char, bytes_left, "%lg", determined_y_scale);
                } else if (strcmp(variable_start, "par") == 0) {
                    written = snprintf(cur_out_char, bytes_left, "%lg", pixel_aspect_ratio);
                } else {
                    written = snprintf(cur_out_char, bytes_left, "%s", variable_start - 1);
                    if (written >= 0 && bytes_left - written > 1) {
                        *(cur_out_char + written) = '}';
                        cur_out_char++;
                    } else {
                        fprintf(stderr, "ERROR: Format too long.\n");
                        exit(-1);
                    }
                }
                if (written >= 0) {
                    cur_out_char += written;
                } else {
                    fprintf(stderr, "ERROR: Format too long.\n");
                    exit(-1);
                }
                *cur_char = '}';
            }
        }
    }

    *cur_out_char = 0;

    free(modifiable_format);
    printf("%s\n", out_str);
}
