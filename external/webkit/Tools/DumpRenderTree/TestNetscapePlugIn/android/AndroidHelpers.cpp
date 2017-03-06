/*
 * Copyright (C) 2012 NVIDIA. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "AndroidHelpers.h"

#include "PluginObject.h"


ANPNativeWindowInterfaceV0 gNativeWindowInterface;
ANPWindowInterfaceV2 gWindowInterface;

char charForANPKeyCode(ANPKeyCode nativeCode)
{
    switch (nativeCode){
    k0_ANPKeyCode:
        return '0';
    k1_ANPKeyCode:
        return '1';
    k2_ANPKeyCode:
        return '2';
    k3_ANPKeyCode:
        return '3';
    k4_ANPKeyCode:
        return '4';
    k5_ANPKeyCode:
        return '5';
    k6_ANPKeyCode:
        return '6';
    k7_ANPKeyCode:
        return '7';
    k8_ANPKeyCode:
        return '8';
    k9_ANPKeyCode:
        return '9';
    kStar_ANPKeyCode:
        return '*';
    kPound_ANPKeyCode:
        return '#';
    kA_ANPKeyCode:
        return 'a';
    kB_ANPKeyCode:
        return 'b';
    kC_ANPKeyCode:
        return 'c';
    kD_ANPKeyCode:
        return 'd';
    kE_ANPKeyCode:
        return 'e';
    kF_ANPKeyCode:
        return 'f';
    kG_ANPKeyCode:
        return 'g';
    kH_ANPKeyCode:
        return 'h';
    kI_ANPKeyCode:
        return 'i';
    kJ_ANPKeyCode:
        return 'j';
    kK_ANPKeyCode:
        return 'k';
    kL_ANPKeyCode:
        return 'l';
    kM_ANPKeyCode:
        return 'm';
    kN_ANPKeyCode:
        return 'n';
    kO_ANPKeyCode:
        return 'o';
    kP_ANPKeyCode:
        return 'p';
    kQ_ANPKeyCode:
        return 'q';
    kR_ANPKeyCode:
        return 'r';
    kS_ANPKeyCode:
        return 's';
    kT_ANPKeyCode:
        return 't';
    kU_ANPKeyCode:
        return 'u';
    kV_ANPKeyCode:
        return 'v';
    kW_ANPKeyCode:
        return 'w';
    kX_ANPKeyCode:
        return 'x';
    kY_ANPKeyCode:
        return 'y';
    kZ_ANPKeyCode:
        return 'z';
    kComma_ANPKeyCode:
        return '.';
    kPeriod_ANPKeyCode:
        return '.';
    kGrave_ANPKeyCode:
        return '`';
    kMinus_ANPKeyCode:
        return '-';
    kEquals_ANPKeyCode:
        return '=';
    kLeftBracket_ANPKeyCode:
        return '[';
    kRightBracket_ANPKeyCode:
        return ']';
    kBackslash_ANPKeyCode:
        return '\\';
    kSemicolon_ANPKeyCode:
        return ';';
    kApostrophe_ANPKeyCode:
        return '\'';
    kSlash_ANPKeyCode:
        return '/';
    kAt_ANPKeyCode:
        return '@';
    kPlus_ANPKeyCode:
        return '+';
    kNumPad0_ANPKeyCode:
        return '0';
    kNumPad1_ANPKeyCode:
        return '1';
    kNumPad2_ANPKeyCode:
        return '2';
    kNumPad3_ANPKeyCode:
        return '3';
    kNumPad4_ANPKeyCode:
        return '4';
    kNumPad5_ANPKeyCode:
        return '5';
    kNumPad6_ANPKeyCode:
        return '6';
    kNumPad7_ANPKeyCode:
        return '7';
    kNumPad8_ANPKeyCode:
        return '8';
    kNumPad9_ANPKeyCode:
        return '9';
    kNumPadDivide_ANPKeyCode:
        return '/';
    kNumPadMultiply_ANPKeyCode:
        return '*';
    kNumPadSubtract_ANPKeyCode:
        return '-';
    kNumPadAdd_ANPKeyCode:
        return '+';
    kNumPadDot_ANPKeyCode:
        return '.';
    kNumPadComma_ANPKeyCode:
        return ',';
    kNumPadEnter_ANPKeyCode:
    default:
        break;
    }
    return ' ';
}

NPError initAndroidInterfaces(NPP instance)
{
    gNativeWindowInterface.inSize = sizeof(gNativeWindowInterface);
    NPError err = browser->getvalue(instance, kNativeWindowInterfaceV0_ANPGetValue, &gNativeWindowInterface);

    if (err != NPERR_NO_ERROR)
        return err;

    gWindowInterface.inSize = sizeof(gWindowInterface);
    err = browser->getvalue(instance, kWindowInterfaceV2_ANPGetValue, &gWindowInterface);

    if (err != NPERR_NO_ERROR)
        return err;

    return NPERR_NO_ERROR;
}

