/* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved. */
/*
** Copyright 2010, Icera Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** You may not use this file except in compliance with the License.
** You may obtain a copy of the License at:
**
** http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** imitations under the License.
*/
/*
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "icera-ril.h"
#include "icera-ril-utils.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

/* encode BCD byte to integer */
int bcdByteToInt(char a)
{
    int ret = 0;

    // treat out-of-range BCD values as 0
    if ((a & 0xf0) <= 0x90) {
        ret = (a >> 4) & 0xf;
    }

    if ((a & 0x0f) <= 0x09) {
        ret += (a & 0xf) * 10;
    }

    return ret;
}

/* encode char to hex */
char charToHex(char a)
{
    return (a < 10) ? a + '0' : a - 10 + 'A';
}

/* encode hex to char */
char hexToChar(char a)
{
    if (a >= 'a' && a <= 'f') {
        a -= 'a' - 'A';
    }

    return (a < 'A') ? a - '0' : a - 'A' + 10;
}

/* encode ascii string to hex string */
void asciiToHex(const char *input, char *output, int len)
{
    char ch;

    while (len--) {
        ch = (*input >> 4) & 0x0f;
        *output++ = charToHex(ch);
        ch = *input & 0x0f;
        *output++ = charToHex(ch);
        input++;
    }

    *output = '\0';
}

/* encode hex string to ascii string */
void hexToAscii(const char *input, char *output, int len)
{
    char ch;

    while (len--) {
        ch = hexToChar(*input++) << 4;
        ch |= hexToChar(*input++);
        *output++ = ch;
    }

    *output = '\0';
}

/* encode UCS2 string to UTF-8 string */
void UCS2ToUTF8(const char *input, char *output, int len)
{
    uint16_t ch;

    while (len--) {
        ch = *input++ << 8;
        ch |= *input++;

        if (!ch)
            break;

        if (ch < 0x00080) {
            *output++ = (char)(ch & 0xFF);
        }
        else if (ch < 0x00800) {
            *output++ = (char)(0xC0 + ((ch >> 6) & 0x1F));
            *output++ = (char)(0x80 + (ch & 0x3F));
        }
        else {
            *output++ = (char)(0xE0 + ((ch >> 12) & 0x0F));
            *output++ = (char)(0x80 + ((ch >> 6) & 0x3F));
            *output++ = (char)(0x80 + (ch & 0x3F));
        }
    }

    *output = '\0';
}

