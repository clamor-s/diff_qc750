#------------------------------------------
#
# e.g) python wv_encrypt_key.py --sbk=00000000000000000000000000000000 --widevine_key=./wv_good_key.dat
#
# The output file "wv_keybox.dat" has the encrypted keybox
#

from common import *
from struct import *
from optparse import OptionParser
import sys
import os
import pdb
import string
import shutil
import struct


#------------------------------------------
# OpenSSL configuration  :
#------------------------------------------
# Open SSL Path
# Please Choose here either use the openssl tool in the product package or your local installed openssl
# For the second case, please uncomment the second line, replace the example path with
#    hard-coded openssl executable path and then comment the first line.
openssl =  "openssl"

#------------------------------------------
# Globals used by the script
#------------------------------------------
# A well known buffer will be encoded with the SBK key.
# DO NOT modify the value of these buffers !!
SBK_temp_buffer = "59e2636124674ccd840cb42c8e84f116"
iv = "00000000000000000000000000000000"

gOptions = ""

encrypted_widevine_key = ""


#------------------------------------------
# internal functions
#------------------------------------------

def ParseCmd():
    global gOptions

    # Parse the command line
    parser = OptionParser()
    parser.add_option("--sbk", dest="sbk", type="string", \
           help="SBK key value")
    parser.add_option("--widevine_key", dest="widevine_key", type="string", \
           help="<widevine_key>: file that contains the key for the Widevine service.")

    (gOptions, args) = parser.parse_args()

    # SBK key to encrypt, should not be void
    if gOptions.sbk == None:
        print "Error -> ParseCmd: the SBK is a mandatory argument, should not be void."
        parser.print_help(file = sys.stderr)
        exit(1)

    # Input path of widevine should not be void
    if gOptions.widevine_key != None:
        if gOptions.widevine_key == None or gOptions.widevine_key.strip() == None:
            print "Error -> ParseCmd: the path of widevine key should not be void."
            parser.print_help(file = sys.stderr)
            exit(1)
        elif not os.path.isfile(gOptions.widevine_key):
            print "Error -> ParseCmd: the indicated widevine key does't exist:"
            print "        " + gOptions.widevine_key
            parser.print_help(file = sys.stderr)
            exit(1)

def GetEncryptedSBK():
    global gOptions
    global encrypted_widevine_key
    global SBK_temp_buffer
    global iv
    global openssl

    print "Generate TF_SBK."

    # Derive a key from SBK and encrypt data with aes-cbc
    hTempBuffer = open ("temp_buffer.txt",'w')
    hTempBuffer.write (SBK_temp_buffer.decode('hex'))
    hTempBuffer.close()
    key     = Exec([openssl, 'aes-128-cbc', '-in', 'temp_buffer.txt', '-K', gOptions.sbk, '-iv', iv])
    os.remove("temp_buffer.txt")

    hEncDataFile = open ("wv_keybox.dat",'wb')

    # should be 'h' because the length of magic id is 8 including 'null'
    # Encrypt the data
    print "Encrypt data with TF_SBK."

    format = "i"
    if gOptions.widevine_key != None:
        encrypted_widevine_key = Exec([openssl, 'aes-128-ecb', '-in', gOptions.widevine_key, '-K', key.encode('hex'), '-iv', iv])
        hEncDataFile.write(encrypted_widevine_key)
    else:
        data = struct.pack(format, 0)
        hEncDataFile.write(data)

    hEncDataFile.close()

#------------------------------------------
# Main function, script entry point
#------------------------------------------
def main():

    print ""
    ParseCmd()
    GetEncryptedSBK()

    print "Encrypted data file created."
main()
