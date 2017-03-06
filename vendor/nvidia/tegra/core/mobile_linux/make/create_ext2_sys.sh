#!/bin/bash

if [ $# -lt 6 ]; then
    echo "Usage: create_ext2_sys <image name> <num blocks> <loop dev> <mount> <system path> <host bin path>"
    exit 1
fi

bash create_sys.sh "$1" "$2" "$3" "$4" "$5" "$6" mkfs.ext2
