#
# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
#!/usr/bin/env python

import array
import nvrawfile
import math
import time

################################# sharpness #################################
def calculateSharpness(nvrf):
    # nvrf: NvRawFile object
    sharpness = 0

    # check if we are able to open the nvraw file
    #print "raw file width:%d, height:%d" % (nvrf._width, nvrf._height)

    # get pixels overlapping with RGGB pattern
    [lumaPixelData, width, height] = _convertRawToY(nvrf)

    sharpness = _sharpnessMeasure_Apply5x5Filter(
                                                 lumaPixelData,
                                                 width,
                                                 height
                                                )
    return sharpness

def _convertRawToY(nvrf):
    # converts 16bit raw pixels into
    # 8 bit Y Luma pixels
    start_time = time.clock()

    width = nvrf._width
    height = nvrf._height
    bayerPhase = nvrf._bayerPhase
    pixelData = nvrf._pixelData

    indexOfR = bayerPhase.index('R')

    if(indexOfR == 0):
        start_row = 0
        start_col = 0
    elif(indexOfR == 1):
        start_row = 0
        start_col = 1
    elif(indexOfR == 2):
        start_row = 1
        start_col = 0
    elif(indexOfR == 3):
        start_row = 1
        start_col = 1

    yPixels = array.array('d')

    # remove the end since it is filtered with zeros
    # take pixels overlapping with RGGB pattern
    # convert to luma at the same time
    for i in range(start_row, height-1, 2):
        next_row = (i+1)

        for j in range(start_col, width-1, 2):
            next_col = (j+1)

            r  = pixelData[(i*width) + j]
            gr = pixelData[(i*width) + next_col]
            gb = pixelData[(next_row*width) + j]
            b  = pixelData[(next_row*width) + next_col]

            val = (0.299*r + 0.2935*(gr+gb) + 0.114*b)
            yPixels.append(val)

    width = int(math.ceil((width - start_col)/ 2))
    height = int(math.ceil((height - start_row) / 2))

    end_time = time.clock()
    #print "convertRawToY total time: %.3gs" % (end_time - start_time)

    return [yPixels, width, height]

def _sharpnessMeasure_Apply5x5Filter(
                                     input_image,    # 10-bit Y input image data
                                     width,          # input image dimension : width and height
                                     height
                                    ):

    start_time = time.clock()
    sharpness = []
    cy = 0
    cx = 0
    tt = 0
    bb = 0

    filter = [ 0,  -1,  -1,   -1,   0,
              -1,   1,   1.5,  1,  -1,
              -1,  1.5,  2,   1.5, -1,
              -1,   1,   1.5,  1,  -1,
               0,  -1,  -1,   -1,   0 ]

    # Apply the filter
    #              ll  l  cx   r   rr
    #            +---+---+---+---+---+
    #        tt  |   |   |   |   |   |
    #            +---+---+---+---+---+
    #        t   |   |   |   |   |   |
    #            +---+---+---+---+---+
    #        cy  |   |   | x |   |   |
    #            +---+---+---+---+---+
    #        b   |   |   |   |   |   |
    #            +---+---+---+---+---+
    #        bb  |   |   |   |   |   |
    #            +---+---+---+---+---+
    #print "applying sobel operator to image of width:%d, height:%d" % (width, height)

    for cy in range(2,height-2):
        t = cy - 1
        b = cy + 1
        tt = cy - 2
        bb = cy + 2

        for cx in range(2, width-2):

            l = cx - 1
            r = cx + 1
            ll = cx - 2
            rr = cx + 2

            # 5x5 filter
            val = -input_image[(tt*width) + l ]    \
                  - input_image[(tt*width) + cx]   \
                  - input_image[(tt*width) + r ] + \
                  \
                  -input_image[(t *width) + ll] + input_image[(t *width) + l ] + \
                  filter[ 7]*input_image[(t *width) + cx] +                      \
                  input_image[(t *width) + r ] - input_image[(t *width) + rr] +  \
                  \
                  -input_image[(cy*width) + ll] + filter[11]*input_image[(cy*width) + l ] +\
                  filter[12]*input_image[(cy*width) + cx] +                                \
                  filter[13]*input_image[(cy*width) + r ] -input_image[(cy*width) + rr] +  \
                  \
                  -input_image[(b *width) + ll] + input_image[(b *width) + l ] + \
                  filter[17]*input_image[(b *width) + cx] +                      \
                  input_image[(b *width) + r ] -input_image[(b *width) + rr] +   \
                  \
                  -input_image[(bb*width) + l ]  \
                  - input_image[(bb*width) + cx] \
                  - input_image[(bb*width) + r ]

            # Compute absolute value of val
            val = math.fabs(val)

            # Add val to focus measure
            sharpness.append(val)


    end_time = time.clock()
    #print "sharpnessMeasure_Apply5x5Filter total time: %.3gs" % (end_time - start_time)
    return math.fsum(sharpness)

