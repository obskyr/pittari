#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stddef.h>
#ifdef IMAGEMAGICK_7
#include <MagickWand/MagickWand.h>
#endif
#ifdef IMAGEMAGICK_6
#include <wand/MagickWand.h>
#endif

int main(int argc, char **argv);

#endif
