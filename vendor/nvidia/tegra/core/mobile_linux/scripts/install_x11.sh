#!/bin/bash

#
# Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
#  
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

#
# Download, build and install X11 development libraries and headers
# for a cross-compilation environment.  Also downloads prerequisite
# configuration utilities.
#
# Optional environment variables:
# CROSS_ROOT: points to the root of the cross compilation environment.
#
# Usage: install_x11.sh
#

TARGET=${TARGET-arm-none-linux-gnueabi}
TOPDIR=$PWD/xorg
X11_ROOT=${X11_ROOT-$TOPDIR/$TARGET}
BUILDDIR=${BUILDDIR-$X11_ROOT/build}

LOGFILE=$BUILDDIR/xorg.log

usage() {
    echo `basename $0` [-examples] [-internal]
    echo -e "\t-examples\tInstalls additional libraries required for building the sample code,"
    echo -e "\t\t\tincluding libXfixes, libXcomposite and libXdamage."
    echo -e "\t-internal\tInstalls additional libraries for NVIDIA internal use."
    echo -e "\t-help\t\tPrints this usage message."

    exit $1
}

internal=
examples=
while [ $# -gt 0 ]; do
    arg=$1; shift
    case $arg in
	(-internal) internal=1 ;;
	(-examples) examples=1 ;;
	(-help) usage ;;
	(*) echo Unknown option \"$arg\"; usage 1 ;;
    esac
done

if [ -d "$CROSS_ROOT" ]; then
    export PATH="$X11_ROOT/bin:$CROSS_ROOT/bin:$PATH"
fi

stdout() {
    echo $* > /dev/tty
}

die() {
    stdout
    stdout "Problem encountered, terminating."
    stdout "See $LOGFILE for details."
    exit 1
}

safe() {
    $* || die
}

# Make sure the compiler is in $PATH
if ! hash arm-none-linux-gnueabi-gcc; then
    stdout "Could not find the compiler arm-none-linux-gnueabi-gcc"
    stdout "Please make sure it's in your \$PATH, or set CROSS_ROOT"
    stdout "to point to the root of the cross compiler environment."
    exit 1
fi

# Attempt to wget a file.  If it fails, clean up the partial download
safe-wget() {
    url=$1
    file=`basename $url`
    if wget $1; then
        # Make sure we got a real tarball and not an html error page
        doctype=`strings $file | grep DOCTYPE`
	[ -n "$doctype" ] && rm -f $file
    else
        stdout "wget failed url: $1"
        rm -f `basename $1`
        die
    fi
}

download() {
    retries=0
    base=$1
    url=$2
    file=`basename $url`

    stdout -n "Downloading $base ... "
    if [ -f $file ]; then
	echo "$file exists, skipping download"
    else
	while true; do
	    safe-wget $url
	    [ -f $file ] && break
	    retries=`expr $retries + 1`
	    case $retries in
		(5) stdout Aborting; die ;;
		(*) [ $retries -eq 1 ] && stdout
		    sleep $retries
		    stdout "Problem downloading $base, retrying ($retries)" ;;
	    esac
	done
    fi
    stdout done.
}

extract() {
    file=$1.$2
    ext=`echo $file | awk -F. '{ print $NF }'`
    case $ext in
	(gz|tgz) compress=z ;;
	(bz2) compress=j ;;
    esac
    tar xf${compress} $file
}

build() {
    base=$1
    shift
    configure="$*"
    safe pushd $base
    stdout -n "Installing $base ... "
    safe $configure
    safe make
    safe make install
    stdout done.
    safe popd
}

split-base() {
    basename $1 | awk -F. '{
        i=1;
        while (i < NF-2 ) {
            printf "%s.", $i;
            i++;
        }
        printf "%s %s.%s\n", $i, $(i+1), $(i+2);
    }'
}

package-get() {
    url=$1; shift
    configure=$*
    set -k `split-base $url`
    base=$1; extension=$2
    download $base $url
    extract $base $extension
    build $base $configure
}

# Configuration utilities
GNU_SERVER=http://ftp.gnu.org/gnu

M4=m4/m4-1.4.15
AUTOCONF=autoconf/autoconf-2.65
AUTOMAKE=automake/automake-1.9
LIBTOOL=libtool/libtool-2.2

GNU_UTILS="$M4 $AUTOCONF $AUTOMAKE $LIBTOOL"

