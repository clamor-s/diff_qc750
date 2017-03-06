# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

import traceback
import os.path
import nvrawfile
import shutil
import math
import time

import nvcamera
import nvcstestutils
import nvcameraimageutils

class NvCSTestResult(object):
    SUCCESS = 1
    SKIPPED = 2
    ERROR = 3

class GraphState(object):
    STOPPED = 1
    PAUSED = 2
    RUNNING = 3
    CLOSED = 4

class GraphType(object):
    JPEG = 1
    RAW = 2
    HOST_SENSOR = 3

class NvCSTestException(Exception):
    "NvCSTest Exception Class"

    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class NvCSTestGraph(object):
    "NvCSTest Graph wrapper class"

    _imagerID = 0
    _state = GraphState.CLOSED
    _stillWidth = 0
    _stillHeight = 0
    _previewWidth = 0
    _previewHeight = 0
    _hostSensorWidth = 0
    _hostSensorHeight = 0
    _graphType = GraphType.JPEG
    _obGraph = None
    logger = None

    def __init__(self, graphType=GraphType.JPEG):
        self.graphType = graphType
        self._obGraph = nvcamera.Graph()
        self._state = GraphState.CLOSED
        self.logger = nvcstestutils.Logger()

    def setPreviewParams(self, width, height):
        self._previewWidth = width
        self._previewHeight = height

    def setStillParams(self, width, height):
        self._stillWidth = width
        self._stillHeight = height

    def setHostSensorParams(self, width, height):
        self._hostSensorWidth = width
        self._hostSensorHeight = height

    def setImager(self, imagerID):
        self._imagerID = imagerID

    def setGraphType(self, graphType):
        self._graphType = graphType

    def createAndRunGraph(self):
        if (self._state == GraphState.CLOSED):
            if(self._graphType == GraphType.HOST_SENSOR):
                self._obGraph.setImager("host")
            else:
                self._obGraph.setImager(self._imagerID)

            if(self._graphType != GraphType.HOST_SENSOR):
                self._obGraph.preview(self._previewWidth, self._previewHeight)

            graphTypeString = self.getGraphTypeString()
            if(graphTypeString ==  None):
                raise ValueError("Invalid graph type")

            self._obGraph.still(self._stillWidth, self._stillHeight, graphTypeString)

            self.logger.debug("Creating the %s graph" % graphTypeString)

            self._obGraph.run()

            self._state = GraphState.RUNNING
        else:
            self.logger.error("Couldn't create and run the graph")
            raise NvCSTestException("Graph is in invalid state: %s" % self.getGraphStateString())

    def stopAndDeleteGraph(self):
        if(self._state == GraphState.RUNNING):
            self.logger.debug("Stopping the graph")
            self._obGraph.stop()
            self._state = GraphState.STOPPED
        if(self._state == GraphState.STOPPED):
            self.logger.debug("Closing the graph")
            self._obGraph.close()
            self._state = GraphState.CLOSED

    def getGraphTypeString(self):
        if(self._graphType == GraphType.JPEG or self._graphType == GraphType.HOST_SENSOR):
            return "Jpeg"
        elif(self._graphType == GraphType.RAW):
            return "Bayer"
        else:
            return None

    def getGraphType(self):
        return self._graphType

    def getGraphStateString(self):
        if(self._state == GraphState.CLOSED):
            return "CLOSED"
        elif(self._state == GraphState.PAUSED):
            return "PAUSED"
        elif(self._state == GraphState.RUNNING):
            return "RUNNING"
        else:
            return "STOPPED"

    def getGraphState(self):
        return self._state

