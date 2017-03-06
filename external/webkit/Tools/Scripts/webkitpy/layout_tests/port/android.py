#!/usr/bin/env python
# Copyright (C) 2010 Google Inc. All rights reserved.
# Copyright (C) 2011 NVIDIA. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
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
"""Android implementations of the Port interface."""

from __future__ import with_statement

import codecs
import errno
import logging
import os
import re
import shutil
import select
import signal
import subprocess
import sys
import tempfile
import time
import webbrowser

from webkitpy.common.system.path import cygpath
from webkitpy.layout_tests.layout_package import test_expectations

import base
import http_server

import webkit

import websocket_server
import socket
import calendar
import hashlib
_log = logging.getLogger("webkitpy.layout_tests.port.android")

def find_in_path(bin_name):
    path = os.environ['PATH']
    for dir in path.split(os.pathsep):
        bin_path = os.path.join(dir, bin_name)
        if os.path.exists(bin_path):
            return os.path.abspath(bin_path)
    return None


def parse_android_lsl_line(line):
    line = line.strip()
    if len(line) == 0:
        return None

    parts = line.split(None, 6)

    if len(parts) < 6:
        return None

    is_dir = parts[0][0] == 'd'

    # Directories do not have size, so access fields counting from the last one.
    fn = parts[-1]

    # Use :59 to avoid having the loss of precision cause files uploaded.
    mtime = int(calendar.timegm(time.strptime("%s %s:59 UTC" %(parts[-3], parts[-2]), "%Y-%m-%d %H:%M:%S %Z")))

    return {"file_name": fn, "mtime": mtime, "is_dir": is_dir}

def parse_android_lsRl(test_base_dir, output, filter_out_func):
    files_times = {}
    try:
        lines = iter(output.splitlines())
        lines.next()                # empty
        while (True):
            path = lines.next().strip()
            path = path[len(test_base_dir)+1:-1]
            while (True):
                file_entry = parse_android_lsl_line(lines.next())
                if not file_entry:
                    break

                if file_entry["is_dir"]:
                    continue

                filename = os.path.join(path, file_entry["file_name"])
                if filter_out_func(filename):
                    continue

                files_times[filename] = file_entry["mtime"]
    except StopIteration, e:
        pass
    return files_times