################################# cropping #################################

def cropRawImageFromCenter(nvrf, cropWidth, cropHeight):
    # crop the raw file from center: width x height crop

    #print "raw file width:%d, height:%d" % (nvrf._width, nvrf._height)
    if(cropWidth > nvrf._width):
        cropWidth = nvrf._width
    if(cropHeight > nvrf._height):
        cropHeight = nvrf._height

    pixelDataList = nvrf._pixelData.tolist()

    #print "original pixelData length: %d" % len(pixelDataList)

    x = int(math.floor(nvrf._width/2) - math.floor(cropWidth/2))
    y = int(math.floor(nvrf._height/2) - math.floor(cropHeight/2))

    #print "startIndex: %d" % (x*nvrf._width+y)
    # update bayerPhase field based on (x,y) values
    nvrf._bayerPhase = _getBayerPhaseAtRowAndCol(nvrf._bayerPhase, x, y)

    # empty raw file pixelData
    nvrf._pixelData = array.array('h')
    for i in range(cropHeight):
        # get width pixel data
        startIndex = int(((y + i) * nvrf._width) + x)
        endIndex = startIndex + cropWidth

        nvrf._pixelData.fromlist(pixelDataList[startIndex:endIndex])

    nvrf._width = cropWidth
    nvrf._height = cropHeight
    return True

def _getBayerPhaseAtRowAndCol(currentBayerPhase, i, j):
    "gets the bayer phase at given row and column using current bayer phase"

    odd_row = i & 0x1
    odd_col = j & 0x1

    bayerPhase = currentBayerPhase
    if(currentBayerPhase == "RGGB"):
        # R GR GB B
        # pixel at !odd_col and !odd_row is R
        if(not odd_row and odd_col):
            bayerPhase = "GRBG"
        elif(odd_row and not odd_col):
            bayerPhase = "GBRG"
        elif(odd_row and odd_col):
            bayerPhase = "BGGR"
    elif(currentBayerPhase == "GRBG"):
        # GR R B GB
        # pixel at !odd_col and !odd_row is G
        if(not odd_row and odd_col):
            bayerPhase = "RGGB"
        elif(odd_row and not odd_col):
            bayerPhase = "BGGR"
        elif(odd_row and odd_col):
            bayerPhase = "GBRG"
    elif(currentBayerPhase == "GBRG"):
        # GB B R GR
        if(not odd_row and odd_col):
            bayerPhase = "BGGR"
        elif(odd_row and not odd_col):
            bayerPhase = "RGGB"
        elif(odd_row and odd_col):
            bayerPhase = "GRBG"
    else:
        # B  GB GR R
        if(not odd_row and odd_col):
            bayerPhase = "GBRG"
        elif(odd_row and not odd_col):
            bayerPhase = "GRBG"
        elif(odd_row and odd_col):
            bayerPhase = "RGGB"

    return bayerPhase
