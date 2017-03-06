#!/bin/bash
#
# Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
#
# NVFlash wrapper script for flashing Android from either build environment
# or from a BuildBrain output.tgz package. This script is not intended to be
# called directly, but from vendorsetup.sh 'flash' function or BuildBrain
# package flashing script.

# Usage:
#  flash.sh [-b <file.bct>] [-f <file.cfg>] [-o <odmdata>] [-C <cmdline>] -- [optional args]
# -C flag overrides the entire command line for nvflash, other three
# options are for explicitly specifying bct, cfg and odmdata options.
# optional arguments after '--' are added as-is to end of nvflash cmdline.

# Option precedence is as follows:
#
# 1. Command-line options (-b, -c, -o, -C) override all others.
#  (assuming there are alternative configurations to choose from:)
# 2. Shell environment variables (BOARD_IS_PM269, ENTERPRISE_A03 etc)
# 3. If shell is interactive, prompt for input from user
# 4. If shell is non-interactive, use default values

# Mandatory arguments, passed from calling scripts.
if [[ ! -x ${NVFLASH_BINARY} ]]; then
    echo "error: \${NVFLASH_BINARY} not set or not an executable file"
    exit 1
elif [[ ! -d ${PRODUCT_OUT} ]]; then
    echo "error: \${PRODUCT_OUT} not set or not a directory"
    exit 1
fi

# Optional arguments
while getopts "b:c:o:C:" OPTION
do
    case $OPTION in
    b) _bctfile=${OPTARG};
        ;;
    c) _cfgfile=${OPTARG};
        ;;
    o) _odmdata=${OPTARG};
        ;;
    C) shift ; _cmdline=$@;
        ;;
    esac
done

# Optional command-line arguments, added to nvflash cmdline as-is:
# flash -b my_flash.bct -- <args to nvflash>
shift $(($OPTIND - 1))
_args=$@

# Fetch target board name
product=$(echo ${PRODUCT_OUT%/} | grep -o '[a-zA-Z0-9]*$')

##################################
# Setup functions per target board

# Boards with modems
MDM_BOARDS=(pluto enterprise whistler)

pluto() {
    odmdata=0x40098008
    bctfile=bct_504.cfg
}

roth() {
    odmdata=0x80098000
}

kai() {
    odmdata=0x40098000
}

ventana() {
    odmdata=0x30098011
}

dalmore() {
    # Set default ODM data
    odmdata=0x80098000

    # Set internal board identifier
    [[ -n $BOARD_IS_E1613 ]] && board=e1613
    if [[ -z $board ]] && _shell_is_interactive; then
        # Prompt user for target board info
        _choose "Which Dalmore board revision to flash?" "e1611 e1613" board e1611
    else
        board=${board-e1611}
    fi

    # Set bctfile and cfgfile based on target board
    if [[ $board == e1613 ]]; then
        bctfile=flash_dalmore_e1613.cfg
    elif [[ $board == e1611 ]]; then
        bctfile=flash_dalmore_e1611.cfg
    fi
}

cardhu() {
    # Set default ODM data
    odmdata=0x40080000

    # Set internal board identifier
    [[ -n $BOARD_IS_PM269 ]] && board=pm269
    [[ -n $BOARD_IS_PM305 ]] && board=pm305
    if [[ -z $board ]] && _shell_is_interactive; then
        # Prompt user for target board info
        _choose "Which board to flash?" "cardhu pm269 pm305" board cardhu
    else
        board=${board-cardhu}
    fi

    # Set bctfile and cfgfile based on target board
    if [[ $board == pm269 ]]; then
        bctfile=bct_pm269.cfg
    elif [[ $board == pm305 ]]; then
        bctfile=bct_pm305.cfg
    elif [[ $board == cardhu ]]; then
        bctfile=bct_cardhu.cfg
    fi
}

