#!/bin/bash

#
# This script applies the tegra binaries to the rootfs dir pointed to by
# LDK_ROOTFS_DIR variable.
#

# show the usages text
function ShowUsage {
    local ScriptName=$1

    echo "Use: $1 [--root|-r PATH] [--help|-h]"
cat <<EOF
    This script installs tegra binaries
    Options are:
    --root|-r PATH
                   install toolchain to PATH
    --help|-h
                   show this help
EOF
}

function ShowDebug {
    echo "SCRIPT_NAME    : $SCRIPT_NAME"
    echo "LDK_ROOTFS_DIR : $LDK_ROOTFS_DIR"
}

# if the user is not root, there is not point in going forward
THISUSER=`whoami`
if [ "x$THISUSER" != "xroot" ]; then
    echo "This script requires root privilage"
    exit 1
fi

# script name
SCRIPT_NAME=`basename $0`

# empty root and no debug
DEBUG=

# parse the command line first
TGETOPT=`getopt -n "$SCRIPT_NAME" --longoptions help,debug,root: -o dhr: -- "$@"`

if [ $? != 0 ]; then
    echo "Terminating... wrong switch"
    ShowUsage "$SCRIPT_NAME"
    exit 1
fi

eval set -- "$TGETOPT"

while [ $# -gt 0 ]; do
    case "$1" in
	-r|--root) LDK_ROOTFS_DIR="$2"; shift ;;
	-h|--help) ShowUsage "$SCRIPT_NAME"; exit 1 ;;
	-d|--debug) DEBUG="true" ;;
	--) shift; break ;;
	-*) echo "Terminating... wrong switch: $@" >&2 ; ShowUsage "$SCRIPT_NAME"; exit 1 ;;
    esac
    shift
done

if [ $# -gt 0 ]; then
    ShowUsage "$SCRIPT_NAME"
    exit 1
fi

# done, now do the work, save the directory
LDK_DIR=$(cd `dirname $0` && pwd)

# use default rootfs dir if none is set
if [ -z "$LDK_ROOTFS_DIR" ] ; then
    LDK_ROOTFS_DIR="${LDK_DIR}/rootfs"
fi

echo "Using rootfs directory of: ${LDK_ROOTFS_DIR}"

if [ ! -d "${LDK_ROOTFS_DIR}" ]; then
    mkdir -p "${LDK_ROOTFS_DIR}" > /dev/null 2>&1
fi

# get the absolute path, for LDK_ROOTFS_DIR.
# otherwise, tar behaviour is unknown in last command sets
TOP=$PWD
cd "$LDK_ROOTFS_DIR"
LDK_ROOTFS_DIR="$PWD"
cd "$TOP"

# assumption: this script is part of the BSP
#             so, LDK_DIR/nv_tegra always exist
LDK_NV_TEGRA_DIR="${LDK_DIR}/nv_tegra"

echo "Extracting the NVIDIA user space components to ${LDK_ROOTFS_DIR}"
pushd "${LDK_NV_TEGRA_DIR}/base/" > /dev/null 2>&1
tar cpf - * | ( cd "${LDK_ROOTFS_DIR}" ; tar xpf - )
popd > /dev/null 2>&1

echo "Success!"
