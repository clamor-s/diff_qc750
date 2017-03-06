# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

import os
import shutil
import datetime

class LogLevel(object):
    debug = 1
    info = 2
    warning = 3
    error = 4
    fatal = 5

class Singleton(object):
    """ A Pythonic Singleton """
    def __new__(cls, *args, **kwargs):
        if '_inst' not in vars(cls):
            cls._inst = object.__new__(cls, *args, **kwargs)
        return cls._inst

class Logger(Singleton):
    "Logger class"

    logLevel = LogLevel.debug
    logFileHandle = None
    logFileName = "summarylog.txt"
    logDir = "/data/NVCSTest"

    def __init__(self):
        if(self.logFileHandle == None):
            if(os.path.isdir(self.logDir)):
                shutil.rmtree(self.logDir + "/")
            os.mkdir(self.logDir)
            logFilePath = os.path.join(self.logDir, self.logFileName)
            self.logFileHandle = open(logFilePath,"w")

    def __del__(self):
        if (self.logFileHandle != None):
            print "close file"
            self.logFileHandle.close()
            self.logFileHandle = None

    def __getLevelString(self, level):
        if (level == LogLevel.debug):
            return "DEBUG"
        elif (level == LogLevel.info):
            return "INFO"
        elif (level == LogLevel.warning):
            return "WARNING"
        elif (level == LogLevel.error):
            return "ERROR"
        elif (level == LogLevel.fatal):
            return "FATAL"

    def setLevel(self, level):
        self.setLevel = level

    def info(self, msg):
        self.__log(LogLevel.info, msg)

    def debug(self, msg):
        self.__log(LogLevel.debug, msg)

    def warning(self, msg):
        self.__log(LogLevel.warning, msg)

    def error(self, msg):
        self.__log(LogLevel.error, msg)

    def fatal(self, msg):
        self.__log(LogLevel.fatal, msg)

    def __log(self, level, msg):
        if (level < self.logLevel):
            return
        levelString = self.__getLevelString(level)
        timeStampString = str(datetime.datetime.now())
        print "%s: %s" % (levelString, msg)
        self.logFileHandle.write("%s %7s %s\n" % (timeStampString, levelString, msg))
