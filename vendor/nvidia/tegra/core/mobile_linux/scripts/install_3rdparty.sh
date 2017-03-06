#!/bin/bash

#
# Install 3rd party software used by other scripts.
# Installation happens only once, even if the script is run multiple times.
#
# The following will be installed:
# - Sourcery G++ Lite 2009q1-203 for ARM GNU/Linux
#
# Optional environment variables:
#
# SOURCERY_ROOT     points to an existing Sourcery G++ Lite installation,
#                   Sourcery installation skipped if set
# FORCE             forces installation, if "true"
#
# Usage: install_3rdparty.sh [--force|-f] [--root|-r path_to_install_sourcery_g++] [--url|-u url_of_sourcery_g++] [--help|-h]
#

# show the usages text
function ShowUsage {
    local ScriptName=$1
    
    echo "Use: $1 [--force|-f] [--root|-r PATH] [--url|-u URL] [--help|-h]"
cat <<EOF
    This script installs kernel development toolchain.
    Options are:
    --root|-r PATH
                   install toolchain to PATH
    --url|-u URL
                   download the toolchain from URL
                   default is Sourcery G++ Lite 2009q1-203
    --force|-f     
                   remove previous installation (if any) and force install
    --help|-h
                   show this help
EOF
}

# remove any previous installation and install fresh
function InstallDevTool {
    local DevToolPath=$1
    local url=$2

    local toret=0
    local devtoolfile="$$.tmp"

    mkdir -p "$DevToolPath/_out/3rdparty"
    pushd "$DevToolPath/_out/3rdparty" > /dev/null 2>&1

    rm -rf ./* > /dev/null 2>&1
    
    echo "Downloading and installing Sourcery G++ Lite...."
    wget -q -O "$devtoolfile" -t0 "$url"

    if [ $? -eq 0 ]; then
	# try tar+gz first
	tar tzf "$devtoolfile"  > /dev/null 2>&1
	if [ $? -eq 0 ]; then
	    tar xzf "$devtoolfile"  > /dev/null 2>&1
	else
	    # try tar+bz2 then
	    tar tjf "$devtoolfile"  > /dev/null 2>&1
	    if [ $? -eq 0 ]; then
		tar xjf "$devtoolfile"  > /dev/null 2>&1
	    else
	        # try tar finally
		tar tf "$devtoolfile"  > /dev/null 2>&1
		if [ $? -eq 0 ]; then
		    tar xf "$devtoolfile"  > /dev/null 2>&1
		else
		    echo "Don't know how to handle the downloaded toolchain file..."
		    rm -rf ./* > /dev/null 2>&1		    
		    toret=1
		fi
	    fi
	fi
	rm -f "$devtoolfile"
    else
	echo "Kernel Development toolchain couldn't be found"
	toret=1
    fi

    popd > /dev/null 2>&1

    return $toret
}

function ShowDebug {
    echo "SCRIPT_NAME  : $SCRIPT_NAME"
    echo "FORCE        : $FORCE"
    echo "ROOT         : $ROOT"
    echo "SOURCERY_ROOT: $SOURCERY_ROOT"
}

# script name
SCRIPT_NAME=`basename $0`

# empty root and no debug
ROOT=
DEBUG=

# default g++ dev tool path
URL='http://www.codesourcery.com/sgpp/lite/arm/portal/package4571/public/arm-none-linux-gnueabi/arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2'

# parse the command line first
TGETOPT=`getopt -n "$SCRIPT_NAME" --longoptions help,force,debug,root:,url: -o dhfr:u: -- "$@"`

if [ $? != 0 ]; then 
    echo "Terminating... wrong switch" >&2
    ShowUsage "$SCRIPT_NAME"
    exit 1 
fi


eval set -- "$TGETOPT"

while [ $# -gt 0 ]; do
    case "$1" in
	-f|--force) FORCE="true" ;;
	-r|--root) ROOT="$2"; shift ;;
	-u|--url) URL="$2"; shift ;;
	-h|--help) ShowUsage "$SCRIPT_NAME"; exit 1 ;;
	-d|--debug) DEBUG="true";;
	--) shift; break;;
	-*) echo "Terminating... wrong switch: $@" >&2 ; ShowUsage "$SCRIPT_NAME"; exit 1 ;;
    esac
    shift 
done

if [ $# -gt 0 ]; then
    echo "Terminating... wrong switch" >&2
    ShowUsage "$SCRIPT_NAME"
    exit 1 
fi

if [ "x$DEBUG" == "xtrue" ]; then
    ShowDebug
fi

# save the current directory
TOP=$PWD

if [ "x$FORCE" == "xtrue" ]; then
    # find the path to install 
    if [ -n "$ROOT" ]; then
	# --root switch overwrites everything
	SOURCERY_ROOT="$ROOT/_out/3rdparty/arm-2009q1"
    elif [ -z "$SOURCERY_ROOT" ]; then
	# default is to install in current directory
	SOURCERY_ROOT="$TOP/_out/3rdparty/arm-2009q1"
    fi

    INSTALL_PATH=${SOURCERY_ROOT%'_out/3rdparty/arm-2009q1'}

    if [ "x$INSTALL_PATH" == "x$SOURCERY_ROOT" ]; then
	echo "Installation path mentioned in --root switch or SOURCERY_ROOT environment variable is wrong"
	exit 2
    fi
else # force installation is not imposed
    if [ -n "$SOURCERY_ROOT" ]; then
	echo "SOURCERY_ROOT is set. Skipping installation."
	exit 0
    else
        # find the path to install 
	if [ -n "$ROOT" ]; then
	    # --root switch overwrites everything
	    SOURCERY_ROOT="$ROOT/_out/3rdparty/arm-2009q1"
	else
	    # default is to install in current directory
	    SOURCERY_ROOT="$TOP/_out/3rdparty/arm-2009q1"
	fi

	INSTALL_PATH=${SOURCERY_ROOT%'_out/3rdparty/arm-2009q1'}

	if [ "x$INSTALL_PATH" == "x$SOURCERY_ROOT" ]; then
	    echo "Installation path mentioned in --root switch or SOURCERY_ROOT environment variable is wrong"
	    exit 2
	fi

	if [ -f "$INSTALL_PATH/_out/3rdparty/installed_sourcery" ]
	then
            echo "Sourcery G++ Lite is already installed."
	    exit 0
	fi
    fi
fi

InstallDevTool "$INSTALL_PATH" "$URL"
if [ $? -eq 0 ]; then
    echo $SOURCERY_ROOT >> "$INSTALL_PATH/_out/3rdparty/installed_sourcery"
    echo "Complete, installed Sourcery G++ Lite in:"
    echo "    $SOURCERY_ROOT"
fi
