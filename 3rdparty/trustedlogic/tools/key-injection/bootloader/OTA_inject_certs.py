from common import *
from struct import *
from optparse import OptionParser
import sys
import os
import pdb
import string
import shutil


#------------------------------------------
# OpenSSL configuration  :
#------------------------------------------
# Open SSL Path
# Please Choose here either use the openssl tool in the product package or your local installed openssl
# For the second case, please uncomment the second line, replace the example path with
#    hard-coded openssl executable path and then comment the first line.
openssl =   FindOpenSSL()
#openssl =  "~/system/bin/openssl"

#------------------------------------------
# Slots management
#------------------------------------------
# Offset in the Bootloader who gives the address of the Slots index
gGlobalIndex = 0x44
# List of patterns (One pattern per slot)
gSSlotsPatternsAscii = ["deadbee0", "deadbee1", "deadbee2", "deadbee3", "deadbee4", "deadbee5"]
# List of slots sizes (One size per slot : 1k for pattern deadbee0, 8k for pattern deadbee1...)
gSSlotsSizes = [1024, 8*1024, 256, 8*1024, 256, 1024]

#------------------------------------------
# Globals used by the script
#------------------------------------------

gOptions = ""
gWorkingFile=""
gBlockSize = 512

encrypted_wcert = ""
encrypted_wkey = "" 
encrypted_cert = ""
encrypted_key = ""
encrypted_HDCP_key = ""
encrypted_widevine_key = ""


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
           help="<input bootloader path>: mandatory argument, the full path of the bootloader that must be provisioned.")
    parser.add_option("--output", dest="output", type="string", \
           help="<ouput bootloader path>: optional argument, the full path of the updated bootloader. If not defined, original bootloader is updated.")

    (gOptions, args) = parser.parse_args()
    # Check input argument, should not be void
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
    gWorkingFile = gOptions.input + ".cert_patched"
    shutil.copy(gOptions.input, gWorkingFile)

    if not gOptions.output == None:
        # output defined, check path
        gOptions.output = os.path.abspath(gOptions.output)
        if not os.path.exists(os.path.dirname(os.path.abspath(gOptions.output))):
            print "ERROR -> ParseCmd: output directory not found: " +os.path.dirname(gOptions.output)
            exit(1)

def ReadAddrFromBuff(buff, index):
    addr = int(buff[index + 3].encode('hex'),16)*256**3 + int(buff[index + 2].encode('hex'),16)*256**2 \
           + int(buff[index + 1].encode('hex'),16)*256**1 + int(buff[index + 0].encode('hex'),16)*256**0
    return addr


def GetEncryptedData():
    global encrypted_wcert
    global encrypted_wkey
    global encrypted_cert
    global encrypted_key
    global encrypted_HDCP_key
    global encrypted_widevine_key

    hEncDataFile = open ("encrypted_data.txt",'r')
    lines = hEncDataFile.readlines()

    # lines should be 6
    if (len(lines)==6):
        print "Reading provisioning certifcates"
        encrypted_wcert = lines[0].strip("\r\n")
        encrypted_wkey = lines[1].strip("\r\n")
        encrypted_cert = lines[2].strip("\r\n")
        encrypted_key = lines[3].strip("\r\n")
        print "Reading HDCP key"
        encrypted_HDCP_key = lines[4].strip("\r\n")
        print "Reading widevine key"
        encrypted_widevine_key = lines[5].strip("\r\n")
    else:
        print "Wrong encrypted_data.txt"
        exit(1)

