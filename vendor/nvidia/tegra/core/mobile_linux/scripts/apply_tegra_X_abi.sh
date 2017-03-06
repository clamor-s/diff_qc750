#!/bin/bash

#
# This installs a tegra X driver targetting the specified ABI into the rootfs dir pointed to by
# LDK_ROOTFS_DIR variable.
#

function ShowUsage {
    local ScriptName=$1
    local Msg=$2

    if [ -n "$Msg" ]; then
	echo "$Msg"
    fi

    echo "Use: $ScriptName [--abi ABI] [--root|-r PATH] [--help|-h]"
    echo "Or : $ScriptName [--root|-r PATH] [--help|-h] ABI"
cat <<EOF
    This script installs the tegra X driver with the requested ABI to a root filesystem.

    Options are:
    --root|-r PATH
                   install X driver to PATH
                   default is 'rootfs' directory inside the script directory
    --abi ABI
                   specify the ABI version
    --help|-h
                   show this help

EOF
}

function ShowDebug {
    echo "SCRIPT_NAME    : $SCRIPT_NAME"
    echo "LDK_ROOTFS_DIR : $LDK_ROOTFS_DIR"
    echo "ABI            : $ABI"
}

# script name
SCRIPT_NAME=`basename $0`

# parse the command line first
TGETOPT=`getopt -n "$SCRIPT_NAME" --longoptions help,debug,root:,abi: -o dhr: -- "$@"`

if [ $? != 0 ]; then
    ShowUsage "$SCRIPT_NAME" "ERROR: Terminating... wrong switch"
    exit 1
fi

# empty root and no debug
ROOT=
DEBUG=

eval set -- "$TGETOPT"

while [ $# -gt 0 ]; do
    case "$1" in
	-r|--root) LDK_ROOTFS_DIR="$2"; shift ;;
	-h|--help) ShowUsage "$SCRIPT_NAME"; exit 1 ;;
	-d|--debug) DEBUG="true" ;;
	--abi) ABI="$2"; shift ;;
	--) shift; break ;;
	-*) ShowUsage "$SCRIPT_NAME" "ERROR: Terminating... wrong command switch"; exit 1 ;;
    esac
    shift
done

# enforce this argument
if [ -z "$ABI" ]; then
    if [ $# -gt 1 -o $# -lt 1 ]; then
	ShowUsage "$SCRIPT_NAME" "ERROR: Terminating... wrong command switch"
	exit 1
    fi
    ABI=$1
else
    if [ $# -gt 0 ]; then
	ShowUsage "$SCRIPT_NAME" "ERROR: Terminating... wrong command switch"
	exit 1
    fi
fi

if [ "x$DEBUG" == "xtrue" ]; then
    ShowDebug
fi

# done, now do the work, save the directory
TOP=$PWD

LDK_DIR=$(cd `dirname $0` && pwd)

# use default rootfs dir if none is set
if [ -z "$LDK_ROOTFS_DIR" ] ; then
    LDK_ROOTFS_DIR="${LDK_DIR}/rootfs"
fi

echo "Using rootfs directory of: ${LDK_ROOTFS_DIR}"

LDK_NV_TEGRA_DIR="${LDK_DIR}/nv_tegra"

if [ ! -d "${LDK_ROOTFS_DIR}/usr/lib/xorg/modules/drivers" ]; then
    echo "ERROR: ${LDK_ROOTFS_DIR}/usr/lib/xorg/modules/drivers does not exist!"
    echo "       Create your root filesystem first and then run this script."
    exit 3
fi

if [ ! -f "${LDK_NV_TEGRA_DIR}/x/tegra_drv.abi${ABI}.so" ]; then
    echo "ERROR: ABI version $ABI is not supported."
    exit 4
fi

echo "Installing X ABI version $ABI into ${LDK_ROOTFS_DIR}..."

install --owner=root --group=root --mode=644 "${LDK_NV_TEGRA_DIR}/x/tegra_drv.abi${ABI}.so" "${LDK_ROOTFS_DIR}/usr/lib/xorg/modules/drivers/tegra_drv.so"

echo "Success!"
