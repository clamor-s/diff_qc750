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

class NvCSGainTest(NvCSTestBase):
    "Gain test"

    startGainVal = 0.0
    stopGainVal = 0.0
    step = 0.0
    errMargin = 10.0/100.0

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Gain")
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
        # get the gain range
        gainRange = self.obCamera.getAttr(nvcamera.attr_gainrange)
        self.startGainVal = gainRange[0]
        self.stopGainVal = gainRange[1]
        self.step = (gainRange[1] - gainRange[0])/10.0

        while(self.startGainVal <= self.stopGainVal):
            self.obCamera.startPreview()
            self.obCamera.setAttr(nvcamera.attr_bayergains, [self.startGainVal] * 4)

            # take an image
            fileName = "out_%s_%s_test" % (self.testID, str(self.startGainVal).replace('.', '_'))
            jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")
            rawFilePath = os.path.join(self.testDir, fileName + ".nvraw")
            self.logger.info("capturing jpeg image with gains set to %.2f..." % self.startGainVal)

            # take an image with specified ISO
            self.obCamera.captureConcurrentJpegAndRawImage(jpegFilePath)

            self.obCamera.stopPreview()

            if not self.nvrf.readFile(rawFilePath):
                self.logger.error("couldn't open the file: %s" % rawFilePath)
                return NvCSTestResult.ERROR

            if (abs(self.nvrf._sensorGains[0] -  self.startGainVal) > self.errMargin):
                self.logger.error("SensorGains value is not correct in the raw header: %f" % self.nvrf._sensorGains[0])
                return NvCSTestResult.ERROR
            self.startGainVal = self.startGainVal + self.step

        return NvCSTestResult.SUCCESS

class NvCSExposureTimeTest(NvCSTestBase):
    "Exposure Time Test"

    # set exposure values in ms to test
    exposureTimeValues = [0.5, 1.0, 5, 10, 37.5, 100, 200]
    errMargin = 10.0/100.0

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Exposure_Time")
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
        if (args != None):
            self.exposureTimeValues = args

        # query and print exposuretime range
        exposureTimeRange = self.obCamera.getAttr(nvcamera.attr_exposuretimerange)
        self.logger.info("exposuretime range: [%fsec, %fsec]" %  (exposureTimeRange[0], exposureTimeRange[1]))

        retVal = NvCSTestResult.SUCCESS

        for exposureTimeMS in self.exposureTimeValues:
            # keep a Seconds unit, as blocks-camera and nvraw header uses Seconds
            exposureTimeS = exposureTimeMS/1000.0

            # take an image specified rotation
            self.logger.info("capturing raw image with exposure time set to %fms..." % exposureTimeMS)
            self.obCamera.startPreview()

            self.obCamera.setAttr(nvcamera.attr_exposuretime, float(exposureTimeMS)/1000)
            # take an image
            fileName = "out_%s_%s_test" % (self.testID, str(exposureTimeMS).replace('.','_'))
            jpegFilePath = os.path.join(self.testDir, fileName + ".jpg")
            rawFilePath = os.path.join(self.testDir, fileName + ".nvraw")

            self.obCamera.captureConcurrentJpegAndRawImage(jpegFilePath)

            self.obCamera.stopPreview()

            if not self.nvrf.readFile(rawFilePath):
                self.logger.error("couldn't open the file: %s" % rawFilePath)
                retVal = NvCSTestResult.ERROR
                break
            minExpectedExpTime = exposureTimeS - (exposureTimeS * self.errMargin)
            maxExpectedExpTime = exposureTimeS + (exposureTimeS * self.errMargin)

            # check SensorExposure value
            if ((self.nvrf._exposureTime) < minExpectedExpTime or
                (self.nvrf._exposureTime) > maxExpectedExpTime):
                self.logger.error( "exposure value is not correct in the raw header: %.6f" %
                                    self.nvrf._exposureTime)
                self.logger.error( "exposure value should be between %.6f and %.6f" %
                                    (minExpectedExpTime, maxExpectedExpTime))
                retVal = NvCSTestResult.ERROR
                break

            expTime = self.obCamera.getAttr(nvcamera.attr_exposuretime)
            if (expTime < minExpectedExpTime or expTime > maxExpectedExpTime):
                self.logger.error("exposuretime is not set in the driver...")
                self.logger.error( "exposure value %.6f should be between %.6f and %.6f" %
                                    (expTime, minExpectedExpTime, maxExpectedExpTime))
                retVal = NvCSTestResult.ERROR
                break

        return retVal