#------------------------------------------
# Main function, script entry point
#------------------------------------------
def main():
    global gOptions
    global gGlobalIndex
    global gWorkingFile
    tAddrList = []
    tSortedAddrList = []

    print ""
    ParseCmd()
    GetEncryptedData()

    if (encrypted_HDCP_key=="" and encrypted_wcert=="" and encrypted_widevine_key==""):
      print "No data to inject"
      return

    # Get the data stockage address
    fhdl = open(gWorkingFile, 'rb')
    if fhdl != -1:
        data = fhdl.read(gBlockSize)
        if (data != ""):
            # The global index entry (gGlobalIndex), in this case 0x44
            nAddressOrigin = ReadAddrFromBuff(data, gGlobalIndex)
            print ""
            print "Slots index found at offset 0x" + ('%08x' % nAddressOrigin) + "."

            # Jumpt to this offset and get the slots entries
            fhdl.seek(nAddressOrigin, os.SEEK_SET)
            data = fhdl.read(gBlockSize)
            if (data != ""):
                # small endian, read full address, the slots addresses are adjacent
                nAddress0 = ReadAddrFromBuff(data, 0)
                nAddress1 = ReadAddrFromBuff(data, 4)
                nAddress2 = ReadAddrFromBuff(data, 8)
                nAddress3 = ReadAddrFromBuff(data, 12)
                nAddress4 = ReadAddrFromBuff(data, 16)
                nAddress5 = ReadAddrFromBuff(data, 20)
            else:
                print "Error -> main: Read of slots index failed."
                fhdl.close()
                exit(1)

        else:
            print "Error -> main: Read address "+('%08x' % gGlobalIndex)+" failed."
            fhdl.close()
            exit(1)
    fhdl.close()
    print "Slot location 0 found at offset 0x" + ('%08x' % nAddress0) + "."
    print "Slot location 1 found at offset 0x" + ('%08x' % nAddress1) + "."
    print "Slot location 2 found at offset 0x" + ('%08x' % nAddress2) + "."
    print "Slot location 3 found at offset 0x" + ('%08x' % nAddress3) + "."
    print "Slot location 4 found at offset 0x" + ('%08x' % nAddress4) + "."
    print "Slot location 5 found at offset 0x" + ('%08x' % nAddress5) + "."

    # Append the objects into a tuple struct (address, encrypted data)
    if encrypted_HDCP_key != "":
        print "Injecting HDCP key"
#         print encrypted_HDCP_key
        if nAddress0 != 0x00000000:
            tAddrList.append((nAddress0, encrypted_HDCP_key.ljust(gSSlotsSizes[0], '\x00')))
        else:
            print "Error -> Slot for HDCP key not found in binary."
            exit(1)

    if encrypted_wcert != "":
        print "Injecting provisioning materials "
#         print encrypted_wcert, encrypted_wkey, encrypted_cert, encrypted_key
        tAddrList.append((nAddress1, encrypted_wcert.ljust(gSSlotsSizes[1], '\x00')))
        tAddrList.append((nAddress2, encrypted_wkey.ljust(gSSlotsSizes[2], '\x00')))
        tAddrList.append((nAddress3, encrypted_cert.ljust(gSSlotsSizes[3], '\x00')))
        tAddrList.append((nAddress4, encrypted_key.ljust(gSSlotsSizes[4], '\x00')))

    if encrypted_widevine_key != "":
        print "Injecting widevine key"
        if nAddress5 != 0x00000000:
            tAddrList.append((nAddress5, encrypted_widevine_key.ljust(gSSlotsSizes[5], '\x00')))
        else:
            print "Error -> Slot for widevine key not found in binary."
            exit(1)

    print ""
    print "Updating binary..."
    fhdl = open(gWorkingFile, 'r+b')

    # loop on the elements and write encrypted data at specified offset
    nbElem = len(tAddrList)
    for elemIndex in range (0, nbElem):
        elemAddr = tAddrList[elemIndex][0]
        elem = tAddrList[elemIndex][1]

        # move to next offset
        fhdl.seek(elemAddr, os.SEEK_SET)
        # write the encrypted data
        fhdl.write( elem )
    fhdl.close()

    if gOptions.output == None:
      shutil.move(gWorkingFile, gOptions.input)
    else:
      shutil.move(gWorkingFile, gOptions.output)

    print "Binary successfully updated."
main()
