#!/bin/bash

SCRIPT_NAME=`basename $0`
MODULE_NAME="oemcrypto_client"

if [ -z "$PRODUCT_ROOT" ]; then
   echo "$SCRIPT_NAME: Please set the \$PRODUCT_ROOT envvar!"
   exit 1
elif [ ! -d "$PRODUCT_ROOT" ]; then
   echo "$SCRIPT_NAME: \$PRODUCT_ROOT does not reference a directory!"
   exit 1
fi

if [ -z "$ANDROID_BUILD_TOP" ]; then
   echo "$SCRIPT_NAME: Please set the \$ANDROID_BUILD_TOP envvar!"
   exit 1
elif [ ! -d "$ANDROID_BUILD_TOP" ]; then
   echo "$SCRIPT_NAME: \$ANDROID_BUILD_TOP does not reference a directory!"
   exit 1
fi

echo ""
echo "Files will be copied to your ANDROID_BUILD_TOP directory : $ANDROID_BUILD_TOP"
echo ""

mkdir -p $ANDROID_ROOT/tl
mkdir -p $ANDROID_ROOT/tl/$MODULE_NAME

cp Android_tester.mk    $ANDROID_ROOT/tl/$MODULE_NAME/Android.mk
cp ../src/tester.cpp   $ANDROID_ROOT/tl/$MODULE_NAME
cp ../src/OEMCrypto.h   $ANDROID_ROOT/tl/$MODULE_NAME

pushd $ANDROID_ROOT

source $ANDROID_BUILD_TOP/build/envsetup.sh
mmm tl/oemcrypto_client

mmm tl/$MODULE_NAME || exit 1