class NvCSTestConfiguration:
    MaxTestAttempts = 10
    filename = "undefined"
    testing = "undefined"
    attr_gain = -1
    attr_expo = -1
    AvgR = 0.0
    AvgG = 0.0
    AvgB = 0.0

    def __init__(self, exposure, gain, subtest, filepath):
        self.attr_expo = exposure
        self.attr_gain = gain
        self.testing = subtest
        self.filename = filepath
        self.AvgR = 0.0
        self.AvgG = 0.0
        self.AvgB = 0.0
    def setAvgs(self, red, green, blue):
        self.AvgR = red
        self.AvgG = green
        self.AvgB = blue
    def processRawAvgs(self, nvrf):
        CenterRegionSize_W = 640
        CenterRegionSize_H = 480

        if (nvrf._width < CenterRegionSize_W):
            CenterRegionSize_W = nvrf._width
        if (nvrf._width < CenterRegionSize_H):
            CenterRegionSize_H = nvrf._height

        # round to even coordinates
        CenterRegionSize_W = (CenterRegionSize_W & (~1))
        CenterRegionSize_H = (CenterRegionSize_H & (~1))

        # Compute the starting point of the interesting region (even coordinates)
        CenterRegion_left = (((nvrf._width -
                            CenterRegionSize_W) / 2) & (~1))
        CenterRegion_top = (((nvrf._height -
                            CenterRegionSize_H) / 2) & (~1))
        CenterRegion_right = CenterRegion_left + CenterRegionSize_W
        CenterRegion_bottom = CenterRegion_top + CenterRegionSize_H

        w = CenterRegion_right - CenterRegion_left
        h = CenterRegion_bottom - CenterRegion_top

        bayerIndex = [0,0,0,0]
        AvgR = 0.0
        AvgG = 0.0
        AvgB = 0.0
        numPixels = 0.0

        bayerPhaseStr = nvrf._bayerPhase

        if (bayerPhaseStr == "GBRG"):
            # case NvColorFormat_X6Bayer10GBRG:
            # self.logger.debug("BayerPhase GBRG")
            IndexGb = 0;
            IndexB = 1;
            IndexR = 2;
            IndexGr = 3;
        elif (bayerPhaseStr == "RGGB"):
            # case NvColorFormat_X6Bayer10RGGB:
            # self.logger.debug("BayerPhase RGGB")
            IndexR = 0;
            IndexGr = 1;
            IndexGb = 2;
            IndexB = 3;
        elif (bayerPhaseStr == "BGGR"):
            # case NvColorFormat_X6Bayer10BGGR:
            # self.logger.debug("BayerPhase BGGR")
            IndexB = 0;
            IndexGb = 1;
            IndexGr = 2;
            IndexR = 3;
        elif (bayerPhaseStr == "GRBG"):
            # case NvColorFormat_X6Bayer10GRBG:
            # self.logger.debug("BayerPhase GRBG")
            IndexGr = 0;
            IndexR = 1;
            IndexB = 2;
            IndexGb = 3;
        else:
            # self.logger.error("BayerPhase Not Recognized '%s'", bayerPhaseStr)
            print "BayerPhase Not Recognized '%s'" % bayerPhaseStr

        for y in range(0, h, 2):
            for x in range(0, w, 2):
                bayerIndex[0] = nvrf._pixelData[(CenterRegion_top+y) * nvrf._width + (CenterRegion_left+x)]
                bayerIndex[1] = nvrf._pixelData[(CenterRegion_top+y) * nvrf._width + (CenterRegion_left+x)+1]
                bayerIndex[2] = nvrf._pixelData[((CenterRegion_top+y)+1) * nvrf._width + (CenterRegion_left+x)]
                bayerIndex[3] = nvrf._pixelData[((CenterRegion_top+y)+1) * nvrf._width + (CenterRegion_left+x)+1]

                AvgR +=  bayerIndex[IndexR]
                AvgG +=  (bayerIndex[IndexGr] + bayerIndex[IndexGb])/2
                AvgB +=  bayerIndex[IndexB]
                numPixels += 1.0

        AvgR /= numPixels
        AvgG /= numPixels
        AvgB /= numPixels

        self.AvgR = AvgR
        self.AvgG = AvgG
        self.AvgB = AvgB

        return AvgR, AvgG, AvgB

    @classmethod
    def runConfig(self, test, testConfig):
        test.obCamera.startPreview()
        test.obCamera.setAttr(nvcamera.attr_continuousautofocus, 0)
        test.obCamera.setAttr(nvcamera.attr_autofocus, 0)
        test.obCamera.setAttr(nvcamera.attr_focuspos, 450)
        test.obCamera.setAttr(nvcamera.attr_exposuretime, testConfig.attr_expo)
        test.obCamera.setAttr(nvcamera.attr_bayergains, [testConfig.attr_gain] * 4)

        # take an image
        test.logger.info("capturing image with exposure time set to %.5f..." % testConfig.attr_expo)
        test.logger.info("capturing image with gains set to %.2f..." % testConfig.attr_gain)

        # take an image with specified ISO
        fileName = testConfig.filename
        rawFilePath = os.path.join(test.testDir, fileName + ".nvraw")
        jpegFilePath = os.path.join(test.testDir, fileName + ".jpg")
        test.obCamera.captureConcurrentJpegAndRawImage(jpegFilePath)

        test.obCamera.stopPreview()

        if not test.nvrf.readFile(rawFilePath):
            test.logger.error("couldn't open the file: %s" % rawFilePath)
            return NvCSTestResult.ERROR

        #test.logger.info("bayerphase value is %d..." % (test.nvrf._bayerPhase >> 20))
        # check SensorGain value
        if (abs(test.nvrf._sensorGains[0] - float(testConfig.attr_gain)) > 0.001):
            test.logger.error("SensorGains value is not correct in the raw header: %f" % test.nvrf._sensorGains[0])
            return NvCSTestResult.ERROR
        # check SensorExposure value
        if (abs(testConfig.attr_expo- test.nvrf._exposureTime) > 0.001):
            test.logger.error( "exposure value is not correct in the raw header...")
            return NvCSTestResult.ERROR
        expTime = test.obCamera.getAttr(nvcamera.attr_exposuretime)
        if (not ((expTime > testConfig.attr_expo - test.errMargin) and (expTime < testConfig.attr_expo + test.errMargin))):
            test.logger.error("exposuretime is not set int the driver...")
            return NvCSTestResult.ERROR

        testConfig.processRawAvgs(test.nvrf)

    @classmethod
    def runConfigList(self, test, configList):
        for testConfig in configList:
            testConfig.runConfig(test, testConfig)

    @classmethod
    def runLinearity(self, test, configList, testName):
        Linearity = False
        attemptNumber = 1
        logStr = ""

        while((Linearity == False) and (attemptNumber <= self.MaxTestAttempts)):
            test.logger.info("Starting %s Attempt #%d/%d" % (testName, attemptNumber, self.MaxTestAttempts))
            NvCSTestConfiguration.runConfigList(test, configList)
            Linearity, logStr = test.analyzeLinearityStats(testName, configList)
            test.logger.info("%s test Attempt #%d/%d %s" % (testName, attemptNumber, self.MaxTestAttempts, "PASSED" if (Linearity) else "FAILED"))
            attemptNumber += 1

        return Linearity, logStr

    @classmethod
    def runSNR(self, test, configList, testName):
        Linearity = False
        attemptNumber = 1
        logStr = ""

        while((Linearity == False) and (attemptNumber <= self.MaxTestAttempts)):
            test.logger.info("Starting %s Attempt #%d/%d" % (testName, attemptNumber, self.MaxTestAttempts))
            NvCSTestConfiguration.runConfigList(test, configList)
            Linearity, logStr = test.analyzeSNRStats(testName, configList)
            test.logger.info("%s test Attempt #%d/%d %s" % (testName, attemptNumber, self.MaxTestAttempts, "PASSED" if (Linearity) else "FAILED"))
            attemptNumber += 1

        return Linearity, logStr


