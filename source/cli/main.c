#include "cli/main.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN64
#include <Windows.h>
#endif
#include <argp.h>
#include <MagickWand/MagickWand.h>
#include "algorithm/compare.h"
#include "algorithm/dimensions.h"
#include "algorithm/interface.h"
#include "cli/format.h"

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
    {"leeway", 'l', "[0..255]", 0, "How much an R, G, or B value can differ when using --inexact (0-255). " STRINGIFY(DEFAULT_FUZZINESS) " by default."},
    {"nearest-neighbor-variation", 'n', "[0...]", 0, "With nearest-neighbor scaling to a non-integer factor, pixels will only vary by 1 pixel in size in each dimension. However, if nearest-neighbor scaling has been applied multiple times to an image, this variation may be larger. For such images, this option lets you set the maximum variation in width/height of rows/columns. 1 by default."},

    {0, 0, 0, 0, "Output options:"},
    {"custom", 'c', "format", 0, "Print the data in a custom format you supply and exit. Available variables are {width}, {height}, {scaled_width}, {scaled_height}, {x_scale}, {y_scale}, and {par}."},
    {"print", 'p', "property", 0, "Print one property and exit. Valid values are \"resolution\" (or \"r\"), \"scale\" (or \"s\"), and \"pixel aspect ratio\" (or \"par\"), printing in the formats \"{width}x{height}\", \"{x_scale}x{y_scale}\", and \"{par}\" respectively. Try --custom for more precise output control."},

    {0, 0, 0, 0, "Help:", -1},
    {"help", 'h', 0, 0, "Print this help page and exit."},
    {"usage", 0x80, 0, 0, "Print a short usage message and exit."},
    {"version", 'v', 0, 0, "Print the program name and version and exit."},
    {0, 'V', 0, OPTION_ALIAS},

    {0}
};

struct options {
    bool inexact;
    int leeway;
    int nearest_neighbor_max_variation;

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
            if ((options->leeway == 0 && strcmp(arg, "0") != 0) || options->leeway < 0) {
                fprintf(stderr, "ERROR: Invalid --leeway argument: \"%s\"\n", arg);
                exit(-1);
            }
            break;
        case 'n':
            options->nearest_neighbor_max_variation = atoi(arg);
            if ((options->nearest_neighbor_max_variation == 0 && strcmp(arg, "0") != 0) || options->nearest_neighbor_max_variation < 0) {
                fprintf(stderr, "ERROR: Invalid --nearest-neighbor-variation argument: \"%s\"\n", arg);
                exit(-1);
            }
            break;

        case 'c':
            options->format_specified = true;
            options->format = arg;
            break;
        case 'p':
            options->format_specified = true;
            if (strcmp(arg, "resolution") == 0 || strcmp(arg, "r") == 0) {
                options->format = "{width}x{height}";
            } else if (strcmp(arg, "scale") == 0 || strcmp(arg, "s") == 0) {
                options->format = "{x_scale}x{y_scale}";
            } else if (strcmp(arg, "pixel aspect ratio") == 0 || strcmp(arg, "par") == 0) {
                options->format = "{par}";
            } else {
                fprintf(stderr, "ERROR: Invalid --print argument: \"%s\"\nValid arguments are \"resolution\" (or \"r\"), \"scale\" (or \"s\"), and \"pixel aspect ratio\" (or \"par\").", arg);
                exit(-1);
            }
            break;

        case 'h':
            argp_state_help(state, stdout, ARGP_HELP_SHORT_USAGE | ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_BUG_ADDR | ARGP_HELP_EXIT_OK);
            break;
        case 0x80:
            argp_state_help(state, stdout, ARGP_HELP_USAGE | ARGP_HELP_EXIT_OK);
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

int main(int argc, char **argv)
{
    struct options options;

    options.inexact = false;
    options.leeway = DEFAULT_FUZZINESS;
    options.nearest_neighbor_max_variation = 1;
    options.format_specified = false;
    options.format = 0;

    argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &options);

    if (options.inexact) {compare_pixel = compare_pixel_fuzzy;}
    compare_pixel_fuzzy_fuzziness = options.leeway;
    nearest_neighbor_max_variation = options.nearest_neighbor_max_variation;

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
