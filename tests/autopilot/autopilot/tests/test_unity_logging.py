# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.unity import (
    start_log_to_file,
    stop_logging_to_file,
    set_log_severity,
    log_unity_message,
    )

from os import remove
from os.path import exists
from tempfile import mktemp
from time import sleep


class UnityLoggingTests(AutopilotTestCase):
    """Tests for Unity's debug logging framework."""

    def test_new_file_created(self):
        """Unity must create log file when we call start_log_to_file.
        """
        fpath = mktemp()
        start_log_to_file(fpath)
        self.addCleanup(remove, fpath)
        self.addCleanup(stop_logging_to_file)
        sleep(1)
        self.assertTrue(exists(fpath))


