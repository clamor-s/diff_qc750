import string
import sys
import os
from optparse import OptionParser
import re
import pdb
import shutil
import binascii
import struct


#------------------------------------------
# Slots management
#------------------------------------------
# Offset in the Bootloader who gives the address of the Slots index
gGlobalIndex = 0x44
gGlobalIndexSize = 4
# Pattern to search for slots index
nSlotsIndexPatternHex = 0xe0beaddee1beaddee2beaddee3beaddee4beaddee5beaddee6beadde
nSlotsIndexPatternHexSize = len("deadbee0") * 7
# List of patterns (One pattern per slot)
gSSlotsPatternsAscii = ["deadbee0", "deadbee1", "deadbee2", "deadbee3", "deadbee4", "deadbee5", "deadbee6"]
# List of slots sizes (One size per slot : 1k for pattern deadbee0, 8k for pattern deadbee1...)
gSSlotsSizes = [1024, 8*1024, 256, 8*1024, 256, 1024, 256]


#------------------------------------------
# Globals used by the script
#------------------------------------------
gOptions=""
gWorkingTfFile=""
gBlockSize = 512


#------------------------------------------
# internal functions
#------------------------------------------
def PrintInfo():
    global gOptions
    global gWorkingTfFile

    # Print execution information
    print ""
    print "Input TF binary: " + os.path.abspath(gOptions.inputTf)
    print "Working TF file: " + gWorkingTfFile
    print ""


def ParseCmd():
    global gOptions
    global gWorkingTfFile

    # Parse the command line
    parser = OptionParser()
    parser.add_option("--tf", dest="inputTf", type="string", \
           help="<tf binary path>: mandatory argument, the full path of the TF binary that must be provisioned.")

    (gOptions, args) = parser.parse_args()
    if gOptions.inputTf == None or gOptions.inputTf.strip() == None:
        print "Error -> ParseCmd: bad command line."
        parser.print_help(file = sys.stderr)
        exit(1)
    elif not os.path.exists(gOptions.inputTf):
        print "Error -> ParseCmd: input TF not found."
        parser.print_help(file = sys.stderr)
        exit(1)
    elif os.path.isdir(gOptions.inputTf):
        print "Error -> ParseCmd: input TF is a diretory."
        parser.print_help(file = sys.stderr)
        exit(1)

    gOptions.inputTf = os.path.abspath(gOptions.inputTf)
    gWorkingTfFile = gOptions.inputTf + ".addr_patched"
    shutil.copy(gOptions.inputTf, gWorkingTfFile)

def GetHexPatternAddr(sInputFile, nSlotsIndexPatternHex, nLenHexPattern):
    dPattern = {}
    nIterBlock = 0
    data=''
    sSearchPatternHex = binascii.a2b_hex(("%0" + str(nLenHexPattern) + "x") % nSlotsIndexPatternHex)
    fhdl = open (sInputFile,'rb')
    nSetBack = len (sSearchPatternHex)
    nOffset = -1
    if fhdl != -1:
        while 1:
            data = fhdl.read(gBlockSize)
            if (data != ""):
                nOffset = data.find(sSearchPatternHex)
                if nOffset != -1:
                    fhdl.close()
                    return nIterBlock * gBlockSize - nSetBack * nIterBlock + nOffset
                if (len(data) < gBlockSize):
                    # This means we have reached the end but nothing found
                    fhdl.close()
                    break
                # If pattern not found, make sure the next read include the transistion zoom in case an unlucky data breaks the matching
                fhdl.seek(0-nSetBack, os.SEEK_CUR)
                nIterBlock += 1
            else:
                fhdl.close()
                break
    else:
        print "ERROR -> sSearchPatternHex: could not open input file."

def GetAsiiPatternAddr(sInputFile, sSearchPatternAscii, nStart = 0 ):
    dPattern = {}
    nIterBlock = 0
    data=''
    fhdl = open (sInputFile,'rb')
    nSetBack = len (sSearchPatternAscii)
    nOffset = -1
    # Start the search from nStart position
    fhdl.seek(nStart, os.SEEK_SET)
    if fhdl != -1:
        while 1:
            data = fhdl.read(gBlockSize)
            if (data != ""):
                nOffset = data.find(sSearchPatternAscii)
                if nOffset != -1:
                    fhdl.close()
                    return nIterBlock * gBlockSize - nSetBack * nIterBlock + nOffset + nStart
                if (len(data) < gBlockSize):
                    # This means we have reached the end but nothing found
                    fhdl.close()
                    break
                # If pattern not found, make sure the next read include the transistion zoom in case an unlucky data breaks the matching
                fhdl.seek(-nSetBack, os.SEEK_CUR)
                nIterBlock += 1
            else:
                fhdl.close()
                break
    else:
        print "ERROR -> GetAsciiPatternAddr: could not open input file."



