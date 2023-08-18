#include "cli/format.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
