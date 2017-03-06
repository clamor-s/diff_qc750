#!/usr/bin/env python
# Copyright (C) 2010 Google Inc. All rights reserved.
# Copyright (C) 2012 NVIDIA. All rights reserved.
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

"""Android software pixel tests implementation of the Port interface."""

import logging

import android

_log = logging.getLogger("webkitpy.layout_tests.port.android_sw")


class AndroidSWPort(android.AndroidPort):
    """Android Linux implementation of the Port class, with software pixel tests."""

    def __init__(self, **kwargs):
        kwargs.setdefault('port_name', 'android-sw')
        android.AndroidPort.__init__(self, **kwargs)

    def baseline_search_path(self):
        port_names = ["android-sw"]
        return map(self._webkit_baseline_path, port_names) + android.AndroidPort.baseline_search_path(self)

    def create_driver(self, worker_number):
        """Starts a new Driver and returns a handle to it."""
        return android.AndroidDriver(self, worker_number, False)

    def check_build(self, needs_http):
        if self.get_option('pixel_tests'):
            _log.info("pixel tests are using non-accelerated compositing mode")
        return android.AndroidPort.check_build(self, needs_http)

    def path_to_test_expectations_file(self):
        return self.path_from_webkit_base('LayoutTests', 'platform',
            'android-sw', 'test_expectations_nwrt.txt')

    def default_results_directory(self):
        return self._build_path('layout-test-results', 'android-sw')
