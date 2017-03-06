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

PRODUCT_ROOT=`pwd`/../../../../..
export PRODUCT_ROOT

echo "Files will be copied to your ANDROID_ROOT directory : $ANDROID_ROOT"

if [ $# -gt 0 ]; then
echo "Non interactive mode"
else
echo "Type ENTER key to continue. (CTRL+C to exit)"
read temp_var
fi

mkdir -p $ANDROID_ROOT/tf

MODULE_NAME="cryptocatalogue_client"
mkdir -p $ANDROID_ROOT/tf/$MODULE_NAME
cp Android.mk $ANDROID_ROOT/tf/$MODULE_NAME/
rm -Rf $ANDROID_PRODUCT/obj/EXECUTABLES/"$MODULE_NAME"_intermediates

pushd $ANDROID_ROOT

source build/envsetup.sh
mmm tf/$MODULE_NAME

popd

if [ -z "$BUILD_VARIANT" ] || [ "$BUILD_VARIANT" == "release" ]; then
   mkdir release
   cp $ANDROID_PRODUCT/system/bin/$MODULE_NAME release
elif [ "$BUILD_VARIANT" == "debug" ]; then
   mkdir debug
   cp $ANDROID_PRODUCT/system/bin/$MODULE_NAME debug
else
   echo "$SCRIPT_NAME: Invalid build variant!"
   exit 1
fi

