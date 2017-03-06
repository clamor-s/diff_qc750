#!/bin/bash

SCRIPT_NAME=`basename $0`

if [ -z "$ANDROID_ROOT" ]; then
   echo "$SCRIPT_NAME: Please set the \$ANDROID_ROOT envvar!"
   exit 1
elif [ ! -d "$ANDROID_ROOT" ]; then
   echo "$SCRIPT_NAME: \$ANDROID_ROOT does not reference a directory!"
   exit 1
fi

if [ -z "$ANDROID_PRODUCT" ]; then
   echo "$SCRIPT_NAME: Please set the \$ANDROID_PRODUCT envvar!"
   exit 1
elif [ ! -d "$ANDROID_PRODUCT" ]; then
   echo "$SCRIPT_NAME: \$ANDROID_PRODUCT does not reference a directory!"
   exit 1
fi

echo "Files will be copied to your ANDROID_ROOT directory : $ANDROID_ROOT"
echo "PRODUCT ROOT: $PRODUCT_ROOT"

if [ $# -gt 0 ]; then
echo "Non interactive mode"
else
echo "Type ENTER key to continue. (CTRL+C to exit)"
read temp_var
fi

mkdir -p $ANDROID_ROOT/tl

MODULE_NAME="liboemcrypto"
mkdir -p $ANDROID_ROOT/tl/$MODULE_NAME
cp Android.mk $ANDROID_ROOT/tl/$MODULE_NAME/Android.mk
rm -Rf $ANDROID_PRODUCT/obj/STATIC_LIBRARIES/"$MODULE_NAME"_intermediates

pushd $ANDROID_ROOT

source $ANDROID_ROOT/build/envsetup.sh
mmm tl/liboemcrypto

popd
