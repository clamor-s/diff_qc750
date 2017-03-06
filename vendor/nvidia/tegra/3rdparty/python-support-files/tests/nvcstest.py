#-------------------------------------------------------------------------------
# Name:        nvcstest.py
# Purpose:
#
# Created:     01/23/2012
#
# Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
#-------------------------------------------------------------------------------
#!/usr/bin/env python
import nvcamera
import string
import os
import time
import shutil
import math
import traceback
import re
import nvrawfile
import nvcstestutils
from nvcssensortests import *
from nvcsflashtests import *
from nvcsfocusertests import *
from nvcshostsensortests import *
from nvcstestcore import *

# nvcstest module version
__version__ = "4.0.0"

def printVersionInformation():
    # print nvcstest version
    print "nvcstest version: %s" % __version__

    # print nvcamera module /nvcs version
    print "nvcamera version: %s" % nvcamera.__version__

class NvCSJPEGCapTest(NvCSTestBase):
    "JPEG Capture Test"

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Jpeg_Capture")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        print "capturing jpeg image..."

        self.obCamera.startPreview()

        fileName = "out_%s_test" % self.testID
        jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")
        self.obCamera.captureJpegImage(jpegFilePath)

        self.obCamera.stopPreview()

        if (os.path.exists(jpegFilePath) != True):
            self.logger.error("Couldn't capture the jpeg image: %s" % jpegFilePath)
            retVal = NvCSTestResult.ERROR

        return NvCSTestResult.SUCCESS

class NvCSMultipleRawTest(NvCSTestBase):
    "Multiple Raw Image Test"

    imageNumValues = range(1, 101)

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Multiple_Raw")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def __del__(self):
        if (os.path.exists(self.testDir)):
            shutil.rmtree(self.testDir)

    def runPreTest(self):
        if (not self.obCamera.isRawCaptureSupported()):
            self.logger.info("raw capture is not supported")
            return NvCSTestResult.SKIPPED
        else:
            return NvCSTestResult.SUCCESS

    def runTest(self, args):
        retVal = NvCSTestResult.SUCCESS

        for imageNum in self.imageNumValues:
            # take an image
            fileName = "out_%s_%d_test" % (self.testID, imageNum)
            jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")
            rawfilePath = os.path.join(self.testDir, fileName + ".nvraw")

            self.obCamera.startPreview()

            self.logger.debug("capturing image: %d" % imageNum)
            self.obCamera.captureConcurrentJpegAndRawImage(jpegFilePath)

            self.obCamera.stopPreview()

            if os.path.exists(rawfilePath) != True:
                self.logger.error("couldn't find file: %s" % rawfilePath)
                retVal = NvCSTestResult.ERROR
                break

            if not self.nvrf.readFile(rawfilePath):
                self.logger.error("couldn't open the file: %s" % rawfilePath)
                retVal = NvCSTestResult.ERROR

            pattern = ".*"
            if((imageNum % 10) == 0):
                regexOb = re.compile(pattern)
                for fname in os.listdir(self.testDir):
                    if regexOb.search(fname):
                        self.logger.debug("removing file %s" % fname)
                        os.remove(os.path.join(self.testDir, fname))

        return retVal

class NvCSRAWCapTest(NvCSTestBase):
    "RAW Capture Test"

    def __init__(self, options):
        NvCSTestBase.__init__(self, "RAW_Capture")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)
        self.obGraph.setGraphType(GraphType.RAW)

        return NvCSTestResult.SUCCESS

    def runTest(self, args):
        retVal = NvCSTestResult.SUCCESS

        self.obCamera.startPreview()

        fileName = "out_%s_test" % self.testID
        filePath = os.path.join(self.testDir, fileName + ".nvraw")

        try:
            self.obCamera.captureRAWImage(filePath)
        except nvcamera.NvCameraException, err:
            if (err.value == nvcamera.NvError_NotSupported):
                self.logger.info("raw capture is not supported")
                return NvCSTestResult.SKIPPED
            else:
                raise

        self.obCamera.stopPreview()

        if (os.path.exists(filePath) != True):
            self.logger.error("Couldn't capture the raw image: %s" % filePath)
            retVal = NvCSTestResult.ERROR

        return retVal

class NvCSConcurrentRawCapTest(NvCSTestBase):
    "Concurrent raw capture test"

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Concurrent_Raw_Capture")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runPreTest(self):
        if (not self.obCamera.isRawCaptureSupported()):
            self.logger.info("raw capture is not supported")
            return NvCSTestResult.SKIPPED
        else:
            return NvCSTestResult.SUCCESS

    def runTest(self, args):
        retVal = NvCSTestResult.SUCCESS

        self.obCamera.startPreview()

        fileName = "out_%s_test" % self.testID
        jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")
        rawFilePath = os.path.join(self.testDir, fileName + ".nvraw")

        self.obCamera.captureConcurrentJpegAndRawImage(jpegFilePath)

        self.obCamera.stopPreview()

        if (os.path.exists(rawFilePath) != True):
            self.logger.error("Couldn't capture the raw image: %s" % rawFilePath)
            retVal = NvCSTestResult.ERROR

        if not self.nvrf.readFile(rawFilePath):
            self.logger.error("couldn't open the file: %s" % rawFilePath)
            retVal = NvCSTestResult.ERROR

        # Check no values over 2**bitsPerSample
        if (max(self.nvrf._pixelData.tolist()) > (2**self.nvrf._bitsPerSample)):
            self.logger.error("pixel value is over %d." % 2**self.nvrf._bitsPerSample)
            retVal = NvCSTestResult.ERROR

        return retVal