class NvCSTestCamera(object):
    _obGraph = None
    _obCamera = None
    concurrentRawImageDir = "/data/raw"
    _isPreviewRunning = False
    logger = None

    def __init__(self, obGraph):
        if(obGraph == None):
            raise ValueError("Graph is not open")
        self._obGraph = obGraph
        self._obCamera = nvcamera.Camera()
        self.logger = nvcstestutils.Logger()

    # redirect the setAttr call to nvcamera.Camera object
    def setAttr(self, attribute, value):
        self._obCamera.setAttr(attribute, value)

    # redirect the setRawImage call to nvcamera Camera object
    def setRawImage(self, header, pixelData, iteration):
        self._obCamera.setRawImage(header, pixelData, iteration)

    def isRawCaptureSupported(self):
        retVal = True
        try:
            self._obCamera.setAttr(nvcamera.attr_concurrentrawdumpflag, 7)
        except NvCameraException, err:
            retVal = False
        return retVal

    def isFocuserSupported(self):
        retVal = True
        try:
            self._obCamera.getAttr(nvcamera.attr_focuspos)
        except nvcamera.NvCameraException, err:
            if (err.value == nvcamera.NvError_NotSupported):
                self.logger.info("focuser is not present..")
                retVal = False
            else:
                print err.value
                raise
        return retVal

    # redirect the getAttr call to nvcamera.Camera object
    def getAttr(self, attribute):
        return self._obCamera.getAttr(attribute)

    def captureRAWImage(self, imageName):
        self.logger.debug("Capturing RAW image: %s" % imageName)
        graphType = self._obGraph.getGraphType()
        if(graphType == GraphType.RAW):
            self._obCamera.halfpress(10000)
            self._obCamera.waitForEvent(12000, nvcamera.CamConst_ALGS)
            self._obCamera.still(imageName)
            self._obCamera.waitForEvent(12000, nvcamera.CamConst_CAP_READY)
            self._obCamera.hp_release()
        else:
            self.logger.error("Can't capture RAW image with %s graph" %
                              self._obGraph.getGraphTypeString())
            raise NvCSTestException("Can't capture RAW image with %s graph" %
                                     self._obGraph.getGraphTypeString())

    def captureConcurrentJpegAndRawImage(self, imageName):
        self.logger.debug("Capturing concurrent RAW and jpeg image %s" % imageName)
        graphType = self._obGraph.getGraphType()
        if(graphType == GraphType.JPEG):
            self.logger.debug("Setting concurrent raw dump flag to 7")
            self._obCamera.setAttr(nvcamera.attr_concurrentrawdumpflag, 7)
            self._obCamera.setAttr(nvcamera.attr_pauseaftercapture, 1)

            # remove existing raw files under /data/raw
            if os.path.exists(self.concurrentRawImageDir):
                fileList = os.listdir(self.concurrentRawImageDir)
                for fileName in fileList:
                    filePath = os.path.join(self.concurrentRawImageDir, fileName)
                    self.logger.debug("removing %s file" % fileName)
                    os.remove(filePath)
            else:
                self.logger.debug("Creating %s directory" % self.concurrentRawImageDir)
                os.mkdir(self.concurrentRawImageDir)

            # capture an image
            self.captureJpegImage(imageName)

            rawFilesList = os.listdir(self.concurrentRawImageDir)

            # extract the filename
            (fileName, ext) = os.path.splitext(imageName)
            rawFilePath = fileName + ".nvraw"

            # rename the file
            if (rawFilesList):
                os.rename(os.path.join(self.concurrentRawImageDir, rawFilesList[0]),
                          rawFilePath)
            else:
                self.logger.error("Couldn't capture concurrent RAW and jpeg")
                raise NvCSTestException("Couldn't capture concurrent RAW and jpeg")
        else:
            self.logger.error("Can't capture concurrent RAW and jpeg image with %s graph" %
                              self._obGraph.getGraphTypeString())
            raise NvCSTestException("Can't capture concurrent RAW and jpeg image with %s graph" %
                                    self._obGraph.getGraphTypeString())

    def captureJpegImage(self, imageName):
        self.logger.debug("capturing jpeg image")
        graphType = self._obGraph.getGraphType()
        if(graphType == GraphType.JPEG):
            self._obCamera.halfpress(10000)
            self._obCamera.waitForEvent(12000, nvcamera.CamConst_ALGS)
            self._obCamera.still(imageName)
            self._obCamera.waitForEvent(12000, nvcamera.CamConst_CAP_READY, nvcamera.CamConst_CAP_FILE_READY)
        elif(graphType == GraphType.HOST_SENSOR):
            self._obCamera.still(imageName)
            self._obCamera.waitForEvent(12000, nvcamera.CamConst_CAP_READY, nvcamera.CamConst_CAP_FILE_READY)
            #self._obCamera.waitForEvent(12000, nvcamera.CamConst_ALL_CAPTURE_DONE)
        else:
            self.logger.error("Can't capture jpeg image with %s graph" %
                              self._obGraph.getGraphTypeString())
            raise NvCSTestException("Can't capture jpeg image with %s graph" %
                                    self._obGraph.getGraphTypeString())

    def startPreview(self):
        if(not self._isPreviewRunning):
            self._obCamera.startPreview()
            self._isPreviewRunning = True

    def stopPreview(self):
        if(self._isPreviewRunning):
            self._obCamera.stopPreview()
            self._obCamera.waitForEvent(12000, nvcamera.CamConst_PREVIEW_EOS)
            self._isPreviewRunning = False


