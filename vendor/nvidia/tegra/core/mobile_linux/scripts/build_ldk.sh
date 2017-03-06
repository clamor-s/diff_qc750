#!/bin/bash

#
# Compile NVIDIA source drivers and tests.  The results are placed into:
#       _out/_out/built_{build}_nvap20_gnu_linux_armv6/
# 
# Use apply_ldk.sh to install the built drivers on top of the filesystem for
# the target board.
#
# Required enviroment variables:
# TARGET_BOARD: board name (harmony or whistler)
# 
# Optional environment variables:
# SOURCERY_ROOT: points to a Sourcery G++ Lite installation.  
#       Defaults to ./_out/3rdparty/arm-2009q1
#
# Usage: build_ldk.sh
# 

if [ "$TARGET_BOARD" != harmony -a "$TARGET_BOARD" != whistler ]
then
    echo "TARGET_BOARD invalid. Should be set to harmony or whistler."
    exit 1
fi

make -C ldk/make
