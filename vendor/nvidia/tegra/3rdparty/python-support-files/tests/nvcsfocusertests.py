# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

from nvcstestcore import *
import nvcstestutils

class NvCSFocusPosTest(NvCSTestBase):
    "Focus Position test"

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Focus_Position")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runPreTest(self):
        # check if the focuser is supported
        if(not self.obCamera.isFocuserSupported()):
            return NvCSTestResult.SKIPPED

        if (not self.obCamera.isRawCaptureSupported()):
            self.logger.info("raw capture is not supported")
            return NvCSTestResult.SKIPPED
        else:
            return NvCSTestResult.SUCCESS

    def runTest(self, args):
        retVal = NvCSTestResult.SUCCESS

        # query+print focus position physical range
        physicalRange = self.obCamera.getAttr(nvcamera.attr_focuspositionphysicalrange)
        self.logger.info("focuser physical range: [%d, %d]" % (physicalRange[0], physicalRange[1]))

        # calculate working range
        positionInf = self.obCamera.getAttr(nvcamera.attr_focuspositioninf)
        positionInfOffset = self.obCamera.getAttr(nvcamera.attr_focuspositioninfoffset)
        positionmMacro = self.obCamera.getAttr(nvcamera.attr_focuspositionmacro)
        positionmMacroOffset = self.obCamera.getAttr(nvcamera.attr_focuspositionmacrooffset)

        workingPositionLow = positionInf + positionInfOffset
        workingPositionHigh = positionmMacro + positionmMacroOffset
        self.logger.info("focuser working range: [%d, %d]" % (workingPositionLow, workingPositionHigh))

        # check for unusual values and flag warning
        if(physicalRange[0] > physicalRange[1]):
            self.logger.warning("focus position infinity is larger than focus position macro")
            self.logger.warning("focus position infinity: %d, focus position macro: %d" %
                                (physicalRange[0], physicalRange[1]))

        if((physicalRange[1] - physicalRange[0]) > 1023):
            self.logger.warning("Total focuser steps which is %d are larger than 1023" %
                                (physicalRange[1] - physicalRange[0]))

        startFocusPosition =  physicalRange[0]
        stopFocusPosition = physicalRange[1]
        step = (physicalRange[1] - physicalRange[0])/5

        while(startFocusPosition <= stopFocusPosition):
            focusPos = int(startFocusPosition)
            self.logger.info("capturing jpeg image with focuspos set to %d..." % focusPos)

            self.obCamera.startPreview()

            self.obCamera.setAttr(nvcamera.attr_focuspos, focusPos)

            # take an image
            fileName = "out_%s_%d_test" % (self.testID, focusPos)
            jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")
            rawFilePath = os.path.join(self.testDir, fileName + ".nvraw")

            self.obCamera.captureConcurrentJpegAndRawImage(jpegFilePath)

            self.obCamera.stopPreview()

            if not self.nvrf.readFile(rawFilePath):
                self.logger.error("couldn't open the file: %s" % rawFilePath)
                retVal = NvCSTestResult.ERROR
                break

            if (self.nvrf._focusPosition !=  focusPos):
                self.logger.error("FocusPosition value is not correct in the raw header: %d" % self.nvrf._focusPosition)
                retVal = NvCSTestResult.ERROR
                break

            focusPosVal = self.obCamera.getAttr(nvcamera.attr_focuspos)
            if (focusPos != focusPosVal):
                self.logger.error("focus position value is not correct in the driver: %d..." % focusPosVal)
                retVal = NvCSTestResult.ERROR
                break

            startFocusPosition = startFocusPosition + step

        return retVal