def WriteAddrHexVal(sInputFile,sOutputFile, dContent2Write, nLength):
    global gBlockSize
    data = ''
    nIterBlock = 0
    fhdlIn = open (sInputFile,'rb')
    fhdlOut = open (sOutputFile,'wb')
    if (fhdlIn == -1 or fhdlOut == -1):
        print "ERROR -> WriteAddrHexVal: could not open input or output file."
    else:
        while 1:
            data = fhdlIn.read(gBlockSize);
            if (data != '' or nIterBlock == 0):
                for nAddress, nValue in dContent2Write.iteritems():
                    for i in range (0, nLength):
                        if (nAddress + i >= gBlockSize * nIterBlock) and (nAddress + i < gBlockSize * (nIterBlock + 1)):
                            if (nAddress + i - gBlockSize * nIterBlock == 0):
                                data = binascii.a2b_hex('%02x' % ((nValue & (0xff << i*8)) >> (i)*8)) + data[1:len(data)]
                            elif (nAddress + i - gBlockSize * nIterBlock == len(data)-1) :
                                data = data[0:len(data)-1] + binascii.a2b_hex(format(((nValue & (0xff << i*8)) >> (i)*8),'02x'))
                            else :
                                data = data[0:nAddress + i - gBlockSize * nIterBlock] + \
                                binascii.a2b_hex(('%02x' % ((nValue & (0xff << i*8)) >> (i)*8))) + \
                                data[nAddress + i - gBlockSize * nIterBlock+1:len(data)]
                nIterBlock += 1
                fhdlOut.write(data)
            else:
                break
        fhdlIn.close()
        fhdlOut.close()



#------------------------------------------
# Main function, script entry point
#------------------------------------------
def main():
    global gOptions
    global gWorkingTfFile
    global nSlotsIndexPatternHex
    global gSSlotsPatternsAscii
    global gSSlotsSizes
    dContent2WriteTf = {}
    nbSlots = len(gSSlotsPatternsAscii)
    slotsIndexEntries = []
    slotsAddrs = []


    print ""
    ParseCmd()
    sInputFile = gOptions.inputTf

    # Find Slots index pattern (in hex)

    slotsIndexEntries.append( GetHexPatternAddr (sInputFile, nSlotsIndexPatternHex, nSlotsIndexPatternHexSize) )
    if  slotsIndexEntries[0] == None:
        print "Error -> Slots index not found in binary."
        print "      -> Pattern "+ format(nSlotsIndexPatternHex,'08x') + " could not be found."
        exit (1)
    print "Slots index found at offset 0x" + ('%08x' % slotsIndexEntries[0]) + "."

    # Update entries in Slots index (minus one, entry 0 already set)
    for i in range(1, nbSlots):
       previousEntryAddr = slotsIndexEntries[i-1]
       slotsIndexEntries.append( previousEntryAddr + 4 )

    # Find Slots location (in ascii, service properties are in ascii)
    for i in range(0, nbSlots):
        # duplicate properties, not the real one, need second search
        slotsAddrs.append( GetAsiiPatternAddr(sInputFile, gSSlotsPatternsAscii[i]))
        if  slotsAddrs[i] == None:
            print "Slot location " + str(i) + " not found. Set it to 0x00000000."
            slotsAddrs[i] = 0x00000000
        else:
            # we know that properties is present in the file (first search returns successfully)
            # we search this time the real address (start research after 1st pattern)
            slotsAddrs[i] = GetAsiiPatternAddr(sInputFile, gSSlotsPatternsAscii[i], slotsAddrs[i] + gSSlotsSizes[i])
            print "Slot location " + str(i) + " found at offset 0x" + ('%08x' % slotsAddrs[i]) + "."

    hdrFile = open ("eks_index_offset.h",'w')

    hdrFile.write("#ifndef EKS_INDEX_OFFSET_H\r\n")
    hdrFile.write("#define EKS_INDEX_OFFSET_H\r\n")
    hdrFile.write("/* DO NOT MODIFY DIRECTLY */\r\n")
    hdrFile.write("/* Offset of slot index of encrypted keys */\r\n")
    hdrFile.write("#define EKS_INDEX_OFFSET    ")
    hdrFile.write( "0x" + ('%08x' % slotsIndexEntries[0]) + "\r\n" )
    hdrFile.write("#endif")

    hdrFile.close()

    # contents update for TF
    for i in range(0, nbSlots ):
        dContent2WriteTf[slotsIndexEntries[i]] = slotsAddrs[i]

    WriteAddrHexVal(sInputFile, gWorkingTfFile, dContent2WriteTf, gGlobalIndexSize)

    shutil.move(gWorkingTfFile, gOptions.inputTf)

    print ""
    print "Binary successfully updated."
main()

