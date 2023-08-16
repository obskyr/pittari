1. Install [MSYS2](http://www.msys2.org/).
2. In the “MSYS2 MinGW x64” shell (`C:\msys64\mingw64.exe`, assuming MSYS2 was installed into the default location), run the following commands:
   
    ```bash
    # Update and upgrade all packages, if you haven't already.
    pacman -Syuu

    # Dependencies…
    pacman -S mingw-w64-x86_64-gcc
    pacman -S mingw-w64-x86_64-pkg-config
    pacman -S mingw-w64-x86_64-zlib

    # ImageMagick itself.
    pacman -S mingw-w64-x86_64-imagemagick
    ```
<!-- Seems like *maybe* this isn't needed?

3. In the regular Windows shell (`cmd.exe`, not PowerShell), assuming MSYS2 was installed into the default location, run the following commands:

    ```cmd
    set PATH=%PATH%C:\msys64\mingw64\bin;
    set PKG_CONFIG_PATH=C:\msys64\mingw64\lib\pkgconfig
    set MAGICK_CODER_MODULE_PATH=C:\msys64\mingw64\lib\[ IMAGEMAGICK DIRECTORY HERE ]\modules-Q16HDRI\coders
    ```
-->
