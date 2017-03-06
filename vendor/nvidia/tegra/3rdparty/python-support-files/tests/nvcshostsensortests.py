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

class NvCSHostSensorTest(NvCSTestBase):
    "Host Sensor Basic Test"

    iterationPerImage = 10

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Host_Sensor")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runTest(self, args):

        rawImagePathList = []

        # capture three raw images
        for i in [1, 2, 3]:
            self.obCamera.startPreview()
            # set the file name
            fileName = "out_%s_test_%d" % (self.testID, i)
            jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")
            rawFilePath = os.path.join(self.testDir, fileName + ".nvraw")
            rawImagePathList.append(rawFilePath)

            self.obCamera.captureConcurrentJpegAndRawImage(jpegFilePath)

            if (os.path.exists(rawFilePath) != True):
                self.logger.error("Couldn't capture the image: %s" % rawFilePath)
                return NvCSTestResult.ERROR
            self.obCamera.stopPreview()

        # stop the graph running by default
        self.obGraph.stopAndDeleteGraph()

        # iterate through images captured
        for rawImagePath in rawImagePathList:
            # HACK - random wait
            # sleep for two seconds before next iteration
            # graph execution fails if we don't do this.
            time.sleep(2)

            # ---- Read and load raw file ---- #
            if self.nvrf.readFile(rawImagePath):
                header = self.nvrf.makeLegacyHeader()

                # ---- setup the host sensor graph ---- #
                self.obGraph.setGraphType(GraphType.HOST_SENSOR)
                self.obGraph.setStillParams(self.nvrf._width, self.nvrf._height)
                self.obGraph.createAndRunGraph()

                # enable ANR
                self.obCamera.setAttr(nvcamera.attr_anr, 1)

                for i in range(self.iterationPerImage):
                    # set the exposure time from the raw file
                    self.logger.debug("Setting the exposure time to: %f" % self.nvrf._exposureTime)
                    self.obCamera.setAttr(nvcamera.attr_exposuretime, self.nvrf._exposureTime)

                    # set the iso from raw file
                    self.logger.debug("Setting the iso to: %d" % self.nvrf._iso)
                    self.obCamera.setAttr(nvcamera.attr_iso, self.nvrf._iso)

                    # set the file name
                    (fileName, ext) = os.path.splitext(rawImagePath)

                    # append the image number at the end
                    fileName += "_%d" % i
                    jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")

                    # pass the raw image header, data and iteration information
                    self.obCamera.setRawImage(header, self.nvrf._pixelData, i)
                    self.obCamera.captureJpegImage(jpegFilePath)

                    if (os.path.exists(jpegFilePath) != True):
                        self.logger.error("Couldn't capture the jpeg image: %s" % jpegFilePath)
                        return NvCSTestResult.ERROR

                self.obGraph.stopAndDeleteGraph()
            else:
                self.logger.error("Couldn't open the raw image: %s" % rawImagePath)
                return NvCSTestResult.ERROR
        return NvCSTestResult.SUCCESS

