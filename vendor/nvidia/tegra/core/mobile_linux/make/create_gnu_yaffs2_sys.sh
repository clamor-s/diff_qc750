#!/bin/bash

export PATH=$PATH:/sbin

_usage()
{
    echo "Usage: create_gnu_yaffs2_sys.sh <image name> <targetfs path> <image generation tool>"
    echo "This tool must be run as root"
    exit 1
}

if [ $# -ne 3 ]; then
    _usage
fi

if [ "$(whoami)" != "root" ] ; then
    _usage
    exit 1
fi

img="$1"
targetfspath="$2"
imgtool="$3"

if [ ! -d $targetfspath ] ; then
    echo "$targetfspath is not the name of a directory"
    _usage
    exit 1
fi

_die()
{
    rm -f $img
    echo "FAILED"
    exit 1
}

rm -f $img
$imgtool $targetfspath $img || _die

