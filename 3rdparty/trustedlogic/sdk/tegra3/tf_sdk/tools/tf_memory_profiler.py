#!/usr/bin/python
# -*- coding: Utf-8 -*-

#
# Copyright © 2012 Trusted Logic Mobility SAS, 
# RCS Nanterre (France) 480 011 998
# All Rights Reserved.
# The present software is the confidential and proprietary information 
# of Trusted Logic Mobility SAS. You shall not disclose the present software 
# and shall use it only in accordance with the terms of the license agreement 
# you entered into with Trusted Logic Mobility SAS. “Trusted Logic” is 
# a registered trademark of Trusted Logic SAS. This software may be subject 
# to export or import laws in certain countries.
#
# TRUSTED LOGIC MOBILITY SAS MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
# SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
# NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
# MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
#
# In order to run the script for Win Simulator mode (--win_simu option),
# two environment varables have to be set:
# SIMULATOR_EXE - path to simulator executable including the executable's name
# SERVICE_BIN - path to service binary including the binary's name

#------------------------------------------
# Imports
#------------------------------------------

import threading
import os
import subprocess
from subprocess import *
import time
import sys
import re

import sys
from subprocess import PIPE, Popen
from threading  import Thread

from optparse import OptionParser    
from optparse import OptionError    
import optparse
    
    
#------------------------------------------
# Constants
#------------------------------------------    
#Version 
mpTitle = 'TF Memory Profiler'
mpVersion = '2.0'
mpTitleWithVersion = mpTitle + ' ' + mpVersion 

#Patterns
#dmesg pattern 
#<6>[ 2664.150727] TF : INFO 00003355 00:46:47.024 P-0001 T-0009 System --------- Heap usage: Free size=221000, Largest Free Size=220632
dmesgPattern = re.compile(r'<([0-9]+)>\[\s*(?P<bdot>[0-9]+)\.(?P<adot>[0-9]+)\]\s*TF\s*\:\s*(?P<tf_log>[\s\S]+)')
#Main pattern
wholeLinePattern = re.compile(r"(?P<level>[A-Z]{4})\s*(?P<counter>[0-9]{8})\s*(?P<time>\S{12})\s*(?P<pid>\S{6})\s*(?P<tid>\S{6})\s*(?P<name>[\s\S]{16})\s*(?P<memact>[\s\S]+)")
#wholeLinePattern = re.compile(r"(?P<level>[A-Z]{4})\s*(?P<counter>[0-9]{8})\s*(?P<time>\S{12})\s*(?P<id>\S+\s*\S+)\s*(?P<name>[\s\S]{16})\s*(?P<memact>[\s\S]+)")
#SMemAlloc(4096,4) -> 0x00350260
allocPattern = re.compile('.*SMemAlloc\(([0-9]+),([0-9]+)\) -> 0x([0-9A-Fa-f]+).*')
#SMemRealloc(0x003B09A0, 128) -> 0x003B09A0
reallocPattern = re.compile('.*SMemRealloc\(0x([0-9A-Fa-f]+), ([0-9]+)\) -> 0x([0-9A-Fa-f]+).*')
#SMemFree(0x003A2E80)
freePattern = re.compile('.*SMemFree\(0x([0-9A-Fa-f]+)\).*')
#Heap usage: Free size=24400, Largest Free Size=24392
memUsagePattern = re.compile('.*Mem usage: HF=(?P<HeapFree>[0-9]+),\s*HT=(?P<HeapTotal>[0-9]+),\s*SM=(?P<StackMax>[0-9]+),\s*ST=(?P<StackTotal>[0-9]+)')

#Unnamed memory activities
actUnnamed='--------------- '


#For display
colModuleName=0
colHeapSize=1
colFreeSizeBytes=2
colFreeSizePercent=3
colLiveBlockNum=4
colStackTotal=5
colStackMaxUsage=6
colNumber=7