class NvCSTestBase(object):
    "NvCSTest Base Class"

    testID = None
    obCamera = None
    obGraph = None
    logger = None
    nvrf = None
    options = None
    testDir = "/data/NVCSTest"

    def __init__(self, testID=None):
        self.testID = testID
        self.obGraph = NvCSTestGraph()
        self.obCamera = NvCSTestCamera(self.obGraph)
        self.testDir = os.path.join(self.testDir, testID)
        self.logger = nvcstestutils.Logger()
        self.nvrf = nvrawfile.NvRawFile()

    def run(self, args=None):
        retVal = 0
        try:
            self.logger.info("############################################")
            self.logger.info("Running the %s test..." % self.testID)
            self.logger.info("############################################")

            retVal = self.setupTest()
            if (retVal != NvCSTestResult.SUCCESS):
                self.logger.error("Test is not setup to run")
                return retVal

            self.setupGraph()
            self.obGraph.createAndRunGraph()

            # create test specific dir
            if(os.path.exists(self.testDir)):
                shutil.rmtree(self.testDir)
            os.makedirs(self.testDir)

            retVal = self.runPreTest()
            if (retVal != NvCSTestResult.SUCCESS):
                return retVal

            retVal = self.runTest(args)

            self.obGraph.stopAndDeleteGraph()
            return retVal

        except Exception, err:
            self.logger.error("TEST FAILURE: %s" % self.testID)
            self.logger.error("Error: %s" % str(err))
            traceback.print_exc()
            self.obGraph.stopAndDeleteGraph()
            raise

        return retVal

    def confirmTestSetup(self):
        # ask user about the confirmation
        ans = raw_input("Press Enter or enter comments, when the test setup is ready:")

        if (ans != ''):
            self.logger.info('User comments: %s' % ans)

        return True

    def needTestSetup(self):
        return False

    def getSetupString(self):
        return ''

    def setupTest(self):
        if((self.options.force == False) and self.needTestSetup()):
            setupString = self.getSetupString()
            if(setupString != ''):
                self.logger.info("%s" % setupString)

            if(self.confirmTestSetup()):
                return NvCSTestResult.SUCCESS
            else:
                return NvCSTestResult.ERROR
        return NvCSTestResult.SUCCESS

    def runTest(self, args=None):
        return NvCSTestResult.SUCCESS

    def runPreTest(self, args=None):
        return NvCSTestResult.SUCCESS

    def setupGraph(self):
        return NvCSTestResult.SUCCESS