class AndroidPort(base.Port):
    """Android implementation of the Port class."""

    def __init__(self, **kwargs):
        kwargs.setdefault('port_name', 'android')
        base.Port.__init__(self, **kwargs)
        self._adb_path = None
        self._compare_path = None
        self._build_base_dir = None

    def _check_file_exists(self, path_to_file, file_description,
                           override_step=None, logging=True):
        """Verify the file is present where expected or log an error.

        Args:
            file_name: The (human friendly) name or description of the file
                you're looking for (e.g., "HTTP Server"). Used for error logging.
            override_step: An optional string to be logged if the check fails.
            logging: Whether or not log the error messages."""
        if not self._filesystem.exists(path_to_file):
            if logging:
                _log.error('Unable to find %s' % file_description)
                _log.error('    at %s' % path_to_file)
                if override_step:
                    _log.error('    %s' % override_step)
                    _log.error('')
            return False
        return True

    def default_child_processes(self):
        return 1

    def baseline_path(self):
        return self._webkit_baseline_path(self._name)

    def check_build(self, needs_http):
        result = self.check_android_env()

        if result:
            result = self._check_file_exists(self._dump_render_tree_package_path(), "DumpRenderTree package") and result

        if self.get_option('pixel_tests'):
            result = self.check_image_diff() and result

        # It's okay if pretty patch isn't available, but we will at
        # least log a message.
        self.check_pretty_patch()

        if needs_http:
            if self.get_option('use_apache'):
                result = self._check_apache_install() and result
            else:
                result = self._check_lighttpd_install() and result
        result = self._check_wdiff_install() and result

        if not result:
            _log.error('For complete Linux build requirements, please see:')
            _log.error('')
            _log.error('    apt-get install apache2 libapache2-mod-php5 imagemagick'
                       '')
        return result

    def check_sys_deps(self, needs_http):
        if not self.check_android_env():
            return False

        adb_path = self._path_to_adb()
        if not (adb_path and self._check_file_exists(adb_path, 'adb', None, True)):
            return False

        output = self._run_on_device(["echo", "ok"]).strip()

        if output != "ok":
            if logging:
                _log.error('Cannot execute adb shell. Check permissions and the device.')
            return False

        need_package_install = True
        result = self._file_mtime_on_device("/data/app/org.webkit.dumprendertree-1.apk")
        if result != None:
            need_package_install = result < os.stat(self._dump_render_tree_package_path()).st_mtime
            if need_package_install:
                self._run_adb(["uninstall", "org.webkit.dumprendertree"])
                time.sleep(2)

        if need_package_install:
            exit_code = self._run_adb(["install", self._dump_render_tree_package_path()],  return_exit_code=True)
            if exit_code != 0:
                if logging:
                    _log.error('Cannot install DumpRenderTree package. Check the build.')
                return False

        return True

    def check_image_diff(self, override_step=None, logging=True):
        compare_path = self._path_to_image_diff()
        result = compare_path and self._check_file_exists(compare_path, 'image diff exe (compare)', override_step, logging)
        return result

    def diff_image(self, expected_contents, actual_contents,
                   diff_filename=None):

        def compare_error_handler(error):
            _log.error("image diff failed:\n" + error.message_with_output())

        compare_exe = self._path_to_image_diff()

        tempdir = tempfile.mkdtemp()
        expected_filename = os.path.join(tempdir, "expected.png")
        with open(expected_filename, 'w+b') as file:
            file.write(expected_contents)
        actual_filename = os.path.join(tempdir, "actual.png")
        with open(actual_filename, 'w+b') as file:
            file.write(actual_contents)

        output_filename = "/dev/null"
        if diff_filename:
            output_filename = diff_filename

        cmd = [compare_exe, '-metric', 'RMSE', '-dissimilarity-threshold', '1', expected_filename, actual_filename, output_filename]

        are_different = True
        try:
            output = self._executive.run_command(cmd, error_handler=compare_error_handler)
            rmse = float(output.strip().split()[0])
            are_different = (rmse > 0)
        except OSError, e:
            if e.errno == errno.ENOENT or e.errno == errno.EACCES:
                _compare_available = False
            else:
                raise e
        finally:
            shutil.rmtree(tempdir, ignore_errors=True)
        return are_different

    def expected_checksum(self, test):
        result = super(AndroidPort, self).expected_checksum(test)
        if result is not None:
            return result

        png_path = self.expected_filename(test, '.png')

        if self.path_exists(png_path):
            with self._filesystem.open_binary_file_for_reading(png_path) as filehandle:
                return hashlib.md5(filehandle.read()).hexdigest()
        return None

    def path_to_test_expectations_file(self):
        return self.path_from_webkit_base('LayoutTests', 'platform',
            'android', 'test_expectations_nwrt.txt')

    def default_results_directory(self):
        return self._build_path('layout-test-results', 'android-hw')

    def create_driver(self, worker_number):
        """Starts a new Driver and returns a handle to it."""
        return AndroidDriver(self, worker_number)

    def test_base_platform_names(self):
        return ("android")

    def test_platform_name(self):
        return self._name + self.version()

    def test_platform_names(self):
        return self.test_base_platform_names() + ("android")

    def test_platform_name_to_name(self, test_platform_name):
        if test_platform_name in self.test_platform_names():
            return test_platform_name
        raise ValueError('Unsupported test_platform_name: %s' %
                         test_platform_name)

    def version(self):
        return ''


    def layout_tests_dir_on_device(self):
        return '/data/webkit/layout-tests'

    def filename_to_uri_for_test_command(self, filename):
        uri = self.filename_to_uri(filename)
        if uri.startswith("http://") or uri.startswith("https://"):
            return uri

        relative_path = self.relative_test_filename(filename)
        return self._filesystem.join(self.layout_tests_dir_on_device(), relative_path)


    def setup_test_run(self):
        self._run_on_device(['date', '-u', '%s' % int(time.mktime(time.gmtime()))])

        if self.get_option('no_sync'):
            _log.warning("Not pushing changed tests to device.")
        else:
            _log.info("Checking and pushing changed files to device (use --no-sync to disable).")
            self.sync_tests_to_device()

    def sync_tests_to_device(self):
        output = self._run_on_device(['ls', '-Rl', '%s' % self.layout_tests_dir_on_device()])

        def is_not_test_file(testname):
            return testname.endswith('-expected.txt') or testname.endswith('-expected.png') or testname.endswith('-expected.checksum') or testname.startswith('http/')

        files_on_device = parse_android_lsRl(self.layout_tests_dir_on_device(), output, is_not_test_file)

        tempdir = None
        num_files_to_be_updated = 0

        for root, dirs, files in os.walk(self.layout_tests_dir()):
            for filename in files:
                fullname = self._filesystem.join(root, filename)
                testname = self.relative_test_filename(fullname)
                if is_not_test_file(testname):
                    continue

                if testname in files_on_device:
                    host_mtime = os.stat(fullname).st_mtime
                    device_mtime = files_on_device[testname]
                    if host_mtime <= device_mtime:
                        continue

                # Copy the file to temp directory, to be pushed to device later.
                if not tempdir:
                    tempdir = tempfile.mkdtemp()

                tempname = self._filesystem.join(tempdir, testname)
                tempfulldir = self._filesystem.join(tempdir, os.path.dirname(tempname))
                if not os.path.exists(tempfulldir):
                    os.makedirs(tempfulldir)
                shutil.copy2(fullname, tempname)
                num_files_to_be_updated += 1

        if tempdir != None:
            self._run_adb(["shell", "mkdir", self.layout_tests_dir_on_device()])
            self._run_adb(["shell", "touch", "%s/.nomedia" % self.layout_tests_dir_on_device()])

            # Push the temp directory to device.
            _log.info('Pushing %s layout test files to device' % num_files_to_be_updated)
            self._push_file_to_device(tempdir, self.layout_tests_dir_on_device())
            shutil.rmtree(tempdir, ignore_errors=True)

    #
    # PROTECTED METHODS
    #
    # These routines should only be called by other methods in this file
    # or any subclasses.
    #
    def check_android_env(self):
        env_names = ['ANDROID_PRODUCT_OUT']
        for env_name in env_names:
            if env_name not in os.environ:
                _log.error('You need to define Android environment. '
                           'Missing environment variable ' + env_name)
                return False
        return True

    def _run_adb(self, cmd, **kwargs):
        cmd = [self._path_to_adb()] + cmd
        return self._executive.run_command(cmd, **kwargs)

    def _run_on_device(self, cmd, **kwargs):
        return self._run_adb(["shell"] + cmd, **kwargs)

    def _push_file_to_device(self, host_filename, device_filename):
        return self._run_adb(["push", host_filename, device_filename], return_exit_code=True);

    def _file_mtime_on_device(self, device_filename):
        """Return the mtime of a file on device if it exists or None if it does not.
        """
        output = self._run_on_device(['ls', '-l', device_filename , "2>/dev/null"])
        result = parse_android_lsl_line(output)
        if not result:
            return None

        return result["mtime"]

    def _android_baseline_path(self, platform):
        if platform is None:
            platform = self.name()
        return self.path_from_webkit_base('LayoutTests', 'platform', platform)

    def baseline_search_path(self):
        port_names = ["android", "mac"]
        return map(self._webkit_baseline_path, port_names)

    def _dump_render_tree_package_path(self):
        return self._build_path("data", "app", "org.webkit.dumprendertree.apk")

    def _path_to_image_diff(self):
        if not self._compare_path:
            self._compare_path = find_in_path("compare")
        return self._compare_path

    def _path_to_adb(self):
        if not self._adb_path:
            self._adb_path = find_in_path("adb")

        return self._adb_path

    def _build_path(self, *comps):
        if self._build_base_dir is None:
            self._build_base_dir = os.getenv('ANDROID_PRODUCT_OUT')
        return self._filesystem.join(self._build_base_dir, *comps)

    def _check_apache_install(self):
        result = self._check_file_exists(self._path_to_apache(),
            "apache2")
        result = self._check_file_exists(self._path_to_apache_config_file(),
            "apache2 config file") and result
        if not result:
            _log.error('    Please install using: "sudo apt-get install '
                       'apache2 libapache2-mod-php5"')
            _log.error('')
        return result

    def _check_lighttpd_install(self):
        result = self._check_file_exists(
            self._path_to_lighttpd(), "LigHTTPd executable")
        result = self._check_file_exists(self._path_to_lighttpd_php(),
            "PHP CGI executable") and result
        result = self._check_file_exists(self._path_to_lighttpd_modules(),
            "LigHTTPd modules") and result
        if not result:
            _log.error('    Please install using: "sudo apt-get install '
                       'lighttpd php5-cgi"')
            _log.error('')
        return result

    def _check_wdiff_install(self):
        result = self._check_file_exists(self._path_to_wdiff(), 'wdiff')
        if not result:
            _log.error('    Please install using: "sudo apt-get install '
                       'wdiff"')
            _log.error('')
        # FIXME: The AndroidMac port always returns True.
        return result

    def _path_to_apache(self):
        if self._is_redhat_based():
            return '/usr/sbin/httpd'
        else:
            return '/usr/sbin/apache2'

    def _path_to_apache_config_file(self):
        if self._is_redhat_based():
            config_name = 'fedora-httpd.conf'
        else:
            config_name = 'apache2-debian-httpd.conf'

        return os.path.join(self.layout_tests_dir(), 'http', 'conf',
                            config_name)

    def _path_to_lighttpd(self):
        return "/usr/sbin/lighttpd"

    def _path_to_lighttpd_modules(self):
        return "/usr/lib/lighttpd"

    def _path_to_lighttpd_php(self):
        return "/usr/bin/php-cgi"

    def _path_to_wdiff(self):
        if self._is_redhat_based():
            return '/usr/bin/dwdiff'
        else:
            return '/usr/bin/wdiff'

    def _is_redhat_based(self):
        return os.path.exists(os.path.join('/etc', 'redhat-release'))

    def _shut_down_http_server(self, server_pid):
        """Shut down the lighttpd web server. Blocks until it's fully
        shut down.

        Args:
            server_pid: The process ID of the running server.
        """
        # server_pid is not set when "http_server.py stop" is run manually.
        if server_pid is None:
            # TODO(mmoss) This isn't ideal, since it could conflict with
            # lighttpd processes not started by http_server.py,
            # but good enough for now.
            self._executive.kill_all("lighttpd")
            self._executive.kill_all("apache2")
        else:
            try:
                os.kill(server_pid, signal.SIGTERM)
                # TODO(mmoss) Maybe throw in a SIGKILL just to be sure?
            except OSError:
                # Sometimes we get a bad PID (e.g. from a stale httpd.pid
                # file), so if kill fails on the given PID, just try to
                # 'killall' web servers.
                self._shut_down_http_server(None)