helperWithoutExit='* TF Memory Profiler'
helper='* TF Memory Profiler. Please press ENTER to exit ...'
beautiful='------------------------------------------------'
colNames=('Module name', 'Heap size', 'Free size', 'Free size (%)', 'Live blocks', 'Stack size', 'Stack max usage')
colSeps= ('-----------', '---------', '---------', '-------------', '-----------', '----------', '---------------')
lowMemSign='  !!!'
lowNotPrecise='(estim.)'
adbReturnedError='Adb Tool returned an error. Please press ENTER to exit ...'
adbNoRespond='Adb Tool is not responding. Please press ENTER to exit ...'
adbAttempt='Adb Tool: attempt to connect...'

# Columns' indents
colIndent=16


#Recovery setup
recoveryTime=5       #Timeout to wait adb result
recoveryCounter=20   #Number of recovery attempts


#Parameters of heap algo
memBlockServiceLength=8
memBlockAlignLength=8


#------------------------------------------
# Global variables
#------------------------------------------ 

#Service data dictionary
srvIn = {}
#Flag of data update
opHeapUsage=False

#Warning messages
mesText=[]


colEnabled=[True, False, False, False, False, False, False]

defDuration=0
defRefresh=1
noDataUpdateCounter=0


last_btime=0
last_atime=0
memactLine=''

#------------------------------------------
# Options
#------------------------------------------ 
gOptions = ''
wMode=''
modeAndroid='android'
modeWinSim='win_simu'
usage = '%prog [Options] Mode\n\n' + 'Mode: \n' + '   ' + modeAndroid + '            Mode = Android mode is enabled, based on ADB\n' + '   ' + modeWinSim + '           Mode = Win32 Simulator mode is enabled'
parser=OptionParser(usage, version=mpTitleWithVersion)
#------------------------------------------
# Main function, script entry point
#------------------------------------------
def main():   
   ParseCmd()   
   
   if sys.platform == 'win32':
      os.system('title '+ mpTitle)
   elif sys.platform in ('linux2', 'linux3'):
      sys.stdout.write('\x1b]2;' + mpTitle + '\x07')   
   else:
      print mpTitleWithVersion 
      print 'Error -> Current version of Memory Profiler supports only Win32 and Linux platforms\n'
      exit (1)               
      
   
   if wMode == modeWinSim:
      if sys.platform != 'win32':   
         print mpTitleWithVersion 
         print 'Error -> Win32 Simulator mode is allowed only for Windows platform\n'      
         exit (1)         
      

      inputList=[]
      inputList.append(os.environ['SIMULATOR_EXE'])
      for el in os.environ['SIMULATOR_ARGS'].split():
         inputList.append(el)
      
      #c=CommandWinSimu([os.environ['SIMULATOR_EXE'], os.environ['SERVICE_BIN']], gOptions.duration, gOptions.refresh, callback_filter, callback_collect, callback_display)
      c=CommandWinSimu(inputList, gOptions.duration, gOptions.refresh, callback_filter, callback_collect, callback_display)
   elif wMode == modeAndroid:
      c=CommandBoard(['adb','shell','dmesg'], gOptions.duration, gOptions.refresh, callback_filter, callback_collect, callback_display)

      
   if gOptions.duration==0:      
      c.start()   
      s = raw_input()
      c.stop()
      c.join()
   else:
      c.run()
         
