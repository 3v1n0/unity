# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools import TestCase
from testtools.matchers import raises

from autopilot.utilities import get_keystroke_string_from_compiz_setting

class CompizConfigKeyTranslateTests(TestCase):
    """Tests that the compizconfig keycode translation routes work as advertised."""

    def test_requires_setting_instance(self):
        """Function must raise TypeError unless given an instance of compizconfig.Setting."""
        self.assertThat(lambda: get_keystroke_string_from_compiz_setting(True), raises(TypeError))