class NvCSSharpnessTest(NvCSTestBase):

    # crop dimentions for sharpness calculations
    # This should select cropWidth x cropHeight image from the center of the image.
    # If the image dimensions are lower than crop size then we re-adjust
    # the crop size to match image dimensions.
    cropWidth = 600
    cropHeight = 600

    # sharpness error margin
    errMargin = 10.0/100.0

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Sharpness")
        self.options = options

    def needTestSetup(self):
        return True

    def getSetupString(self):
        return "This test requires camera to be pointed at the target"

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runPreTest(self):
        # check if the focuser is supported
        if(not self.obCamera.isFocuserSupported()):
            return NvCSTestResult.SKIPPED

        if (not self.obCamera.isRawCaptureSupported()):
            self.logger.info("raw capture is not supported")
            return NvCSTestResult.SKIPPED
        else:
            return NvCSTestResult.SUCCESS

    def runTest(self, args):
        retVal = NvCSTestResult.SUCCESS

        # query+print focus position physical range
        physicalRange = self.obCamera.getAttr(nvcamera.attr_focuspositionphysicalrange)
        self.logger.info("focuser physical range: [%d, %d]" % (physicalRange[0], physicalRange[1]))

        # capture images at min, (max-min)/2 and max focus positions
        focusPositions = [physicalRange[0], int((physicalRange[1]+physicalRange[0])/2), physicalRange[1]]

        previousSharpness = 0
        previousFocusPosition = physicalRange[0]
        sharpnessDictForward = {}
        sharpnessDictBackward = {}
        for focusPosition in focusPositions:
            currentSharpness = self._getSharpnessAtFocusPosition(focusPosition)

            sharpnessDictForward[focusPosition] = currentSharpness

            # check if the current sharpness is within the 10%
            # range of previous sharpness and fail the test
            minCheckSharpness = previousSharpness - (previousSharpness * self.errMargin)
            maxCheckSharpness = previousSharpness + (previousSharpness * self.errMargin)

            if (currentSharpness >= minCheckSharpness and
                currentSharpness <= maxCheckSharpness ):
                self.logger.error("sharpness has not changed as expected after we moved focuser from %d to %d" %
                                 (previousFocusPosition, focusPosition))
                self._printSummary(focusPositions, sharpnessDictForward, sharpnessDictBackward)
                return NvCSTestResult.ERROR

            previousSharpness = currentSharpness

        # print focusPosition vs sharpness
        # move focuser backward now using previous positions
        for focusPosition in focusPositions[::-1]:
            currentSharpness = self._getSharpnessAtFocusPosition(focusPosition)
            sharpnessDictBackward[focusPosition] = currentSharpness

            # check if the shapness matches(~ within 10% difference) with the previous
            # sharpness measurements
            minCheckSharpness = sharpnessDictForward[focusPosition] - \
                               (sharpnessDictForward[focusPosition] * self.errMargin)
            maxCheckSharpness = sharpnessDictForward[focusPosition] + \
                               (sharpnessDictForward[focusPosition] * self.errMargin)

            if (currentSharpness <= minCheckSharpness or
                currentSharpness >= maxCheckSharpness):
                self.logger.error("sharpness is not the same as the previous measurement for focus position %d" %
                                  focusPosition)
                self._printSummary(focusPositions, sharpnessDictForward, sharpnessDictBackward)
                return NvCSTestResult.ERROR

        # print the summary information
        self._printSummary(focusPositions, sharpnessDictForward, sharpnessDictBackward)

        return retVal

    def _printSummary(self, focusPositions, sharpnessDictForward, sharpnessDictBackward):
        # print the information
        self.logger.info("%20s %20s %20s" % ("Focus Position", "Sharpness", "Measurement Order"))
        for focusPosition in focusPositions:
            if focusPosition in sharpnessDictForward:
                self.logger.info("%20d %20.3f %20s" % (focusPosition,
                                                       sharpnessDictForward[focusPosition],
                                                       "Forward"
                                                      ))
            if focusPosition in sharpnessDictBackward:
                self.logger.info("%20d %20.3f %20s" % (focusPosition,
                                                       sharpnessDictBackward[focusPosition],
                                                       "Backward"
                                                      ))

    def _getSharpnessAtFocusPosition(self, focusPosition):
        rawFilePath = self._captureImageAtFocusPosition(focusPosition)

        if(not os.path.exists(rawFilePath)):
            self.logger.error("raw file %s is not present" % rawFilePath)
            return NvCSTestResult.ERROR

        # open the rawfile
        if not self.nvrf.readFile(rawFilePath):
            self.logger.error("couldn't open the file: %s" % rawFilePath)
            return NvCSTestResult.ERROR

        # crop the raw image before we calculate sharpness
        # this is for speedup
        retVal = nvcameraimageutils.cropRawImageFromCenter(self.nvrf, self.cropWidth, self.cropHeight)
        if(not retVal):
            self.logger.error("Unable to crop the image %s for crop width %d, height: %d " %
                              (rawFilePath, self.cropWidth, self.cropHeight))

        # get the sharpness
        sharpness = nvcameraimageutils.calculateSharpness(self.nvrf)
        return sharpness

    def _captureImageAtFocusPosition(self, focusPos):
        self.obCamera.startPreview()
        # set focus position
        self.obCamera.setAttr(nvcamera.attr_focuspos, focusPos)

        if focusPos < 0:
            negStr = "_neg"
        else:
            negStr = ""

        # take an image
        fileName = "out_%s%s_%d_test" % (self.testID, negStr ,abs(focusPos))
        jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")

        # HACKY way to appned "_1" to the the filename
        # if file already exists
        if(os.path.exists(jpegFilePath)):
            fileName = fileName + "_1"

        jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")
        rawFilePath = os.path.join(self.testDir, fileName + ".nvraw")

        # capture concurrent jpeg+raw image
        self.obCamera.captureConcurrentJpegAndRawImage(jpegFilePath)
        self.obCamera.stopPreview()

        return rawFilePath