#------------------------------------------------------------
def ParseCmd():
   global gOptions
   global wMode

   # Parse the command line

   parser.add_option("--duration", dest="duration", type="int", \
      help="Run duration") 
           
   parser.add_option("--refresh", dest="refresh", type="int", \
           help="Refresh period")

   parser.add_option("--service", dest="service", type="string", \
           help="Name of service to profile")           
    
   parser.add_option("--noFreebytes", action="store_false", dest="colFreeSizeBytes", default=True, \
            help='"%s" column is turned off' % (colNames[colFreeSizeBytes]))    
   
   parser.add_option("--noHeapSize", action="store_false", dest="colHeapSize", default=True, \
            help='"%s" column is turned off' % (colNames[colHeapSize]))                
           
   parser.add_option("--noFreepercent", action="store_false", dest="colFreeSizePercent", default=True, \
            help='"%s" column is turned off' % (colNames[colFreeSizePercent]))    

   parser.add_option("--noLiveblocks", action="store_false", dest="colLiveBlockNum", default=True, \
            help='"%s" column is turned off' % (colNames[colLiveBlockNum]))
            
   parser.add_option("--noStackMaxUsage", action="store_false", dest="colStackMaxUsage", default=True, \
            help='"%s" column is turned off' % (colNames[colStackMaxUsage]))            
            
   parser.add_option("--noStackTotal", action="store_false", dest="colStackTotal", default=True, \
            help='"%s" column is turned off' % (colNames[colStackTotal]))                        
           
   parser.add_option("--noMes", action="store_true", dest="noMes", default=False, \
            help='Debug messages are disabled')
    
  
   (gOptions, args) = parser.parse_args(sys.argv[1:len(sys.argv)])
   
   if gOptions.duration is None:
      gOptions.duration=defDuration
   if gOptions.duration < 0:
      print mpTitleWithVersion 
      print "gOptions.duration=", gOptions.duration
      print "Error -> ParseCmd: Run duration has not to be less than zero\n"      
      parser.print_help(file = sys.stderr) 
      exit(2)           

   if gOptions.refresh is None:
      gOptions.refresh=defRefresh      
   if gOptions.refresh <= 0:
      print mpTitleWithVersion 
      print "Error -> ParseCmd: Refresh time has not to be less or equal to zero\n"      
      parser.print_help(file = sys.stderr)        
      exit(2)    

   if sys.argv[-1] == modeAndroid:
      wMode=modeAndroid
   elif sys.argv[-1]== modeWinSim:
      wMode=modeWinSim
   else:
      print mpTitleWithVersion 
      parser.print_help(file = sys.stdout)       
      exit(0)      
      
   
   colEnabled[colFreeSizeBytes]=gOptions.colFreeSizeBytes
   colEnabled[colHeapSize]=gOptions.colHeapSize
   colEnabled[colFreeSizePercent]=gOptions.colFreeSizePercent
   colEnabled[colLiveBlockNum]=gOptions.colLiveBlockNum
   colEnabled[colStackMaxUsage]=gOptions.colStackMaxUsage
   colEnabled[colStackTotal]=gOptions.colStackTotal   

#------------------------------------------               
   
def callback_filter(line):
   global last_btime
   global last_atime
   global memactLine
   
   if dmesgPattern.match(line):
      matchedLine = dmesgPattern.match(line)
      if (int(matchedLine.group('bdot'))>int(last_btime) or (int(matchedLine.group('bdot'))==int(last_btime) and int(matchedLine.group('adot'))>=int(last_atime))):
         if memactLine!=matchedLine.group('tf_log'):                  
            last_btime=matchedLine.group('bdot')
            last_atime=matchedLine.group('adot')
            memactLine = matchedLine.group('tf_log')      
            return memactLine
         
   return None   
#------------------------------------------               
def findUnnamed(serviceName, servicePID):
   if serviceName!=actUnnamed and srvIn.has_key(actUnnamed):      
      srvdata_src = srvIn[actUnnamed]
      for blockAddress, LBN in srvdata_src.liveBlocks.items():
         if LBN.pid==servicePID and srvIn.has_key(serviceName):
            #Have to copy this block
            srvdata_dest = srvIn[serviceName]               
            #Copy
            LBNNew=LiveBlocksData()
            LBNNew.blockSize=LBN.blockSize
            LBNNew.pid=LBN.pid
            srvdata_dest.liveBlocks[blockAddress] =  LBNNew
            #Delete       
            del srvdata_src.liveBlocks[blockAddress]
               
#------------------------------------------               
def callback_collect(line):
   global opHeapUsage
   global actUnnamed
  
   if wholeLinePattern.match(line):
      wholeLine=wholeLinePattern.match(line)
   else:
      return  
     
   soughtLine=wholeLine.group('memact')
   serviceName = wholeLine.group('name')   
   servicePID = wholeLine.group('pid')   
   
   if gOptions.service and gOptions.service.lower() not in serviceName.lower():
      return

