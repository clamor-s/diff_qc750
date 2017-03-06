#!/bin/bash

cd `dirname $0`
echo "Generate TF include"
echo 

SCRIPT_NAME=`basename $0`


# Check optional env var.
function check_env_var()
{
echo "Check $2"
if [ -z "$1" ]; then
   echo "$SCRIPT_NAME: Missing $2 envvar!"
   exit 1
fi
}

check_env_var "$JAVA_HOME"  '$JAVA_HOME'

#Check command line parameter, else display help message.
if [ -z "$1" ]||[ -n "$2" ];then
    echo This script will generate an include header file with inside the binary code of the Trusted Foundations.
    echo Usage :
    echo $0 [PATH_TO_TRUSTED_FOUNDATIONS_BINARY]    
    exit 1
fi  

TF_BIN_NAME=./trusted_foundations.bin
cp -v  $1 $TF_BIN_NAME
$JAVA_HOME/bin/java -jar ../bin2src.jar -verbose -c99 -header ../bin2src_header.txt -o ./tf_include.h $TF_BIN_NAME
rm $TF_BIN_NAME

echo   