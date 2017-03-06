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
# List of slots sizes (One size per slot : 1k for pattern deadbee0, 8k for pattern deadbee1...)
gSSlotsSizes = [1024, 8*1024, 256, 8*1024, 256, 1024]

# The maximal size that the generated key and certificate can occupy
gKeyMaxSize = gSSlotsSizes[2]
gCertMaxSize = gSSlotsSizes[1]
gHDCPMaxSize = gSSlotsSizes[0]
gWidevineMaxSize = gSSlotsSizes[5]

#------------------------------------------
# Globals used by the script
#------------------------------------------
# A well known buffer will be encoded with the SBK key.
# DO NOT modify the value of these buffers !!
SBK_temp_buffer = "59e2636124674ccd840cb42c8e84f116"
iv = "00000000000000000000000000000000"

gOptions = ""

gWorkingHDCPFile = "HDCP_key.dat"

encrypted_wcert = ""
encrypted_wkey = ""
encrypted_cert = ""
encrypted_key = ""
encrypted_HDCP_key = ""
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
    parser.add_option("--wmdrmpd_key", dest="wmdrmpd_key", type="string", \
           help="<wmdrmpd_key>: mandatory argument, the full path of wmdrm key")
    parser.add_option("--wmdrmpd_cert", dest="wmdrmpd_cert", type="string", \
           help="<wmdrmpd_cert>: mandatory argument, full path of wmdrm certificate")
    parser.add_option("--prdy_key", dest="prdy_key", type="string", \
           help="<prdy_key>: mandatory argument, full path of playready key")
    parser.add_option("--prdy_cert", dest="prdy_cert", type="string", \
           help="<prdy_cert>: mandatory argument, full path of playready certificate")
    parser.add_option("--hdcp_key", dest="hdcp_key", type="string", \
           help="<hdcp_key>: optional argument, file that contains the key for the HDCP service.")
    parser.add_option("--widevine_key", dest="widevine_key", type="string", \
           help="<widevine_key>: optional argument, file that contains the key for the Widevine service.")

    (gOptions, args) = parser.parse_args()

    # SBK key to encrypt, should not be void
    if gOptions.sbk == None:
        print "Error -> ParseCmd: the SBK is a mandatory argument, should not be void."
        parser.print_help(file = sys.stderr)
        exit(1)
    # Input path of wmdrm key should not be void
    if gOptions.wmdrmpd_key != None or gOptions.wmdrmpd_cert != None \
       or gOptions.prdy_key != None or gOptions.prdy_cert != None:
        if gOptions.wmdrmpd_key == None or gOptions.wmdrmpd_key.strip() == None:
            print "Error -> ParseCmd: the path of wmdrm key should not be void."
            parser.print_help(file = sys.stderr)
            exit(1)
        elif not os.path.isfile(gOptions.wmdrmpd_key):
            print "Error -> ParseCmd: the indicated wmdrm key does't exist:"
            print "        " + gOptions.wmdrmpd_key
            parser.print_help(file = sys.stderr)
            exit(1)
        # Input path of wmdrm certificate should not be void
        if gOptions.wmdrmpd_cert == None or gOptions.wmdrmpd_cert.strip() == None:
            print "Error -> ParseCmd: the path of wmdrm certificate should not be void."
            exit(1)
        elif not os.path.isfile(gOptions.wmdrmpd_cert):
            print "Error -> ParseCmd: the indicated wmdrm certificate does't exist:"
            print "        " + gOptions.wmdrmpd_cert
            parser.print_help(file = sys.stderr)
            exit(1)
        # Input path of playready key should not be void
        if gOptions.prdy_key == None or gOptions.prdy_key.strip() == None:
            print "Error -> ParseCmd: the path of play ready key should not be void."
            parser.print_help(file = sys.stderr)
            exit(1)
        elif not os.path.isfile(gOptions.prdy_key):
            print "Error -> ParseCmd: the indicated play ready key does't exist:"
            print "        " + gOptions.prdy_key
            parser.print_help(file = sys.stderr)
            exit(1)
        # Input path of model certificate should not be void
        if gOptions.prdy_cert == None or gOptions.prdy_cert.strip() == None:
            print "Error -> ParseCmd: the path of prdy cert should not be void."
            parser.print_help(file = sys.stderr)
            exit(1)
        elif not os.path.isfile(gOptions.prdy_cert):
            print "Error -> ParseCmd: the indicated play ready certificate does't exist:"
            print "        " + gOptions.prdy_cert
            parser.print_help(file = sys.stderr)
            exit(1)

    # Input path of HDCP should not be void
    if gOptions.hdcp_key != None:
        if not os.path.isfile(gOptions.hdcp_key):
            print "Error -> ParseCmd: the indicated file for HDCP key does't exist:"
            print "        " + gOptions.hdcp_key
            parser.print_help(file = sys.stderr)
            exit(1)

        # Convert the HDCP key structure in a binary file
        hdcp_key_txt = open(gOptions.hdcp_key, "r")
        hdcp_key_txt_buff = hdcp_key_txt.read()
        hdcp_key_txt_list = hdcp_key_txt_buff.split(", ")
        # Check the private key length is 312 bytes
        if len(hdcp_key_txt_list) != 312:
            print "TError -> he HDCP private key should be 312 bytes length and the format must be '0x00, 0x00 ...'"
            exit(1)

        # Convert to binary
        hdcp_key_bin_buff = ""
        for i in range(312):
            hdcp_key_bin_buff = hdcp_key_bin_buff + pack('B', int(hdcp_key_txt_list[i], 16))
        hdcp_key_txt.close()
        hdcp_key_bin = open(gWorkingHDCPFile, "wb")
        hdcp_key_bin.write(hdcp_key_bin_buff)
        hdcp_key_bin.close()

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
    global gWorkingHDCPFile
    global encrypted_wcert
    global encrypted_wkey
    global encrypted_cert
    global encrypted_key
    global encrypted_HDCP_key
    global encrypted_widevine_key
    global gKeyMaxSize
    global gCertMaxSize
    global gHDCPMaxSize
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

    hEncDataFile = open ("encrypted_data.txt",'w')

    # Encrypt the data
    print "Encrypt data with TF_SBK."
    if gOptions.wmdrmpd_cert != None:
        encrypted_wcert = Exec([openssl, 'aes-128-cbc', '-in', gOptions.wmdrmpd_cert, '-K', key.encode('hex'), '-iv', iv])
        if len(encrypted_wcert) > gCertMaxSize:
            print "Error -> GetEncryptedSBk: encrypted wmdrm certificate exceeds "+str(gCertMaxSize)+"."
            exit(1)

        encrypted_wkey  = Exec([openssl, 'aes-128-cbc', '-in', gOptions.wmdrmpd_key, '-K', key.encode('hex'), '-iv', iv])
        if len(encrypted_key) > gKeyMaxSize:
            print "Error -> GetEncryptedSBk: encrypted wmdrm key exceeds "+str(gKeyMaxSize)+"."
            exit(1)

        encrypted_cert  = Exec([openssl, 'aes-128-cbc', '-in', gOptions.prdy_cert, '-K', key.encode('hex'), '-iv', iv])
        if len(encrypted_cert) > gCertMaxSize:
            print "Error -> GetEncryptedSBk: encrypted certificate exceeds "+str(gCertMaxSize)+"."
            exit(1)

        encrypted_key   = Exec([openssl, 'aes-128-cbc', '-in', gOptions.prdy_key, '-K', key.encode('hex'), '-iv', iv])
        if len(encrypted_key) > gKeyMaxSize:
            print "Error -> GetEncryptedSBk: encrypted key exceeds "+str(gKeyMaxSize)+"."
            exit(1)

        hEncDataFile.write(encrypted_wcert.encode('hex'))
        hEncDataFile.write('\r\n')

        hEncDataFile.write(encrypted_wkey.encode('hex'))
        hEncDataFile.write('\r\n')

        hEncDataFile.write(encrypted_cert.encode('hex'))
        hEncDataFile.write('\r\n')

        hEncDataFile.write(encrypted_key.encode('hex'))
        hEncDataFile.write('\r\n')
    else:
        hEncDataFile.write('\r\n')
        hEncDataFile.write('\r\n')
        hEncDataFile.write('\r\n')
        hEncDataFile.write('\r\n')

    if gOptions.hdcp_key != None:
        encrypted_HDCP_key   = Exec([openssl, 'aes-128-cbc', '-in', gWorkingHDCPFile, '-K', key.encode('hex'), '-iv', iv])
        if len(encrypted_HDCP_key) > gHDCPMaxSize:
            print "Error -> GetEncryptedSBk: encrypted HDCP key exceeds "+str(gHDCPMaxSize)+"."
            exit(1)

        hEncDataFile.write(encrypted_HDCP_key.encode('hex'))

    hEncDataFile.write('\r\n')

    if gOptions.widevine_key != None:
        encrypted_widevine_key = Exec([openssl, 'aes-128-ecb', '-in', gOptions.widevine_key, '-K', key.encode('hex'), '-iv', iv])
        if len(encrypted_widevine_key) > gWidevineMaxSize:
            print "Error -> GetEncryptedSBk: encrypted widevine exceeds "+str(gWidevineMaxSize)+"."
            exit(1)

        hEncDataFile.write(encrypted_widevine_key.encode('hex'))

    hEncDataFile.write('\r\n')

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