#-----------------------------------------------------------
   if allocPattern.match(soughtLine):
   
      blockInfo = allocPattern.match(soughtLine)
      blockSize = blockInfo.group(1)
      blockAddress = blockInfo.group(3)
      
      if srvIn.has_key(serviceName):
         srvdata = srvIn[serviceName]

      else:         
         srvdata = ServiceInfo()
         srvdata.liveBlocks = {}
         srvIn[serviceName]=srvdata    

      LBN=LiveBlocksData()
      LBN.blockSize=blockSize
      LBN.pid=servicePID         
      srvdata.liveBlocks[blockAddress] =  LBN

      findUnnamed(serviceName, servicePID)   
#-----------------------------------------------------------
   elif reallocPattern.match(soughtLine):
      blockInfo = reallocPattern.match(soughtLine)
      blockOldAddress = blockInfo.group(1)
      blockSize = blockInfo.group(2)
      blockNewAddress = blockInfo.group(3)
     
      if srvIn.has_key(serviceName):      
         srvdata = srvIn[serviceName]
         #We have to free at first
         if srvdata.liveBlocks.has_key(blockOldAddress):         
            del srvdata.liveBlocks[blockOldAddress]
         else:
            mesText.append('Realloc of unknown block %s' % blockOldAddress)            
      else:    
         mightBe=False      
         srvdata = ServiceInfo()
         srvdata.liveBlocks = {}
         srvIn[serviceName]=srvdata    
        
         mesText.append('Realloc of block %s of unknown module' % blockOldAddress)
      
      #Then new alloc
      LBN=LiveBlocksData()
      LBN.blockSize=blockSize
      LBN.pid=servicePID            
      srvdata.liveBlocks[blockNewAddress] = LBN
      
      findUnnamed(serviceName, servicePID)
#-----------------------------------------------------------      
   elif freePattern.match(soughtLine):
      blockInfo = freePattern.match(soughtLine)
      blockAddress = blockInfo.group(1)     
      
      if srvIn.has_key(serviceName):
         srvdata = srvIn[serviceName]
         if srvdata.liveBlocks.has_key(blockAddress):         
            del srvdata.liveBlocks[blockAddress]
         else:
            mesText.append('Free of unknown block %s' % blockAddress)
         
         findUnnamed(serviceName, servicePID)                  
      else:
         srvdata = ServiceInfo()
         srvdata.liveBlocks = {}
         srvIn[serviceName]=srvdata   
      
         findUnnamed(serviceName, servicePID)         
         
         if srvdata.liveBlocks.has_key(blockAddress):         
            del srvdata.liveBlocks[blockAddress]
         else:
            mesText.append('Free of unknown block %s' % blockAddress)
      
#-----------------------------------------------------------      
   elif memUsagePattern.match(soughtLine):

      usageInfo = memUsagePattern.match(soughtLine)
      l_HeapFreeSize = usageInfo.group('HeapFree')
      l_HeapTotalSize = usageInfo.group('HeapTotal')
      l_StackMaxUsage= usageInfo.group('StackMax')
      l_StackTotalSize = usageInfo.group('StackTotal')      
      
         
      if srvIn.has_key(serviceName):         
         srvdata = srvIn[serviceName]
         srvdata.heapFreeSize=l_HeapFreeSize
         srvdata.heapTotalSize=l_HeapTotalSize
         if (int(srvdata.stackMaxUsage)<int(l_StackMaxUsage)):
            srvdata.stackMaxUsage=l_StackMaxUsage
         if (int(srvdata.stackTotalSize)<int(l_StackTotalSize)):
            srvdata.stackTotalSize=l_StackTotalSize
      
      opHeapUsage=True     
#-----------------------------------------------------------  

def printNoUpdate():
   global noDataUpdateCounter
   
   if not opHeapUsage:   
      print '[No data update', '.'*noDataUpdateCounter,']'
      noDataUpdateCounter=noDataUpdateCounter+1
      if noDataUpdateCounter>colIndent:
         noDataUpdateCounter=0
   else:
      noDataUpdateCounter=0
      print 
      
