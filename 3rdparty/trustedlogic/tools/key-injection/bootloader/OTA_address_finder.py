import string
import sys
import os
from optparse import OptionParser
import re
import pdb
import shutil
import binascii


#------------------------------------------
# Slots management
#------------------------------------------
# Offset in the Bootloader who gives the address of the Slots index
gGlobalIndex = 0x44
gGlobalIndexSize = 4
# Pattern to search for slots index
nSlotsIndexPatternHex = 0xe0beaddee1beaddee2beaddee3beaddee4beaddee5beadde
nSlotsIndexPatternHexSize = len("deadbee0") * 6
# List of patterns (One pattern per slot)
gSSlotsPatternsAscii = ["deadbee0", "deadbee1", "deadbee2", "deadbee3", "deadbee4", "deadbee5"]
# List of slots sizes (One size per slot : 1k for pattern deadbee0, 8k for pattern deadbee1...)
gSSlotsSizes = [1024, 8*1024, 256, 8*1024, 256, 1024]


#------------------------------------------
# Globals used by the script
#------------------------------------------
gOptions=""
gWorkingFile=""
gBlockSize = 512


#------------------------------------------
# internal functions
#------------------------------------------
def PrintInfo():
    global gOptions
    global gWorkingFile

    # Print execution information
    print ""
    print "Input binary: " + os.path.abspath(gOptions.input)
    print "Output binary: " + os.path.abspath(gOptions.output)
    print "Working file: " + gWorkingFile
    print ""


def ParseCmd():
    global gOptions
    global gWorkingFile

    # Parse the command line
    parser = OptionParser()
    parser.add_option("--input", dest="input", type="string", \
           help="<bootloader path>: mandatory argument, the full path of the bootloader that must be provisioned.")
    parser.add_option("--output", dest="output", type="string", \
           help="<ouput bootloader path>: optional argument, the full path of the updated bootloader. If not defined, original bootloader is updated.")

    (gOptions, args) = parser.parse_args()
    if gOptions.input == None or gOptions.input.strip() == None:
        print "Error -> ParseCmd: bad command line."
        parser.print_help(file = sys.stderr)
        exit(1)
    elif not os.path.exists(gOptions.input):
        print "Error -> ParseCmd: input file not found."
        parser.print_help(file = sys.stderr)
        exit(1)
    elif os.path.isdir(gOptions.input):
        print "Error -> ParseCmd: input file is a diretory."
        parser.print_help(file = sys.stderr)
        exit(1)

    gOptions.input = os.path.abspath(gOptions.input)
    gWorkingFile = gOptions.input + ".addr_patched"
    shutil.copy(gOptions.input, gWorkingFile)

    if not gOptions.output == None:
        # output defined, check path
        gOptions.output = os.path.abspath(gOptions.output)
        if not os.path.exists(os.path.dirname(os.path.abspath(gOptions.output))):
            print "ERROR -> ParseCmd: output directory not found: " +os.path.dirname(gOptions.output)
            exit(1)

    #PrintInfo()


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
    global gWorkingFile
    global nSlotsIndexPatternHex
    global gSSlotsPatternsAscii
    global gSSlotsSizes
    dContent2Write = {}
    nbSlots = len(gSSlotsPatternsAscii)
    slotsIndexEntries = []
    slotsAddrs = []


    print ""
    ParseCmd()
    sInputFile = gOptions.input

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

    # Update bootloader file
    dContent2Write[gGlobalIndex] = slotsIndexEntries[0]
    for i in range(0, nbSlots ):
        dContent2Write[slotsIndexEntries[i]] = slotsAddrs[i]
    WriteAddrHexVal(sInputFile, gWorkingFile, dContent2Write, gGlobalIndexSize)

    if gOptions.output == None:
      shutil.move(gWorkingFile, gOptions.input)
    else:
      shutil.move(gWorkingFile, gOptions.output)

    print ""
    print "Binary successfully updated."
main()