class NvCSLinearityRawTest(NvCSTestBase):
    "Linearity Raw Image Test"

    startGainVal = 2.0
    stopGainVal = 6.0
    step = 1.0
    maxExpTime = 0.250
    minExpTime = 0.050
    errMargin = 0.001
    TestSensorRangeMax = 0.95

    def __init__(self, options):
        NvCSTestBase.__init__(self, "Linearity")
        self.options = options

    def setupGraph(self):
        self.obGraph.setImager(self.options.imager_id)

    def runPreTest(self):
        if (not self.obCamera.isRawCaptureSupported()):
            self.logger.info("raw capture is not supported")
            return NvCSTestResult.SKIPPED
        else:
            return NvCSTestResult.SUCCESS

    def analyzeSNRStats(self, testName, testsList):
        TmpBlackLevelTotal = 1024.0
        MinBlackLevelTotal = 1024.0
        AvgColor = [0.0,0.0,0.0]
        BlackLevel = [0.0,0.0,0.0]
        NumValidImages = 0
        EffectiveImageCount = 0
        Variance = [0.0, 0.0, 0.0]
        MaxPointError = [0.0, 0.0, 0.0]
        TotalError = [0.0, 0.0, 0.0]
        MIN_SNR_dB = 30.0
        logStr = ""

        Linearity = True

        for imageStat in testsList:
            if (imageStat.testing == "ImagerLinearityTesting_BlackLevel"):
                TmpBlackLevelTotal = imageStat.AvgR + imageStat.AvgG + imageStat.AvgB
                logStr += ("BL %.2f %.2f %.2f = %.2f ?< %.2f\n" %
                    (imageStat.AvgR,  imageStat.AvgG, imageStat.AvgB, TmpBlackLevelTotal, MinBlackLevelTotal))

                if (TmpBlackLevelTotal < MinBlackLevelTotal):
                    MinBlackLevelTotal = TmpBlackLevelTotal;
                    BlackLevel[0] = imageStat.AvgR;
                    BlackLevel[1] = imageStat.AvgG;
                    BlackLevel[2] = imageStat.AvgB;

        logStr += ("Black Level:  %.1f, %.1f, %.1f\n" % (BlackLevel[0], BlackLevel[1], BlackLevel[2]))
        logStr += ("-----------------------\n")
        logStr += ("%s\n" % testName)
        logStr += ("-----------------------\n")
        logStr += ("%8s %8s %8s %7s %7s %7s %7s %7s %7s\n" % ("Exposure", "Gain", "EV", "R", "G", "B", "R-BL", "G-BL", "B-BL"))

        for imageStat in testsList:
            AvgColorNBL = [0.0, 0.0, 0.0]
            if (imageStat.testing  == "ImagerLinearityTesting_BlackLevel"):
                continue
            AvgColorNBL[0] = imageStat.AvgR - BlackLevel[0]
            AvgColorNBL[1] = imageStat.AvgG - BlackLevel[1]
            AvgColorNBL[2] = imageStat.AvgB - BlackLevel[2]
            AvgColor[0] += imageStat.AvgR - BlackLevel[0]
            AvgColor[1] += imageStat.AvgG - BlackLevel[1]
            AvgColor[2] += imageStat.AvgB - BlackLevel[2]

            logStr += ("%8.3f %8.3f %8.3f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f\n" %
                (imageStat.attr_expo, imageStat.attr_gain, imageStat.attr_expo * imageStat.attr_gain,
                imageStat.AvgR, imageStat.AvgG, imageStat.AvgB, AvgColorNBL[0], AvgColorNBL[1], AvgColorNBL[2]))

            EffectiveImageCount +=1;

        AvgColor[0] /= EffectiveImageCount;
        AvgColor[1] /= EffectiveImageCount;
        AvgColor[2] /= EffectiveImageCount;


        for imageStat in testsList:
            if (imageStat.testing  == "ImagerLinearityTesting_BlackLevel"):
                continue
            e =  AvgColor[0] - (imageStat.AvgR - BlackLevel[0]);
            Variance[0] += e * e;
            e =  AvgColor[1] - (imageStat.AvgG - BlackLevel[1]);
            Variance[1] += e * e;
            e =  AvgColor[2] - (imageStat.AvgB - BlackLevel[2]);
            Variance[2] += e * e;

        Variance[0] /= 1.0*EffectiveImageCount # Raw, RGB-value variance
        Variance[1] /= 1.0*EffectiveImageCount
        Variance[2] /= 1.0*EffectiveImageCount

        for j in range(0, 3):
            colorName = ["Red", "Green", "Blue"]
            stdDev = math.sqrt(Variance[j])
            # SNR (units of dB). Typical imaging definition. Take raw S:N ratio
            # of this test's images, then apply the log-20 rule to convert to dB.
            if (stdDev != 0):
                SNR = (20.0 * math.log10( AvgColor[j] / stdDev ))
            else:
                SNR = 0.0

            if(SNR < MIN_SNR_dB):
                Linearity = False
            if (j == 0):
                logStr += ("%5s  %8s  %8s  %7s  %7s\n" %
                    ("Color", "Average", "Variance", "StdDev", "SNR(db)"))

            logStr += ("%5s  %8.2f  %8.3f  %7.3f  %7.3f  %s\n" %
                (colorName[j], AvgColor[j], Variance[j], stdDev, SNR, "OK" if (SNR >= MIN_SNR_dB) else "FAIL"))

        return Linearity, logStr

    def analyzeLinearityStats(self, testName, testsList):
        TmpBlackLevelTotal = 1024.0
        MinBlackLevelTotal = 1024.0
        AvgColor = [0.0,0.0,0.0]
        rSquared = [0.0,0.0,0.0]
        BlackLevel = [0.0,0.0,0.0]
        a = [0.0,0.0,0.0]
        b = [0.0,0.0,0.0]
        SumC = [0.0,0.0,0.0]
        SumCSquared = [0.0,0.0,0.0]
        SumCxEv = [0.0,0.0,0.0]
        Ev = 0.0
        SumEv = 0.0
        SumEvSquared = 0.0
        NumValidImages = 0
        EffectiveImageCount = 0
        Variance = [0.0, 0.0, 0.0];
        MaxPointError = [0.0, 0.0, 0.0]
        TotalError = [0.0, 0.0, 0.0]
        MIN_RSQUARED_ERROR = 0.990
        MIN_SLOPE = 10.0
        OverExposed = False
        UnderExposed = False
        logStr = ""

        Linearity = True
        ChannelLinear = [True, True, True]
        UseGrayCard = False

        #Look for black level tests first.
        for imageStat in testsList:
            if (imageStat.testing == "ImagerLinearityTesting_BlackLevel"):
                TmpBlackLevelTotal = imageStat.AvgR + imageStat.AvgG + imageStat.AvgB
                logStr += ("BL %.2f %.2f %.2f = %.2f ?< %.2f\n" %
                    (imageStat.AvgR,  imageStat.AvgG, imageStat.AvgB, TmpBlackLevelTotal, MinBlackLevelTotal))

                if (TmpBlackLevelTotal < MinBlackLevelTotal):
                    MinBlackLevelTotal = TmpBlackLevelTotal;
                    BlackLevel[0] = imageStat.AvgR;
                    BlackLevel[1] = imageStat.AvgG;
                    BlackLevel[2] = imageStat.AvgB;

        logStr += ("Black Level:  %.1f, %.1f, %.1f\n" %
            (BlackLevel[0], BlackLevel[1], BlackLevel[2]))
        logStr += ("-----------------------\n")
        logStr += ("%s\n" % testName)
        logStr += ("-----------------------\n")
        logStr += ("%8s %8s %8s %7s %7s %7s %7s %7s %7s\n" %
            ("Exposure", "Gain", "EV", "R", "G", "B", "R-BL", "G-BL", "B-BL"))

        #
        # Given N sets of (ExposureValue, Red), we are trying to solve a and b so
        # that sum of (Red - a*ExposureValue + b)^2 is minimal
        # So, we are fitting the data into a line: f(x) = ax + b where x is
        # ExposureValue and f(x) is Red.
        # We can solve this by the least linear square system: XB = Y =>
        # |x0 1|       |f(x0)|
        # |x1 1| |a|   |f(x1)|
        # | : 1| |b| = |  :  |
        # |xn 1|       |f(xn)|
        #
        # Multiply X^T to both side and we have
        # |x0^2 + ... + xn^2   x0 + ... + xn| |a|   |x0*f(x0) + ... + xn*f(xn)|
        # |x0 + ... + xn               n    | |b| = |f(x0) + ... + f(xn)      |
        #
        # Let SumXSquared = x0^2 + ... + xn^2
        #     SumX = x0 + ... + xn
        #     SumY = f(x0) + ... + f(xn)
        #     SumXY = x0*f(x0) + ... + xn*f(xn)
        # Then, we have two equations
        # SumXSquared * a + SumX * b = SumXY
        # SumX * a + n * b = SumY
        #
        # Therefore,
        # a = (SumX * SumY - n * SumXY) / (SumX * SumX - n * SumXSquared)
        # b = (SumY - SumX * a) / n
        #

        for imageStat in testsList:
            if (imageStat.testing  == "ImagerLinearityTesting_BlackLevel"):
                continue
            if (imageStat.AvgR < (BlackLevel[0]*2.0)):
                UnderExposed = True
            if (imageStat.AvgR < (BlackLevel[1]*2.0)):
                UnderExposed = True
            if (imageStat.AvgR < (BlackLevel[2]*2.0)):
                UnderExposed = True

            AvgColor = [0.0, 0.0, 0.0]
            if (imageStat.AvgR > (self.TestSensorRangeMax*1024.0)):
                OverExposed = True
            if (imageStat.AvgG > (self.TestSensorRangeMax*1024.0)):
                OverExposed = True
            if (imageStat.AvgB > (self.TestSensorRangeMax*1024.0)):
                OverExposed = True
            AvgColor[0] = imageStat.AvgR - BlackLevel[0];
            AvgColor[1] = imageStat.AvgG - BlackLevel[1];
            AvgColor[2] = imageStat.AvgB - BlackLevel[2];
            if (AvgColor[0] < 0.0): AvgColor[0] = 0.0
            if (AvgColor[1] < 0.0): AvgColor[1] = 0.0
            if (AvgColor[2] < 0.0): AvgColor[2] = 0.0

            Ev = imageStat.attr_expo * imageStat.attr_gain
            SumEv += Ev;
            SumEvSquared += (Ev * Ev);
            SumC[0] += AvgColor[0];
            SumC[1] += AvgColor[1];
            SumC[2] += AvgColor[2];
            SumCSquared[0] += AvgColor[0] * AvgColor[0];
            SumCSquared[1] += AvgColor[1] * AvgColor[1];
            SumCSquared[2] += AvgColor[2] * AvgColor[2];
            SumCxEv[0] += (AvgColor[0] * Ev);
            SumCxEv[1] += (AvgColor[1] * Ev);
            SumCxEv[2] += (AvgColor[2] * Ev);

            NumValidImages+=1;

            logStr += ("%8.3f %8.3f %8.3f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f\n" %
                (imageStat.attr_expo, imageStat.attr_gain, imageStat.attr_expo * imageStat.attr_gain,
                imageStat.AvgR, imageStat.AvgG, imageStat.AvgB, AvgColor[0], AvgColor[1], AvgColor[2]))

        if (SumEv * SumEv == SumEvSquared):
            logStr += ("Invalid State: SumEv * Sum Ev == SumEvSquared\n")
            Linearity = False
            return Linearity, logStr

        for j in range(0, 3):
        #This set of formulas apply linear regression
            t1 = 0
            t2 = 0
            tx = 0
            t3 = round((NumValidImages * SumEvSquared - SumEv * SumEv), 3)
            a[j] = 0.0 if(t3 == 0.0) else (NumValidImages * SumCxEv[j] - SumEv * SumC[j]) / t3
            b[j] = (SumC[j] - a[j] * SumEv) / NumValidImages;

            t1 = NumValidImages * SumEvSquared - SumEv * SumEv;
            t2 = NumValidImages * SumCSquared[j] - SumC[j] * SumC[j];

            tx = (NumValidImages * SumCxEv[j] - SumEv * SumC[j]);

            rSquared[j] = (tx * tx) / abs(t1 * t2);

        # Calculate the mean squared error
        EffectiveImageCount = 0;
        for imageStat in testsList:
            if (imageStat.testing  == "ImagerLinearityTesting_BlackLevel"):
                continue
            AvgColor = [0.0, 0.0, 0.0]
            AvgColor[0] = imageStat.AvgR - BlackLevel[0];
            AvgColor[1] = imageStat.AvgG - BlackLevel[1];
            AvgColor[2] = imageStat.AvgB - BlackLevel[2];
            if (AvgColor[0] < 0.0): AvgColor[0] = 0.0
            if (AvgColor[1] < 0.0): AvgColor[1] = 0.0
            if (AvgColor[2] < 0.0): AvgColor[2] = 0.0
            EffectiveImageCount+=1;

            for j in range(0, 3):
                e = (AvgColor[j] - a[j] * imageStat.attr_expo * imageStat.attr_gain - b[j]);
                e *= e;
                if (e > MaxPointError[j]):
                    MaxPointError[j] = e;
                    TotalError[j] += e;

        # Calculate average RGB values across all images
        AvgColor = [0.0, 0.0, 0.0]
        for imageStat in testsList:
            if (imageStat.testing  == "ImagerLinearityTesting_BlackLevel"):
                continue
            AvgColor[0] += imageStat.AvgR - BlackLevel[0];
            AvgColor[1] += imageStat.AvgG - BlackLevel[1];
            AvgColor[2] += imageStat.AvgB - BlackLevel[2];
        AvgColor[0] /= EffectiveImageCount;
        AvgColor[1] /= EffectiveImageCount;
        AvgColor[2] /= EffectiveImageCount;

        # Calculate variance of each image relative to the average of all images.
        for imageStat in testsList:

            if (imageStat.testing  == "ImagerLinearityTesting_BlackLevel"):
                continue
            e =  AvgColor[0] - (imageStat.AvgR - BlackLevel[0]);
            Variance[0] += e * e;
            e =  AvgColor[1] - (imageStat.AvgG - BlackLevel[1]);
            Variance[1] += e * e;
            e =  AvgColor[2] - (imageStat.AvgB - BlackLevel[2]);
            Variance[2] += e * e;

        Variance[0] /= (EffectiveImageCount - 1); # Raw, RGB-value variance
        Variance[1] /= (EffectiveImageCount - 1);
        Variance[2] /= (EffectiveImageCount - 1);

        for j in range(0, 3):
            colorName = ["Red", "Green", "Blue"]
            TotalError[j] /= NumValidImages;

            if (j == 0):
                logStr += ("Color:      R^2  AvgErr^2  MaxErr^2   Line\n")

            logStr += ("%5s:  %7.5f  %8.4f  %8.4f   Y=%.2fX + %.2f\n" %
                (colorName[j], rSquared[j], TotalError[j], MaxPointError[j], a[j], b[j]))

            if (rSquared[j] < MIN_RSQUARED_ERROR):
                Linearity = False
                ChannelLinear[j] = False
            if (a[j] < MIN_SLOPE):
                Linearity = False
                ChannelLinear[j] = False

        for i in range(0, 3, 1):
            if (ChannelLinear[i] == True and Linearity == False):
                UseGrayCard = True;

        if (OverExposed):
            logStr += ("There may have been overexposed images in the sample set.  Re-Run the test with lower lights, or darker colors.\n")
            Linearity = False
        if (UnderExposed):
            logStr += ("The sample set may have been underexposed.  Re-Run the test with more light or brighter colors.\n")
            Linearity = False
        if (Linearity != True):
            logStr += ("%s FAILED, minimum R^2 value is %f, minimum slope value is %f\n" % (testName, MIN_RSQUARED_ERROR, MIN_SLOPE))
        if (UseGrayCard == True):
            logStr += ("It looks like some color channels were good, try a more neutral scene (grey card)\n")

        return Linearity, logStr

    def runTest(self, args):
        self.logger.info("NVCS Linearity Test v1.2")

        # Grab "Black Level"
        BlackLevel = [0.0, 0.0, 0.0]
        tmpBLConfig = NvCSTestConfiguration(
            0.00005,
            1.0,
            "ImagerLinearityTesting_BlackLevel",
            "out_%sBL_%.5f_%.2f_test" % (self.testID, self.minExpTime, self.startGainVal))

        NvCSTestConfiguration.runConfig(self, tmpBLConfig)

        BlackLevel[0] = tmpBLConfig.AvgR
        BlackLevel[1] = tmpBLConfig.AvgG
        BlackLevel[2] = tmpBLConfig.AvgB

        # Check environment
        envCheckList = []
        envCheckList.append(NvCSTestConfiguration(
            self.minExpTime,
            self.startGainVal,
            "TestMinEnv",
            "out_%s_ec_%.5f_%.2f_test" % (self.testID, self.minExpTime, self.startGainVal)))
        envCheckList.append(NvCSTestConfiguration(
            self.maxExpTime,
            self.startGainVal,
            "TestMaxEnv",
            "out_%s_ec_%.5f_%.2f_test" % (self.testID, self.maxExpTime, self.startGainVal)))
        envCheckList.append(NvCSTestConfiguration(
            self.minExpTime,
            self.stopGainVal,
            "TestMaxEnv",
            "out_%s_ec_%.5f_%.2f_test" % (self.testID, self.minExpTime, self.stopGainVal)))

        NvCSTestConfiguration.runConfigList(self, envCheckList)

        UnderExposed = False
        OverExposed = False
        for imageStat in envCheckList:
            self.logger.info("Env--BL Check: [%.1f, %.1f, %.1f]--[%.1f, %.1f, %.1f]" %
                (imageStat.AvgR, imageStat.AvgG, imageStat.AvgB, BlackLevel[0], BlackLevel[1], BlackLevel[2]))
            if (imageStat.AvgR < (BlackLevel[0]*2.0)):
                UnderExposed = True
            if (imageStat.AvgG < (BlackLevel[1]*2.0)):
                UnderExposed = True
            if (imageStat.AvgB < (BlackLevel[2]*2.0)):
                UnderExposed = True

            if (imageStat.AvgR > (self.TestSensorRangeMax*1024.0)):
                OverExposed = True
            if (imageStat.AvgG > (self.TestSensorRangeMax*1024.0)):
                OverExposed = True
            if (imageStat.AvgB > (self.TestSensorRangeMax*1024.0)):
                OverExposed = True
        if (OverExposed):
            self.logger.info("")
            self.logger.info("")
            self.logger.error("Environment is too bright to run test.  Re-Run the test with lower lights, or darker colors.")
            self.logger.error("It is possible that an auto feature may also be on, preventing the test from calibrating correctly.")
            self.logger.error("All auto features (ie AGC, AWB, AEC) should be turned off.")
            self.logger.info("")
            self.logger.info("")
            return NvCSTestResult.ERROR
        if (UnderExposed):
            self.logger.info("")
            self.logger.info("")
            self.logger.error("Environment is too dark to run test.  Re-Run the test with more light or brighter colors.")
            self.logger.error("It is possible that an auto feature may also be on, preventing the test from calibrating correctly.")
            self.logger.error("All auto features (ie AGC, AWB, AEC) should be turned off.")
            self.logger.info("")
            self.logger.info("")
            return NvCSTestResult.ERROR

        # Run the Gain Linearity Experiment

        testGainLinearityList = []
        testGainLinearityList.append(NvCSTestConfiguration(
                0.0005,
                1.0,
                "ImagerLinearityTesting_BlackLevel",
                "out_%sBL_%.5f_%.2f_test" % (self.testID, self.minExpTime, self.startGainVal)))
        testGainLinearityList.append(NvCSTestConfiguration(
                0.00005,
                1.0,
                "ImagerLinearityTesting_BlackLevel",
                "out_%sBL_%.5f_%.2f_test" % (self.testID, self.minExpTime, self.startGainVal)))

        for gainVal in range(int(self.startGainVal*1000), int(self.stopGainVal*1000+self.step*1000), int(self.step*1000)):
            testGainLinearityList.append(NvCSTestConfiguration(
                self.minExpTime,
                gainVal/1000.0,
                "ImagerLinearityTesting_Gain",
                "out_%sG_%.3f_%.2f_test" % (self.testID, self.minExpTime, gainVal/1000.0)))

        GainIsLinear, GainLog = NvCSTestConfiguration.runLinearity(self, testGainLinearityList, "Gain Linearity")

        if (not GainIsLinear):
            self.logger.info("%s" % GainLog)
            self.logger.info("")
            self.logger.info("")
            self.logger.error("Gain Linearity Test failed.")
            self.logger.info("--Check if values are being written to the proper camera registers")
            self.logger.info("--Check if values are being translated correctly from floating-point values to register values")
            self.logger.info("--Ensure all exposure value auto features are turned off, we want manual modes (ie. AGC, AEC, AWB)")
            self.logger.info("--Check if test configuration gain values are reaching ODM correctly:")
            self.logger.info("\t-Add a print to confirm gain floating point values that are translated in the ODM driver (F32_TO_GAIN)")
            self.logger.info("\t-Compare printed floating point values from the ODM driver to the test configurations listed in the table")
            self.logger.info("\t\tIf the values are different, there is an issue in blocks-camera (camera/core) and may be missing a patch")
            self.logger.info("")
            self.logger.info("")
            return NvCSTestResult.ERROR

        # Run the Exposure Linearity Experiment

        testExposureLinearityList = []
        testExposureLinearityList.append(NvCSTestConfiguration(0.0005, 1.0,
            "ImagerLinearityTesting_BlackLevel", "out_%sBL_%.5f_%.2f_test" %
            (self.testID, self.minExpTime, self.startGainVal)))
        testExposureLinearityList.append(NvCSTestConfiguration(0.00005, 1.0,
            "ImagerLinearityTesting_BlackLevel", "out_%sBL_%.5f_%.2f_test" %
            (self.testID, self.minExpTime, self.startGainVal)))
        for expoVal in range(int(self.minExpTime*1000), int(self.maxExpTime*1000), int((self.maxExpTime-self.minExpTime)*1000.0/6.0)):
            gainVal = 2.0
            testExposureLinearityList.append(NvCSTestConfiguration(
                expoVal/1000.0,
                gainVal,
                "ImagerLinearityTesting_Exposure",
                "out_%sE_%.3f_%.2f_test" % (self.testID, expoVal/1000.0, gainVal)))

        ExpoIsLinear, ExpoLog = NvCSTestConfiguration.runLinearity(self, testExposureLinearityList, "ExposureTime Linearity")

        if (not ExpoIsLinear):
            self.logger.info("%s" % GainLog)
            self.logger.info("%s" % ExpoLog)
            self.logger.info("")
            self.logger.info("")
            self.logger.error("Exposure Linearity Test failed.")
            self.logger.info("--Check if values are being written to the proper camera registers")
            self.logger.info("--Ensure all exposure value auto features are turned off, we want manual modes (ie. AGC, AEC, AWB)")
            self.logger.info("--Check if test configuration exposure values are reaching ODM correctly:")
            self.logger.info("\t-Add a print to confirm exposure time floating point values that are translated in the ODM driver (WriteExposure, GroupHold, SetMode)")
            self.logger.info("\t-Compare printed floating point values from the ODM driver to the test configurations listed in the table")
            self.logger.info("\t\tIf the values are different, there is an issue in blocks-camera (camera/core) and may be missing a patch")
            self.logger.info("")
            self.logger.info("")
            return NvCSTestResult.ERROR

        # Run the ExposureValue Linearity Experiment

        testEVSnrList = []
        testEVSnrList.append(NvCSTestConfiguration(0.0005, 1.0,
            "ImagerLinearityTesting_BlackLevel", "out_%sBL_%.5f_%.2f_test" %
            (self.testID, self.minExpTime, self.startGainVal)))
        testEVSnrList.append(NvCSTestConfiguration(0.00005, 1.0,
            "ImagerLinearityTesting_BlackLevel", "out_%sBL_%.5f_%.2f_test" %
            (self.testID, self.minExpTime, self.startGainVal)))

        # Arbitrary limits for now
        maxEV = 0.8
        EV = 0.55 * maxEV
        exp = EV / self.stopGainVal
        maxExp = EV / self.startGainVal
        expStep = (maxExp - exp)/4.0
        for expoVal in range(int(exp*1000), int(maxExp*1000), int(expStep*1000)):
            gainVal = EV/(expoVal/1000.0)
            testEVSnrList.append(NvCSTestConfiguration(expoVal/1000.0, gainVal,
                "ImagerLinearityTesting_ExposureValue", "out_%sEV_%.3f_%.2f_test" %
                (self.testID, expoVal/1000.0, gainVal)))

        NvCSTestConfiguration.runConfigList(self, testEVSnrList)

        EVIsLinear, EVLog = NvCSTestConfiguration.runSNR(self, testEVSnrList, "Exposure Value Linearity")

        if (not EVIsLinear):
            self.logger.info("%s" % GainLog)
            self.logger.info("%s" % ExpoLog)
            self.logger.info("%s" % EVLog)
            self.logger.info("")
            self.logger.info("")
            self.logger.error("Exposure Value Linearity Test failed.")
            self.logger.info("The pixel channel values (R,G,B) values here should have remained constant throughout the test")
            self.logger.info("--Check if test configuration exposure and gain values are reaching ODM correctly:")
            self.logger.info("--Double Check gain values are translated correctly")
            self.logger.info("--If it passed the previous Gain and Exposure Linearity tests, try increasing brightness")
            self.logger.info("  to reduce possible low level noises")
            self.logger.info("")
            self.logger.info("")
            return NvCSTestResult.ERROR

        self.logger.info("%s" % GainLog)
        self.logger.info("%s" % ExpoLog)
        self.logger.info("%s" % EVLog)

        return NvCSTestResult.SUCCESS

