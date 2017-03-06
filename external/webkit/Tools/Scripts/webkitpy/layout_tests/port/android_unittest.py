# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import string
import unittest
import os
import android

class AdbServerProcessTest(unittest.TestCase):
    def make_port(self):
        port_obj = android.AndroidPort()
        return port_obj

    def test_pidof_service(self):
        class MockExecute:
            def run_command(self,
                            args,
                            cwd=None,
                            input=None,
                            error_handler=None,
                            return_exit_code=False,
                            return_stderr=True,
                            decode_output=False):
                cmd = string.join(args, " ")

                if return_exit_code:
                    return 0

                if cmd.endswith("adb shell ps"):
                    return """
USER     PID   PPID  VSIZE  RSS     WCHAN    PC         NAME
root      1     0     328    196   c00f7c18 000086cc S /init
root      2     0     0      0     c008fbbc 00000000 S kthreadd
root      3     2     0      0     c007dd74 00000000 S ksoftirqd/0
root      6     2     0      0     c00b08cc 00000000 S migration/0
root      7     2     0      0     c00b08cc 00000000 S migration/1
root      9     2     0      0     c007dd74 00000000 S ksoftirqd/1
root      10    2     0      0     c007dd74 00000000 S org.webkit.dumprendertree
"""
                raise RuntimeError("Unexpected cmd" + cmd)
        port = android.AndroidPort()

        port._executive = MockExecute()
        adb_server = android.AdbServerProcess(port, None, None, port._executive, 1, 2)
        self.assertEquals(10, adb_server._pid_of_service())


class AndroidTestBaselines(unittest.TestCase):
    def test_baselines(self):
        port_obj = android.AndroidPort()

        test_filename = port_obj._filesystem.join(port_obj.layout_tests_dir(), "fast/encoding/hebrew/8859-8-e.html")
        expected_filename = port_obj.expected_filename(test_filename, ".txt")
        exp = port_obj._filesystem.join(port_obj.layout_tests_dir(), "fast/encoding/hebrew/8859-8-e-expected.txt")
        self.assertEquals(exp, expected_filename)

        test_filename = port_obj._filesystem.join(port_obj.layout_tests_dir(), "android/fast/encoding/denormalised-voiced-japanese-chars.html")
        expected_filename = port_obj.expected_filename(test_filename, ".txt")
        exp = port_obj._filesystem.join(port_obj.layout_tests_dir(), "android/fast/encoding/denormalised-voiced-japanese-chars-expected.txt")
        self.assertEquals(exp, expected_filename)

class AndroidTestEnv(unittest.TestCase):
    def test_env(self):
        old = os.environ['ANDROID_PRODUCT_OUT']

        os.environ['ANDROID_PRODUCT_OUT'] = '/a/b/c/d'
        port_obj = android.AndroidPort()
        self.assertEquals(port_obj._build_path("build"), "/a/b/c/d/build")

        os.environ['ANDROID_PRODUCT_OUT'] = old

    def test_missing_env(self):
        old = os.environ['ANDROID_PRODUCT_OUT']

        del os.environ['ANDROID_PRODUCT_OUT']
        port_obj = android.AndroidPort()
        self.assertEquals(port_obj.check_sys_deps(False), False)

        os.environ['ANDROID_PRODUCT_OUT'] = old

class AndroidTestName(unittest.TestCase):
    def test_name(self):
        port_obj = android.AndroidPort()
        self.assertEquals(port_obj.name(), "android")

class AndroidTestBuildType(unittest.TestCase):
    def test_build_type(self):
        port_obj = android.AndroidPort()
        # Only release currently supported
        self.assertEquals(port_obj.test_configuration().build_type, "release")

class AndroidTestAdbLsOutputParse(unittest.TestCase):
    def test_parse(self):
        output="""
        /sdcard/webkit/layout-tests:
drwxrwxr-x root     sdcard_rw          2011-09-20 10:25 dom
drwxrwxr-x root     sdcard_rw          2011-09-20 10:23 fast
drwxrwxr-x root     sdcard_rw          2011-09-20 10:30 http
drwxrwxr-x root     sdcard_rw          2011-09-20 10:21 platform
drwxrwxr-x root     sdcard_rw          2011-09-20 10:22 storage

/sdcard/webkit/layout-tests/dom:
drwxrwxr-x root     sdcard_rw          2011-09-20 10:25 html
drwxrwxr-x root     sdcard_rw          2011-09-20 10:29 xhtml

/sdcard/webkit/layout-tests/dom/html:
drwxrwxr-x root     sdcard_rw          2011-09-20 10:25 level1
drwxrwxr-x root     sdcard_rw          2011-09-20 10:23 level2

/sdcard/webkit/layout-tests/dom/html/level1:
drwxrwxr-x root     sdcard_rw          2011-09-20 10:25 core

/sdcard/webkit/layout-tests/dom/html/level1/core:
-rw-rw-r-- root     sdcard_rw       96 2011-09-06 06:49 documentgetdoctypenodtd-expected.txt
-rw-rw-r-- root     sdcard_rw      598 2011-09-06 06:49 documentgetdoctypenodtd.html
-rw-rw-r-- root     sdcard_rw     2922 2011-09-06 06:49 documentgetdoctypenodtd.js
"""
        def files_not_synched(filename):
            return filename.endswith('-expected.txt')

        files = android.parse_android_lsRl("/sdcard/webkit/layout-tests", output, files_not_synched)
        self.assertEquals(files["dom/html/level1/core/documentgetdoctypenodtd.html"], 1315291799)
        self.assertEquals(files.get("dom/html/level1/core"), None)
        self.assertEquals(files.get("dom/html/level1/core/documentgetdoctypenodtd-expected.txt"), None)

if __name__ == '__main__':
    unittest.main()