# Other utilities
PKGCONFIG=http://pkg-config.freedesktop.org/releases/pkg-config-0.23.tar.gz

UTILITIES="$PKGCONFIG"

# X11 headers and libraries
X11_SERVER=http://xorg.freedesktop.org/releases/individual

MACROS=util/util-macros-1.7.0
XPROTO=proto/xproto-7.0.16
XEXTPROTO=proto/xextproto-7.1.1
INPUTPROTO=proto/inputproto-2.0
KBPROTO=proto/kbproto-1.0.4
XCMISCPROTO=proto/xcmiscproto-1.2.0
BIGREQSPROTO=proto/bigreqsproto-1.1.0
FIXESPROTO=proto/fixesproto-4.1.1
COMPOSITEPROTO=proto/compositeproto-0.4.1
DAMAGEPROTO=proto/damageproto-1.2.0

FONTSPROTO=proto/fontsproto-2.1.0
RENDERPROTO=proto/renderproto-0.9.2
RANDRPROTO=proto/randrproto-1.3.1
INPUTPROTO=proto/inputproto-2.0
RECORDPROTO=proto/recordproto-1.14

LIBXTRANS=lib/xtrans-1.2.5
LIBXAU=lib/libXau-1.0.5
LIBX11=lib/libX11-1.3.3
LIBXEXT=lib/libXext-1.1.2
LIBXFIXES=lib/libXfixes-4.0.4
LIBXCOMPOSITE=lib/libXcomposite-0.4.2
LIBXDAMAGE=lib/libXdamage-1.1.2

#LIBXRENDER=lib/libXrender-0.9.5
#LIBXRANDR=lib/libXrandr-1.3.0
LIBPIXMAN=lib/pixman-0.18.0
LIBXI=lib/libXi-1.3.2
LIBICE=lib/libICE-1.0.6
LIBSM=lib/libSM-1.1.1
LIBXT=lib/libXt-1.0.8
LIBXTST=lib/libXtst-1.1.0
LIBXDMCP=lib/libXdmcp-1.1.0

X11_UTILS="$MACROS"
X11_PROTO1="$XPROTO $XEXTPROTO $XCMISCPROTO $BIGREQSPROTO $INPUTPROTO $KBPROTO"
X11_PROTO2="$X11_PROTO $FIXESPROTO $COMPOSITEPROTO $DAMAGEPROTO"
X11_PROTO3="$FONTSPROTO $RENDERPROTO $RANDRPROTO $INPUTPROTO $RECORDPROTO"
X11_LIBS1="$LIBXAU $LIBXTRANS $LIBX11 $LIBXEXT"
X11_LIBS2="$X11_LIBS $LIBXFIXES $LIBXCOMPOSITE $LIBXDAMAGE"
X11_LIBS3="$LIBPIXMAN $LIBXI $LIBICE $LIBSM $LIBXT $LIBXTST $LIBXDMCP"

X11_PROTO="$X11_PROTO1"
X11_LIBS="$X11_LIBS1"
if [ -n "$examples" -o -n "$internal" ]; then
    X11_PROTO="$X11_PROTO $X11_PROTO2"
    X11_LIBS="$X11_LIBS $X11_LIBS2"
fi
if [ -n "$internal" ]; then
    X11_PROTO="$X11_PROTO $X11_PROTO3"
    X11_LIBS="$X11_LIBS $X11_LIBS3"
fi
X11_PACKAGES="$X11_UTILS $X11_PROTO $X11_LIBS"


# Real work starts here.  Change to a subdirectory so we don't pollute
# the current directory.
safe mkdir -p $BUILDDIR
cd $BUILDDIR

exec &> $LOGFILE
stdout Beginning X11 installation.
stdout Logging to $LOGFILE

for i in $GNU_UTILS; do
    package-get $GNU_SERVER/$i.tar.bz2 ./configure --prefix=$X11_ROOT
done

for i in $UTILITIES; do
    package-get $i ./configure --prefix=$X11_ROOT
done

XLIBFLAGS="--build=i686-pc-linux-gnu --host=$TARGET --target=$TARGET --with-xcb=no"

for i in $X11_PACKAGES; do
    package-get $X11_SERVER/$i.tar.bz2  ./configure --prefix=$X11_ROOT $XLIBFLAGS
done

# Delete .la files, which are supposed to help libtool but usually just get in
# the way or cause warnings.
find $X11_ROOT -name '*.la' -delete

stdout X11 installation successful.