class NvCSPropertyTest(NvCSTestBase):
    "Property verification test"

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Property_Verification")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        # take an image with contrast
        print "capturing jpeg image with contrast value %d..." % 100
        self.obCamera.setAttr(nvcamera.attr_contrast, 50)
        self.captureJpegImage("out_%s_%s_%d_test" % (self.testID, "contrast", 100))

        # take an image with brightness
        self.obCamera.setAttr(nvcamera.attr_brightness, 14.0)
        print "capturing jpeg image with brightness value %d..." % 50.0
        self.captureJpegImage("out_%s_%s_%d_test" % (self.testID, "brightness", 50.0))

        # take an image with max (5.0) saturation
        print "capturing jpeg image with saturation value %d..." % 0.0
        self.obCamera.setAttr(nvcamera.attr_saturation, 5.0)
        self.captureJpegImage("out_%s_%s_%f_test" % (self.testID, "saturation", 0.0))
        return NvCSTestResult.SUCCESS

class NvCSEffectTest(NvCSTestBase):
    "Effect verification test"
    effectValues = ["None",
                    "Noise",
                    "Emboss",
                    "Negative",
                    "Sketch",
                    "OilPaint",
                    "Hatch",
                    "Gpen",
                    "Antialias",
                    "DeRing",
                    "Solarize",
                    "Posterize",
                    "Sepia",
                    "BW"
                  ]
    def __init__(self, options):
        NvCSTestBase.__init__(self, "Effect_Verification")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        for effectVal in self.effectValues:
            # take an image with specified effect
            print "capturing image with %s effect..." % effectVal
            self.obCamera.setAttr(nvcamera.attr_effect, effectVal)
            # take an image
            self.captureJpegImage("out_%s_%s_test" % (self.testID, effectVal))
        return NvCSTestResult.SUCCESS

class NvCSFlickerTest(NvCSTestBase):
    "Flicker verification test"

    flickerValues = ["off", "auto", "50", "60"]
    def __init__(self, options):
        NvCSTestBase.__init__(self, "Flicker")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        for flickerVal in self.flickerValues:
            # take an image with specified Flicker effect
            print "capturing jpeg image flicker value %s..." % flickerVal
            self.obCamera.setAttr(nvcamera.attr_flicker, flickerVal)
            # take an image
            self.captureJpegImage("out_%s_%s_test" % (self.testID, flickerVal))
        return NvCSTestResult.SUCCESS

class NvCSAWBTest(NvCSTestBase):
    "Auto White Balance test"
    awbValues = ["Off",
                 "Auto",
                 "Tungsten",
                 "Fluorescent",
                 "Sunny",
                 "Shade",
                 "Cloudy",
                 "Incandescent",
                 "Flash",
                 "Horizon"]

    def __init__(self, options):
        NvCSTestBase.__init__(self, "AWB")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        # take an image with specified AWB setting
        for awbValue in self.awbValues:
            print "capturing jpeg image with AWB set to %s..." % awbValue
            self.obCamera.setAttr(nvcamera.attr_whitebalance, awbValue)
            # take an image
            self.captureJpegImage("out_%s_%s_test" % (self.testID, awbValue))
        return NvCSTestResult.SUCCESS

class NvCSMeteringTest(NvCSTestBase):
    "Metering test"
    meteringmodeValues = ["center", "spot", "fullframe"]

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Metering")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        # take an image with specified meteringmode value
        for meteringMode in self.meteringmodeValues:
            print "capturing jpeg image with Metering set to %s..." % meteringMode
            self.obCamera.setAttr(nvcamera.attr_meteringmode, meteringMode)
            # take an image
            self.captureJpegImage("out_%s_%s_test" % (self.testID, meteringMode))
        return NvCSTestResult.SUCCESS

class NvCSAFTest(NvCSTestBase):
    "AF test"
    afValues = [-1, 0]
    def __init__(self, options):
        NvCSTestBase.__init__(self, "AF")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        for afVal in self.afValues:
            # take an image with specified AF value
            print "capturing jpeg image with AF set to %d..." % afVal
            self.obCamera.setAttr(nvcamera.attr_autofocus, afVal)
            self.startPreview()
            # take an image
            self.captureJpegImage("out_%s_%d_test" % (self.testID, afVal))
        return NvCSTestResult.SUCCESS

class NvCSAutoFrameRateTest(NvCSTestBase):
    "FPS test"

    def __init__(self, options):
        NvCSTestBase.__init__(self, "FPS")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        # take an image with 0 AFR
        self.obCamera.setAttr(nvcamera.attr_autoframerate, 0)
        self.startPreview()
        # take an image
        self.captureJpegImage("out_%s_%d_test" % (self.testID, 0))
        return NvCSTestResult.SUCCESS

class NvCSJpegRotationTest(NvCSTestBase):
    "JPEG Rotation test"

    rotationValues = ["None", "Rotate90", "Rotate180", "Rotate270"]

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Jpeg_Rotation")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):
        for rotationVal in self.rotationValues:
            # take an image specified rotation
            print "capturing jpeg image with rotation set to %s..." % rotationVal
            self.obCamera.setAttr(nvcamera.attr_transform, rotationVal)
            self.startPreview()
            # take an image
            self.captureJpegImage("out_%s_%s_test" % (self.testID, rotationVal))
        return NvCSTestResult.SUCCESS
