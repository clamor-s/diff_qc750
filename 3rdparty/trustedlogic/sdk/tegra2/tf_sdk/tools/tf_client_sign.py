# Copyright (c) 2005-2008 Trusted Logic S.A.
# All Rights Reserved.
#
# This software is the confidential and proprietary information of
# Trusted Logic S.A. ("Confidential Information"). You shall not
# disclose such Confidential Information and shall use it only in
# accordance with the terms of the license agreement you entered
# into with Trusted Logic S.A.
#
# TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
# SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
# NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
# MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
#

from common import *
from optparse import OptionParser
import sys
import os
import atexit
from tempfile import *
from string import Template
import uuid
#
# Parse the command line
#

parser = OptionParser()
parser.set_defaults(key=os.path.join(scriptDir, "devkeys", "SM_CA_client_auth_privkey.pem"))
parser.add_option("--exec", dest="exec_file", help="Executable file to sign (mandatory)")
parser.add_option("--manifest", dest="manifest_file", help="Client manifest file (mandatory)")
parser.add_option("--output", dest="output_file", help="Signature file to generate. [Default: EXEC_FILE.ssig]")
parser.add_option("--key", dest="key", help="Private key to use for the signature. [Default: Developer's root key]")
parser.add_option("--cert", dest="cert", action="append", help="Certificate to put in the chain. [Default: none]")
parser.add_option("--verbose", dest="verbose", action="store_true")

(options, args) = parser.parse_args()
if len(args) != 0 or options.exec_file is None or options.manifest_file is None:
   parser.print_help(file = sys.stderr)
   sys.exit()

if options.output_file is None:
   options.output_file = options.exec_file + ".ssig"

openssl=FindOpenSSL()
print Template("""
   File to sign:      ${execFile}
   Manifest file:     ${manifest}
   Signing Key:       ${key}
   Certificate Chain: ${cert}
   Output File:       ${output_file}
   Path to OpenSSL:   ${openssl}

""").substitute(
         execFile=options.exec_file,
         openssl=openssl,
         output_file=options.output_file,
         manifest = options.manifest_file,
         key = options.key,
         cert = options.cert)

#
# Hash the executable
#

print "Hashing %s" % options.exec_file

hash = Exec([openssl, 'dgst', '-sha1', '-binary', options.exec_file])
hash64 = Exec([openssl, 'enc', '-base64'], input=hash).strip()

print "The hash of the executable is: %s" % hash64

#
# Read the manifest and generate the compiled manifest
#
properties = ReadManifest(options.manifest_file)
if u'sm.client.id' not in properties:
   raise Exception("Missing property sm.client.id")
# Convert UUID. This will raise an exception if the UUID is ill-formed
uuid.UUID(properties[u'sm.client.id'])
compiled_manifest = "SSIG"
for (key, value) in properties.iteritems():
   if key.startswith(u'sm.client.') and key != u'sm.client.id':
      raise Exception("Invalid property name: %s (must not start with sm.client." % key)
   compiled_manifest += key.encode('utf-8') + ':' + value.encode('utf-8') + '\n'
compiled_manifest += "sm.client.exec.hash:" + hash64

#
# Sign the manifest
#
print "Signing manifest with %s and generate %s" % (options.key, options.output_file)

cmd=[openssl, "smime",
   "-sign", "-binary", "-noattr", "-nodetach", "-outform", "DER",
   "-out", options.output_file,
   "-inkey", options.key]

if options.cert is None:
   # generate a certificate on the fly for the private key
   certificate = Exec([openssl, "req", "-key", options.key,
         "-x509", "-new", "-subj", "/CN=Unknown/", "-config",
         os.path.join(scriptDir, "config_openssl.txt") ], input=None)
   (fd, name) = mkstemp()
   f = os.fdopen(fd, "wb")
   f.write(certificate)
   f.flush()
   f.close()
   atexit.register(os.remove, name)
   cmd = cmd + ["-nocerts", "-nochain", "-signer", name]
else:
   # the first certificate is the signer's certificate
   # the other ones are certificates in the chain
   cmd = cmd + ["-signer", options.cert[0]]
   # Concatenate all other certificates in a cert chain file
   if len(options.cert) > 1:
      (fd, name) = mkstemp()
      f = os.fdopen(fd, "w");
      for cert in options.cert[1:]:
         f.write(open(cert).read())
      f.close()
      atexit.register(os.remove, name)
      cmd = cmd + ["-certfile", name]

Exec(cmd, input=compiled_manifest)

if options.verbose:
   # Dump the generated pkcs7
   Exec([openssl, "asn1parse", "-inform", "DER", "-in", options.output_file, "-dump", "-i"], output=sys.stdout)