/* encode GSM string to UTF-8 string */
void GSMToUTF8(const char *input, char *output, int len)
{
    /* brute force Look up table */
    uint16_t Utf8vsGsmLookup[]=
    {
        /*00*/  0x40,       //@
        /*01*/  0xC2A3,     //£
        /*02*/  0x24,       //$
        /*03*/  0xC2A5,     //¥
        /*04*/  0xC3A8,     //è
        /*05*/  0xC3A9,     //é
        /*06*/  0xC3B9,     //ù
        /*07*/  0xC3AC,     //ì
        /*08*/  0xC3B2,     //ò
        /*09*/  0xC387,     //Ç
        /*0A*/  0x0A,       //<LF>
        /*0B*/  0xC398,     //Ø
        /*0C*/  0xC3B8,     //ø
        /*0D*/  0x0D,       //<CR>
        /*0E*/  0xC385,     //Å
        /*0F*/  0xC3A5,     //å
        /*10*/  0xCE94,     //∆
        /*11*/  0x5F,       //_
        /*12*/  0xCEA6,     //Φ
        /*13*/  0xCE93,     //Γ
        /*14*/  0xCE9B,     //Λ
        /*15*/  0xCEA9,     //Ω
        /*16*/  0xCEA0,     //Π
        /*17*/  0xCEA8,     //Ψ
        /*18*/  0xCEA3,     //Σ
        /*19*/  0xCE98,     //Θ
        /*1A*/  0xCE9E,     //Ξ
        /*1B*/  0x1B,       //<ESC>
        /*1C*/  0xC386,     //Æ
        /*1D*/  0xC3A6,     //æ
        /*1E*/  0xC39F,     //ß
        /*1F*/  0xC389,     //É
        /*20*/  0x20,       //<SP>
        /*21*/  0x21,       //!
        /*22*/  0x22,       //“
        /*23*/  0x23,       //#
        /*24*/  0xC2A4,     //¤
        /*25*/  0x25,       //%
        /*26*/  0x26,       //&
        /*27*/  0x27,       //‘
        /*28*/  0x28,       //(
        /*29*/  0x29,       //)
        /*2A*/  0x2A,       //*
        /*2B*/  0x2B,       //+
        /*2C*/  0x2C,       //,
        /*2D*/  0x2D,       //-
        /*2E*/  0x2E,       //.
        /*2F*/  0x2F,       ///
        /*30*/  0x30,       //0
        /*31*/  0x31,       //1
        /*32*/  0x32,       //2
        /*33*/  0x33,       //3
        /*34*/  0x34,       //4
        /*35*/  0x35,       //5
        /*36*/  0x36,       //6
        /*37*/  0x37,       //7
        /*38*/  0x38,       //8
        /*39*/  0x39,       //9
        /*3A*/  0x3A,       //:
        /*3B*/  0x3B,       //;
        /*3C*/  0x3C,       //<
        /*3D*/  0x3D,       //=
        /*3E*/  0x3E,       //>
        /*3F*/  0x3F,       //?
        /*40*/  0xC2A1,     //¡
        /*41*/  0x41,       //A
        /*42*/  0x42,       //B
        /*43*/  0x43,       //C
        /*44*/  0x44,       //D
        /*45*/  0x45,       //E
        /*46*/  0x46,       //F
        /*47*/  0x47,       //G
        /*48*/  0x48,       //H
        /*49*/  0x49,       //I
        /*4A*/  0x4A,       //J
        /*4B*/  0x4B,       //K
        /*4C*/  0x4C,       //L
        /*4D*/  0x4D,       //M
        /*4E*/  0x4E,       //N
        /*4F*/  0x4F,       //O
        /*50*/  0x50,       //P
        /*51*/  0x51,       //Q
        /*52*/  0x52,       //R
        /*53*/  0x53,       //S
        /*54*/  0x54,       //T
        /*55*/  0x55,       //U
        /*56*/  0x56,       //V
        /*57*/  0x57,       //W
        /*58*/  0x58,       //X
        /*59*/  0x59,       //Y
        /*5A*/  0x5A,       //Z
        /*5B*/  0xC384,     //Ä
        /*5C*/  0xC396,     //Ö
        /*5D*/  0xC391,     //Ñ
        /*5E*/  0xC39C,     //Ü
        /*5F*/  0xC2A7,     //§
        /*60*/  0xC2BF,     //¿
        /*61*/  0x61,       //a
        /*62*/  0x62,       //b
        /*63*/  0x63,       //c
        /*64*/  0x64,       //d
        /*65*/  0x65,       //e
        /*66*/  0x66,       //f
        /*67*/  0x67,       //g
        /*68*/  0x68,       //h
        /*69*/  0x69,       //i
        /*6A*/  0x6A,       //j
        /*6B*/  0x6B,       //k
        /*6C*/  0x6C,       //l
        /*6D*/  0x6D,       //m
        /*6E*/  0x6E,       //n
        /*6F*/  0x6F,       //o
        /*70*/  0x70,       //p
        /*71*/  0x71,       //q
        /*72*/  0x72,       //r
        /*73*/  0x73,       //s
        /*74*/  0x74,       //t
        /*75*/  0x75,       //u
        /*76*/  0x76,       //v
        /*77*/  0x77,       //w
        /*78*/  0x78,       //x
        /*79*/  0x79,       //y
        /*7A*/  0x7A,       //z
        /*7B*/  0xC3A4,     //ä
        /*7C*/  0xC3B6,     //ö
        /*7D*/  0xC3B1,     //ñ
        /*7E*/  0xC3BC,     //ü
        /*7F*/  0xC3A0,     //à
    };

    uint16_t ch;
    while (len--) {
        switch (*input)
        {
            /* Escape, special treatment */
            case 0x1B:
                input++;
                len--;
                switch(*input)
                {
                    case 0x10:
                        *output = 0x0C;//<FF>
                        break;
                    case 0x14:
                        *output = 0x5E;//^
                        break;
                    case 0x28:
                        *output = 0x7B;//{
                        break;
                    case 0x29:
                        *output = 0x7D;//}
                        break;
                    case 0x2F:
                        *output = 0x5C;//\'
                        break;
                    case 0x3C:
                        *output = 0x5B;//[
                        break;
                    case 0x3D:
                        *output = 0x7E;//~
                        break;
                    case 0x3E:
                        *output = 0x5D;//]
                        break;
                    case 0x40:
                        *output = 0x7C;//|
                        break;
                    case 0x65:
                        *output++ = 0xE2;//€
                        *output++ = 0x82;
                        *output = 0xAC;
                        break;
                    default:
                        /*
                         * false alert, this wasn't an escape after all
                         * Pass the esc char and rewind.
                         */ 
                        len++;
                        input--;
                        *output = 0x1B;
                        break;
                }
            break;
            default:
                ch = Utf8vsGsmLookup[(int)*input];
                /* Double char */
                if(ch >= 0x80)
                {
                    *output = (ch>>8);
                    output++;
                    ch &=0xFF;
                }
                *output = ch;
            break;
        }
        output++;
        input++;
    }
    *output = '\0';
}

/* encode numeric string to BCD string */
void numericToBCD(const char *input, char *output, int len)
{
    char ch1, ch2;

    while (len--) {
        ch1 = *input++;
        ch2 = *input++;

        if (ch1) {
            ch1 -= '0';
        }
        else {
            ch1 = 0x0F;
        }

        if (ch2) {
            ch2 -= '0';
        }
        else {
            ch2 = 0x0F;
        }

        *output++ = (ch1 | (ch2 << 4));
    }

    *output = '\0';
}
