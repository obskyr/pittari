#!/usr/bin/env bash

uname_output="$(uname -s)"
case "${uname_output}" in
    MINGW*) machine=MinGw;;
    Linux*) machine=Linux;;
    *)      machine=
esac

if [ $machine == MinGw ]; then
    case "${uname_output}" in
        MINGW64*) mingw_root=/mingw64;;
        MINGW32*) mingw_root=/mingw32;;
    esac
    libs_dir=$mingw_root/lib
elif [ $machine == Linux ]; then
    libs_dir=/usr/lib/$(uname -p)-linux-gnu
else
    >&2 echo "ERROR: Unsupported system."
    exit 1
fi

magick_dirs=( $libs_dir/ImageMagick* )
if [[ ${magick_dirs[-1]} == *"*"* ]]; then
    >&2 echo "ERROR: ImageMagick library not found."
    exit 1
fi
magick_lib_dir=${magick_dirs[-1]}

for output in "$@"
do
    if [ $output == MagickWand-config ]; then
        if [ $machine == MinGw ]; then
            echo $mingw_root/bin/MagickWand-config
        elif [ $machine == Linux ]; then
            echo $magick_lib_dir/bin-*/MagickWand-config
        fi
    elif [ $output == coders ]; then
        echo $magick_lib_dir/modules/*coders
    fi
done
