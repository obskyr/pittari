# TODO

Have you ever taken a screenshot in an emulator that *just wasn't scaled right?* Wonky pixels of different sizes; mamma mia. TODO takes such screenshots and figures out what resolution they originally had for you, so that you can [resize](#resizing-screenshots) them to your heart's content.

## Installation

1. Obtain the program!
    * **[Download for Windows (x64)](TODO)**
    * **[Download for Linux (x64)](TODO)**
    * See *[How to build](#how-to-build)* if you are on a 32-bit system.
2. Extract the files to their own folder, wherever you'd like.
3. On Windows, you may want to [add said folder to your PATH.](https://stackoverflow.com/a/44272417) This will both allow you to run the program from anywhere, and to copy the EXE (without its accompanying DLLs) to anywhere you'd like.

## Usage

To determine the original resolution of a screenshot, **simply pass it as an argument:**

```console
$ TODO spelunker-arcade.png 
Original resolution: 384 x 256
Scale:               2.67188 x 3
Pixel aspect ratio:  0.890625
```

---

Screenshots with horizontal or vertical bands of solid colors may thwart the resolution detection. To make the result more likely to be accurate, **you can supply multiple screenshots** (as long as they were all originally scaled up in the same way):

```console
$ TODO screenshot-1.png screenshot-2.png multiple-screenshots-in-one.gif
Original resolution: 256 x 224
Scale:               2.85547 x 2.5
Pixel aspect ratio:  1.14219
```

---

The option `--print` (`-p`) lets you **print a single attribute** for programmatic parsing – `resolution` (or `r`), `scale` (or `s`), or `pixel aspect ratio` (or `par`).

```console
$ TODO screenshot.png -p r # Original resolution
256x240

$ TODO screenshot.png -p s # Scale
3.42969x3

$ TODO screenshot.png -p par # Pixel aspect ratio
1.14323
```

---

There is also `--custom` (`-c`) for entirely custom print formats, as well as further options for the resolution determination algorithm itself – see `todo --help`!

## Resizing screenshots

To resize your wonkily scaled screenshots back to 1:1 square pixels, you can use [ImageMagick](https://imagemagick.org/script/download.php) (because [most GUI graphics editors do nearest-neighbor scaling wrong](tests/Notes%20on%20nearest-neighbor%20scaling/README.md)). The relevant command is `magick convert`, and make sure to supply `-filter point` (for nearest-neighbor scaling) and to add an exclamation mark after your desired dimensions (to turn off aspect ratio correction). If `TODO` gave you `256x224` as the resolution, for example:

```bash
magick convert input.png -filter point -resize 256x224! output.png
```

To scale an image up, make sure to scale it down *first* – that way, you're actually scaling up the 1:1 image. For example, to take a wonkily scaled SNES screenshot and scale it to 3× at the SNES's native aspect ratio, you can do:

```bash
# For the width: 256 * 3 * 8⁄7 (an NTSC SNES's pixel aspect ratio) ≈ 878
# For the height: 224 × 3 = 672
magick convert input.png -filter point -resize 256x224! -resize 878x672! output.png
```

Until the advent of LCD displays, most systems did not have square pixels – they may be a bit wider than they are tall, or a bit taller than they are wide. This is called the “pixel aspect ratio” and is separate from its display aspect ratio (what you'd usually just call the “aspect ratio”). This is *crucial* to making emulated video (and thus screenshots) look correct, so yours truly heartily recommends taking it into account. It's as simple as multiplying the width of the image by a certain number while leaving the height untouched! [Pixel aspect ratios for various systems can be found here.](https://pineight.com/mw/page/Dot_clock_rates.xhtml)

## Limitations

* Only screenshots that have been scaled up – not scaled down – are supported.
* Only screenshots that have been scaled up with nearest-neighbor scaling are supported – bilinear is not implemented. (Do let me know if that's something you could use!)
    * However, images that are *almost* scaled up with nearest-neighbor scaling and have pixels that ever so slightly vary are supported using the `--inexact` flag. For example, screenshots of PS1 games on PS Vita (e.g. [`tests/254x231 fuzzy.png`](tests/254x231%20fuzzy.png)).

## How to build

## On Windows
1. Install [MSYS2](http://www.msys2.org/).
2. In the “MSYS2 MinGW x64” shell (`C:\msys64\mingw64.exe`, assuming MSYS2 was installed into the default location), run the following commands:
   
    ```bash
    # Update and upgrade all packages, if you haven't already.
    pacman -Syuu

    # Dependencies…
    pacman -S mingw-w64-x86_64-gcc
    pacman -S mingw-w64-x86_64-pkg-config
    pacman -S mingw-w64-x86_64-zlib
    pacman -S mingw-w64-x86_64-imagemagick
    pacman -S mingw-w64-x86_64-meson

    # Clone the necessary repositories…
    git clone https://github.com/obskyr/TODO.git
    cd TODO
    git clone https://github.com/argp-standalone/argp-standalone.git

    # Build!
    make
    ```
3. And presto! The program is now at `build/TODO.exe`. If moving this elsewhere, make sure to copy the rest of the contents of the `build/` directory as well, or to add the directory to your PATH. This is required because `TODO.exe` must be able to find its DLLs.

<!-- Seems like *maybe* this isn't needed?

1. In the regular Windows shell (`cmd.exe`, not PowerShell), assuming MSYS2 was installed into the default location, run the following commands:

    ```cmd
    set PATH=%PATH%C:\msys64\mingw64\bin;
    set PKG_CONFIG_PATH=C:\msys64\mingw64\lib\pkgconfig
    set MAGICK_CODER_MODULE_PATH=C:\msys64\mingw64\lib\[ IMAGEMAGICK DIRECTORY HERE ]\modules-Q16HDRI\coders
    ```
-->

## On Linux

1. Run the following commands:
    ```bash
    # Update your package lists, if you haven't already.
    sudo apt update

    # Dependencies…
    sudo apt install libmagickwand-dev

    # Clone the repository…
    git clone https://github.com/obskyr/TODO.git
    cd TODO

    # Build!
    make
    ```
3. And presto! The program is now at `build/TODO`.
