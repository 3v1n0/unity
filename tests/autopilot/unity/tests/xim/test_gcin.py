# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Brandon Schaefer
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from os import environ
import subprocess
from testtools.matchers import Equals
from testtools import skip
from unity.tests import UnityTestCase


class GcinTestCase(UnityTestCase):
    """Tests the Input Method gcin."""

    @skip("Currenlty no XIM support in Nux")
    def setUp(self):
        super(GcinTestCase, self).setUp()

        # Check that gcin is set as the active IM through im-switch
        if environ.get('XMODIFIERS', '') != "@im=gcin":
            self.skip("Please make sure XMODIFIERS is set to @im=gcin. Set it using 'im-switch'.")

        running_process = subprocess.check_output('ps -e', shell=True)
        if 'gcin' not in running_process:
            self.skip("gcin is not an active process, please start 'gcin' before running these tests.")

        if 'ibus' in running_process:
            self.skip("IBus is currently running, please close 'ibus-daemon' before running these tests.")


class GcinTestHangul(GcinTestCase):
    """Tests the Dash and Hud with gcin in hangul mode."""

    scenarios = [
            ('hangul', {'input': 'han geul ', 'result': u'\ud55c\uae00'}),
            ('morning letters', {'input': 'a chimgeul ', 'result': u'\uc544\uce68\uae00'}),
            ('national script', {'input': 'gug mun ', 'result': u'\uad6d\ubb38'}),
        ]

    @skip("Currenlty no XIM support in Nux")
    def setUp(self):
        super(GcinTestHangul, self).setUp()

    @skip("Currenlty no XIM support in Nux")
    def enter_hangul_mode(self):
        """Ctrl+Space turns gcin on, Ctrl+Alt+/ turns hangul on."""
        self.keyboard.press_and_release("Ctrl+Space")
        self.keyboard.press_and_release("Ctrl+Alt+/")

    @skip("Currenlty no XIM support in Nux")
    def test_dash_input(self):
        """Entering an input string through gcin will result in a Korean string result in the dash."""

        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        self.enter_hangul_mode()

        self.keyboard.type(self.input)
        self.assertThat(self.dash.search_string, Eventually(Equals(self.result)))

    @skip("Currenlty no XIM support in Nux")
    def test_hud_input(self):
        """Entering an input string through gcin will result in a Korean string result in the hud."""

        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        self.enter_hangul_mode()

        self.keyboard.type(self.input)
        self.assertThat(self.hud.search_string, Eventually(Equals(self.result)))