#------------------------------------------               

def callback_display(curTime, wholeTime):
      global opHeapUsage
      global mesDisabled      
      
      os.system([ 'clear', 'cls' ][ os.name == 'nt' ] )   
#-----------------------------------------------------------     
      #Header
      if wholeTime > 0:
         print beautiful
         print helperWithoutExit,
         printNoUpdate()
         print beautiful
         print '%.2f seconds out of  %.2f seconds' % (curTime, wholeTime)  
         print
      else:
         print beautiful
         print helper,
         printNoUpdate()
         print beautiful
         
#-----------------------------------------------------------        
      #Columns names   
      for i in range(colNumber):
         if colEnabled[i]:
            print colNames[i], " " * (colIndent-len(colNames[i])),
      print
      
      for i in range(colNumber):
         if colEnabled[i]:
            print colSeps[i], " " * (colIndent-len(colSeps[i])),
      print
      print
#-----------------------------------------------------------        
      #Columns values      
      for srv_name, srv_data in srvIn.items():
         if srv_name == actUnnamed:
            continue
         
         #Name   
         if colEnabled[colModuleName]:
            print srv_name, " " * (colIndent-len(srv_name)), 
           
         #Heap size
         if colEnabled[colHeapSize]:
            print srv_data.heapTotalSize, " " * (colIndent-len(srv_data.heapTotalSize)),   
         
         #Free bytes
         if colEnabled[colFreeSizeBytes]:
            print srv_data.heapFreeSize, " " * (colIndent-len(srv_data.heapFreeSize)), 
            
         #Free percent
         if colEnabled[colFreeSizePercent] and srv_data.heapTotalSize!='0':            
            percent=int(srv_data.heapFreeSize)*100/int(srv_data.heapTotalSize)
            print percent,
            if percent<10:
               print lowMemSign, " " * (colIndent-1-len(str(percent))-len(lowMemSign)), 
            else:
               print " " * (colIndent-len(str(percent))), 
               
         #Live blocks 
         if colEnabled[colLiveBlockNum]:
            print len(srv_data.liveBlocks), " " * (colIndent-len(str(len(srv_data.liveBlocks)))),
         
         if not srv_data.stackInit and int(srv_data.stackTotalSize)==int(srv_data.stackMaxUsage):
             #Stack total size
             if colEnabled[colStackTotal]:
                print '?', " " * (colIndent-len('?')),                
             #Stack max usage
             if colEnabled[colStackMaxUsage]:
                print '?', " " * (colIndent-len('?')),
         
         else:   
             srv_data.stackInit=True
             #Stack total size
             if colEnabled[colStackTotal]:
                print srv_data.stackTotalSize, " " * (colIndent-len(srv_data.stackTotalSize)),                
             #Stack max usage
             if colEnabled[colStackMaxUsage]:
                print srv_data.stackMaxUsage,
           
             if ((int(srv_data.stackTotalSize)-int(srv_data.stackMaxUsage))*100)/int(srv_data.stackTotalSize) <10:
                print lowMemSign, " " * (colIndent-1-len(str(srv_data.stackMaxUsage))-len(lowMemSign)), 
             else:
                print " " * (colIndent-len(str(srv_data.stackMaxUsage))), 
         
         
         print
#-----------------------------------------------------------        
      #Messages
      if not gOptions.noMes:
         if len(mesText)>0:
            print '\nMESSAGES'
            for strx in mesText:
               print strx

      print 
      
      opHeapUsage=False      
      
   