enterprise() {
    # Set internal board identifier
    [[ -n $ENTERPRISE_A01 ]] && board=a01
    [[ -n $ENTERPRISE_A03 ]] && board=a03
    [[ -n $ENTERPRISE_A04 ]] && board=a03
    if [[ -z $board ]] && _shell_is_interactive; then
        _choose "Which Enterprise board revision to flash?" "a01 a02 a03" board a02
    else
        board=${board-a02}
    fi

    # Set bctfile, cfgfile and odmdata based on target board
    if [[ $board == a01 ]]; then
        bctfile=bct_a01.cfg
        odmdata=0x3009A000
    elif [[ $board == a02 ]]; then
        bctfile=bct_a02.cfg
        odmdata=0x3009A000
    elif [[ $board == a03 ]]; then
        bctfile=flash_a03.cfg
        odmdata=0x4009A018
    fi
}

###################
# Utility functions

# Test if we have a connected output terminal
_shell_is_interactive() { tty -s ; return $? ; }

# Test if string ($1) is found in array ($2)
_in_array() {
    local hay needle=$1 ; shift
    for hay; do [[ $hay == $needle ]] && return 0 ; done
    return 1
}

# Display prompt and loop until valid input is given
_choose() {
    _shell_is_interactive || { "error: _choose needs an interactive shell" ; exit 2 ; }
    local query="$1"                   # $1: Prompt text
    local -a choices=($2)              # $2: Valid input values
    local input=$(eval "echo \${$3}")  # $3: Variable name to store result in
    local default=$4                   # $4: Default choice
    local selected=''
    while [[ -z $selected ]] ; do
        read -e -p "$query [${choices[*]}] " -i "$default" input
        if ! _in_array "$input" "${choices[@]}"; then
            echo "error: $input is not a valid choice. Valid choices are:"
            printf ' %s\n' ${choices[@]}
        else
            selected=$input
        fi
    done
    eval "$3=$selected"
    # If predefined input is invalid, return error
    _in_array "$selected" "${choices[@]}"
}

# Set all needed parameters
_set_cmdline() {
    # Set ODM data
    if [[ -z $odmdata ]] && [[ -z $_odmdata ]]; then
        echo "error: no ODM data found or provided for target product: $product"
        exit 1
    else
        odmdata=${_odmdata-${odmdata}}
    fi

    # Set BCT and CFG files (with fallback defaults)
    bctfile=${_bctfile-${bctfile-"bct.cfg"}}
    cfgfile=${_cfgfile-${cfgfile-"flash.cfg"}}

    # Parse nvflash commandline
    cmdline=(
        --bct $bctfile
        --setbct
        --odmdata $odmdata
        --configfile $cfgfile
        --create
        --bl bootloader.bin
        --go
    )
}

###########
# Main code

# If -C is set, override all others
if [[ $_cmdline ]]; then
    cmdline=(
        $NVFLASH_BINARY
        $_cmdline
    )
# If -b, -c and -o are set, use them
elif [[ $_bctfile ]] && [[ $_cfgfile ]] && [[ $_odmdata ]]; then
    _set_cmdline
else
    # Run product function to set needed parameters
    eval $product
    _set_cmdline
fi

# Backup+restore MDM partition for boards with modems
if _in_array $product "${MDM_BOARDS[@]}" && \
[[ $PRODUCT_MDM_PARTITION != "no" ]] &&
[[ ! $_cmdline ]]; then
    cmdline=(${cmdline[@]%"--go"})
    cmdline=(
        sudo $NVFLASH_BINARY
        --read MDM MDM_${product}.img
        --bl bootloader.bin \&\&
        sudo $NVFLASH_BINARY
        --resume ${cmdline[@]} ${_args[@]} \&\&
        sudo $NVFLASH_BINARY
        --resume
        --download MDM MDM_${product}.img
        --bl bootloader.bin
        --go
    )
else
    cmdline=(sudo $NVFLASH_BINARY ${cmdline[@]} ${_args[@]})
fi

echo "INFO: PRODUCT_OUT = $PRODUCT_OUT"
echo "INFO: CMDLINE = ${cmdline[@]}"

# Execute command
(cd $PRODUCT_OUT && eval ${cmdline[@]})
exit $?
