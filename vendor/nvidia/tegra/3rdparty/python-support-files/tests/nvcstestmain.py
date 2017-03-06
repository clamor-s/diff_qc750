#-------------------------------------------------------------------------------
# Name:        nvcstestmain.py
# Purpose:
#
# Created:     01/23/2012
#
# Copyright (c) 2007-2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
#-------------------------------------------------------------------------------
#!/usr/bin/env python

# global mapping between testname and
# test objects
import nvcstest
import nvcstestutils
import os
import shutil
from optparse import OptionParser
from optparse import make_option
import sys

class TestFactory:

    testDict = {                # enabled
        "jpeg_capture"          : 1,
        "raw_capture"           : 1,
        "concurrent_raw"        : 1,
        "exposuretime"          : 1,
        "gain"                  : 1,
        "focuspos"              : 1,
        "multiple_raw"          : 1,
        "linearity"             : 1,
        "host_sensor"           : 1,
        "sharpness"             : 1
    }

    def __init__(self):
        pass

    @classmethod
    def getTestObject(self, testName, options):
        if (testName == "jpeg_capture"):
            return nvcstest.NvCSJPEGCapTest(options)
        elif (testName == "raw_capture"):
            return nvcstest.NvCSRAWCapTest(options)
        elif (testName == "concurrent_raw"):
            return nvcstest.NvCSConcurrentRawCapTest(options)
        elif (testName == "exposuretime"):
            return nvcstest.NvCSExposureTimeTest(options)
        elif (testName == "gain"):
            return nvcstest.NvCSGainTest(options)
        elif (testName == "focuspos"):
            return nvcstest.NvCSFocusPosTest(options)
        elif (testName == "multiple_raw"):
            return nvcstest.NvCSMultipleRawTest(options)
        elif (testName == "linearity"):
            return nvcstest.NvCSLinearityRawTest(options)
        elif (testName == "host_sensor"):
            return nvcstest.NvCSHostSensorTest(options)
        elif (testName == "sharpness"):
            return nvcstest.NvCSSharpnessTest(options)
        else:
            return None

    @classmethod
    def hasTest(self, testName):
        return self.testDict.has_key(testName)

    @classmethod
    def disableTest(self, testName):
        if (self.testDict.has_key(testName)):
            self.testDict[testName] = 0

    @classmethod
    def enableTest(self, testName):
        if (self.testDict.has_key(testName)):
            self.testDict[testName] = 1

    @classmethod
    def isEnabled(self, testName):
        if (self.testDict.has_key(testName)):
            return self.testDict[testName]
        else:
            return 0


def main():

    numFailures = 0
    numTests = 0
    numSkippedTests = 0

    logger = nvcstestutils.Logger()

    # parse arguments
    usage = "Usage: %prog <options>"

    options = [
            make_option('-s', dest='sanity', default=False, action="store_true",
                    help = 'Run Sanity tests'),
            make_option('--odm', dest='odm_conformance', default=False, action="store_true",
                    help = 'Run ODM Conformance tests'),
            make_option('-t', dest='test_names', default=None, type="string", action = "append",
                    help = 'Test name to execute', metavar="TEST_NAME"),
            make_option('-d', dest='disabled_test_names', default=None, type="string", action = "append",
                    help = 'Disable the test', metavar="TEST_NAME"),
            make_option('-i', dest='imager_id', default=0, type="int",
                    help = 'set imager id. 0 for rear facing, 1 for front facing', metavar="IMAGER_ID"),
            make_option('-f', dest='force', default=False, action="store_true",
                    help = 'run the tests without any prompts', metavar="FORCE")
        ]

    options_parser = OptionParser(usage, option_list=options)

    # parse the command line arguments
    (test_options, args) = options_parser.parse_args()

    # print version infortion
    nvcstest.printVersionInformation()

    testNamesList = []
    if (test_options.sanity):
        testNamesList = ["jpeg_capture",
                         "raw_capture",
                         "concurrent_raw",
                         "exposuretime",
                         "gain",
                         "focuspos"]

    elif (test_options.odm_conformance):
        testNamesList = ["linearity"]

    elif (test_options.test_names != None):
        testNamesList = test_options.test_names

    __validateTestNames(testNamesList)

    # create a dictionary for disabled test names
    if (test_options.disabled_test_names != None):
        for testName in test_options.disabled_test_names:
            # get the test object
            if (TestFactory.hasTest(testName)):
                TestFactory.disableTest(testName)

    logger.info("Imager ID: %d" % test_options.imager_id)
    for testName in testNamesList:
        # get the test object
        try:
            # if the test is enabled, execute the test
            if (TestFactory.isEnabled(testName)):
                numTests = numTests + 1

                testOb = TestFactory.getTestObject(testName, test_options)
                retVal = testOb.run()

                if (retVal == nvcstest.NvCSTestResult.SUCCESS):
                    logger.info("RESULT: PASS\n")
                elif (retVal == nvcstest.NvCSTestResult.SKIPPED):
                    logger.info("RESULT: SKIP\n")
                    numSkippedTests = numSkippedTests + 1
                else:
                    logger.info("RESULT: FAIL\n")
                    numFailures = numFailures + 1

        except Exception, err:
            print err
            numFailures = numFailures + 1
            logger.info("RESULT: FAIL\n")

    logger.info("TOTAL TEST RUNS: %d" % numTests)
    logger.info("TOTAL FAILURES: %d" % numFailures)
    logger.info("TOTAL SKIPPED TESTS: %d" % numSkippedTests)

    if (numFailures > 0):
        sys.exit(1)

def __validateTestNames(testNamesList):
    success = True
    for testName in testNamesList:
        if (TestFactory.hasTest(testName) != True):
            success = False
            print "ERROR: Incorrect test name: %s" % testName

    if (success == False):
        sys.exit(1)

if __name__ == '__main__':
    main()