#------------------------------------------
# Thread class
#------------------------------------------            
class Command(threading.Thread):
   def __init__(self, cmd, timeout,interval, cb_filter, cb_collect, cb_display):
      threading.Thread.__init__(self)
      self.cmd = cmd
      self.process = None
      self.timed_out = False
      self.keeptimeout = timeout
      self.timeout = timeout
      if timeout==0:
         self.infinite=True
      else:
         self.infinite=False         
      self.interval = interval
      self.cb_filter = cb_filter
      self.cb_collect = cb_collect
      self.cb_display = cb_display
      self.stopPolling=False
      
   def stop(self):
      self.stopPolling=True
      print "Please wait while exiting"
               
   def terminate_process(self, pid):
      # this is because we are stuck with Python 2.5 and we cannot use Popen.terminate()
      if sys.platform == 'win32':
         import ctypes
         PROCESS_TERMINATE = 1
         handle = ctypes.windll.kernel32.OpenProcess(PROCESS_TERMINATE, False, pid)
         ctypes.windll.kernel32.TerminateProcess(handle, -1)
         ctypes.windll.kernel32.CloseHandle(handle)
      else:
         os.kill(pid, subproces, signal.SIGKILL)
      
class CommandWinSimu(Command):
   def __init__(self, cmd, timeout,interval, cb_filter, cb_collect, cb_display):
      Command.__init__(self, cmd, timeout,interval, cb_filter, cb_collect, cb_display)


   def run(self):
      self.process = subprocess.Popen(self.cmd,
                                      stdout = subprocess.PIPE, 
                                      stderr=subprocess.STDOUT)
      
      t = Thread(target=self.enqueue_output, name='"Put to Queue"', args=(self.process.stdout, self.stopPolling))
      t.setDaemon(True)
      t.start()
                                      
      
      while (self.process.poll() is None and (self.timeout >= 0 or self.infinite == True) and not self.stopPolling):         
         self.cb_display(self.keeptimeout-self.timeout, self.keeptimeout)
         time.sleep(self.interval)
         if not self.infinite :
            self.timeout-=self.interval
         
      if self.timeout <= 0 or self.stopPolling:
         try:
            self.terminate_process(self.process.pid)
         except:
            pass            
         self.timed_out = True
      else:
         self.timed_out = False
              
   def enqueue_output(self, out, stopFlag):
      try:
         line = out.readline()         
         while line!=None and not stopFlag:       
            self.cb_collect(line.lstrip())
            line = out.readline()
      except:
         pass
         


class CommandBoard(Command):
   def __init__(self, cmd, timeout,interval, cb_filter, cb_collect, cb_display):
      Command.__init__(self, cmd, timeout,interval, cb_filter, cb_collect, cb_display)

   def run(self):

      locRecoveryCounter=0
      
      while ((self.timeout >= 0 or self.infinite == True) and not self.stopPolling):

         hLog = open ("log01.txt",'w')
         self.process = subprocess.Popen(self.cmd, stdin=None, stdout = hLog, stderr=subprocess.STDOUT)         

         
         locRecoveryTime=0
         pollres=0
         start_time=time.clock()         
         pollres=self.process.poll()
         while (pollres==None) and ( time.clock() - start_time < recoveryTime):            
            pollres=self.process.poll()

         
         if pollres!=0:  
            locRecoveryCounter+=1
            if  locRecoveryCounter>=recoveryCounter:               
               if pollres==None:
                  print adbNoRespond
                  self.terminate_process(self.process.pid)
               else:
                  print adbReturnedError
               return   
            else:
               print adbAttempt
               if pollres==None:
                  self.terminate_process(self.process.pid)
               continue
          
         
         hLog = open ("log01.txt",'r')
         while True:
            line = hLog.readline()
            if not line:
               break
            
            lMemactLine=self.cb_filter(line)            
            if lMemactLine:
               self.cb_collect(lMemactLine)
               
         hLog.close()                        
         self.cb_display(self.keeptimeout-self.timeout, self.keeptimeout)
         time.sleep(self.interval)
         if not self.infinite :
            self.timeout-=self.interval            

#------------------------------------------
# Service data
#------------------------------------------         
class LiveBlocksData:
   blockSize='0'  
   pid=''

class ServiceInfo:
  heapFreeSize = '0'
  heapTotalSize='0'
  stackMaxUsage = '0'
  stackTotalSize = '0'  
  stackInit = False
  liveBlocks = None  
       
main()