class AdbServerProcess():
    def __init__(self, port_obj, name, cmd_line, executive, adb_port_out, adb_port_err):
        self._port = port_obj
        self._executive = executive
        self._adb_port_out = adb_port_out
        self._adb_port_err = adb_port_err
        self._name = name
        self._cmd_line = cmd_line
        self._reset()

    def _reset(self):
        self._socket_out = None
        self._socket_err = None
        self._output = ''
        self.crashed = False
        self.timed_out = False
        self.error = ''

    def _start(self):
        port_out_name = 'android-drt-%s' % self._adb_port_out
        port_err_name = 'android-drt-%s' % self._adb_port_err

        cmd = [self._port._path_to_adb(), 'forward',
               'local:%s' % port_out_name,
               'tcp:%s' % self._adb_port_out]
        exit_code = self._executive.run_command(cmd, return_exit_code=True)
        if exit_code != 0:
            raise RuntimeError("Failed to forward output socket")

        cmd = [self._port._path_to_adb(), 'forward',
               'local:%s' % port_err_name,
               'tcp:%s' % self._adb_port_err]
        exit_code = self._executive.run_command(cmd, return_exit_code=True)
        if exit_code != 0:
            raise RuntimeError("Failed to forward error socket")

        exit_code = self._executive.run_command(self._cmd_line, return_exit_code=True)
        if exit_code != 0:
            raise RuntimeError("Failed to start the service")

        # Need to sleep so that the Android service has time to start.
        # Otherwise the first test will timeout because nobody listens
        # to the socket when the test is written to it.
        time.sleep(2)

        self._socket_out = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self._socket_out.connect('/tmp/%s' % port_out_name)
        self._socket_out.settimeout(None)

        self._socket_err = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self._socket_err.connect('/tmp/%s' % port_err_name)
        self._socket_err.settimeout(None)

    def poll(self):
        #FIXME: this is fake.
        # Running _pid_of_service is too intensive.
        return None

    def _pid_of_service(self):
        output = self._port._run_on_device(["ps"])
        processes = output.splitlines()
        for proc in processes:
            fields = proc.split(None, 9)
            if len(fields) != 9:
                continue
            pid = fields[1]
            name = fields[8]
            if name.startswith('org.webkit.dumprendertree'):
                return int(pid)
        return None

    def write(self, input):
        if not self._socket_out:
            self._start()
        self._socket_out.send(input.encode('utf-8'))


    def _read(self, timeout, size):
        """Internal routine that actually does the read."""
        index = -1
        select_fds = (self._socket_out.fileno(), self._socket_err.fileno())
        deadline = time.time() + timeout
        while not self.timed_out and not self.crashed:
            if self.poll() != None:
                self.crashed = True

            now = time.time()
            if now > deadline:
                self.timed_out = True

            # Check to see if we have any output we can return.
            if size and len(self._output) >= size:
                index = size
            elif size == 0:
                index = self._output.find('\n') + 1

            if index > 0 or self.crashed or self.timed_out:
                output = self._output[0:index]
                self._output = self._output[index:]
                return output

            # Nope - wait for more data.
            (read_fds, write_fds, err_fds) = select.select(select_fds, [],
                                                           select_fds,
                                                           deadline - now)
            try:
                if self._socket_out.fileno() in read_fds:
                    self._output += self._socket_out.recv(4096)
            except IOError, e:
                pass

            try:
                if self._socket_err.fileno() in read_fds:
                    self.error += self._socket_err.recv(4096)
            except IOError, e:
                pass

    def stop(self):
        if self._socket_out:
            self._socket_out.close()
        if self._socket_err:
            self._socket_err.close()
        if self._pid_of_service() is None:
            return
        cmd = [self._port._path_to_adb(), 'shell', 'am', 'start',
               '-a', 'android.intent.action.SHUTDOWN',
               '-n', 'org.webkit.dumprendertree/.DumpRenderTreeActivity']
        exit_code = self._executive.run_command(cmd, return_exit_code=True)
        if exit_code != 0:
            raise RuntimeError("Failed to start the service")


        KILL_TIMEOUT = 3.0
        timeout = time.time() + KILL_TIMEOUT

        while self._pid_of_service() is not None and time.time() < timeout:
            time.sleep(0.1)

        pid = self._pid_of_service()
        if pid is not None:
            _log.warning('stopping %s timed out, killing it' % self._name)

            self._port._run_on_device(["kill", pid])

            _log.warning('killed')
        self._reset()

    def read(self, timeout, size):
        if size <= 0:
            raise ValueError('ServerProcess.read() called with a '
                             'non-positive size: %d ' % size)
        return self._read(timeout, size)

    def read_line(self, timeout):
        return self._read(timeout, size=0)


class AndroidDriver(webkit.WebKitDriver):
    """Driver for Android DumpRenderTree."""

    def __init__(self, port, worker_number, use_hw_pixel_tests = True):
        webkit.WebKitDriver.__init__(self, port, worker_number)
        self._adb_port_out = 50000 + worker_number
        self._adb_port_err = 60000 + worker_number
        self._use_hw_pixel_tests = use_hw_pixel_tests

    def start(self):
        self._server_process = AdbServerProcess(self._port, "DumpRenderTreeActivity", self.cmd_line(), self._port._executive, self._adb_port_out, self._adb_port_err)

    def cmd_line(self):
        return [self._port._path_to_adb(), 'shell', 'am', 'start',
                '-a', 'android.intent.action.VIEW',
                '--ei', 'outPort', '%s' % self._adb_port_out,
                '--ei', 'errPort', '%s' % self._adb_port_err,
                '--ez', 'useHWPixelTests', '%s' % self._use_hw_pixel_tests,
                '-n', 'org.webkit.dumprendertree/.DumpRenderTreeActivity']
