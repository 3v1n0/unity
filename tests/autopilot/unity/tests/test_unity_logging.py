# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from os import remove
from os.path import exists
from tempfile import mktemp
from testtools.matchers import Contains, Not
from time import sleep


from unity.emulators.unity import (
    start_log_to_file,
    reset_logging,
    set_log_severity,
    log_unity_message,
    )
from unity.tests import UnityTestCase


class UnityLoggingTests(UnityTestCase):
    """Tests for Unity's debug logging framework."""

    def start_new_log_file(self):
        fpath = mktemp()
        start_log_to_file(fpath)
        return fpath

    def test_new_file_created(self):
        """Unity must create log file when we call start_log_to_file.
        """
        fpath = self.start_new_log_file()
        self.addCleanup(remove, fpath)
        self.addCleanup(reset_logging)
        sleep(1)
        self.assertTrue(exists(fpath))

    def test_messages_arrive_in_file(self):
        fpath = self.start_new_log_file()
        log_unity_message("WARNING", "This is a warning of things to come")
        sleep(1)
        reset_logging()

        with open(fpath, 'r') as f:
            self.assertThat(f.read(), Contains("This is a warning of things to come"))

    def test_default_log_level_unchanged(self):
        fpath = self.start_new_log_file()
        log_unity_message("DEBUG", "This is some INFORMATION")
        sleep(1)
        reset_logging()
        with open(fpath, 'r') as f:
            self.assertThat(f.read(), Not(Contains("This is some INFORMATION")))

    def test_can_change_log_level(self):
        fpath = self.start_new_log_file()
        set_log_severity("", "DEBUG")
        self.addCleanup(set_log_severity, "", "INFO")
        log_unity_message("DEBUG", "This is some more INFORMATION")
        sleep(1)
        reset_logging()
        with open(fpath, 'r') as f:
            self.assertThat(f.read(), Contains("This is some more INFORMATION"))
