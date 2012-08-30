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
from testtools.matchers import Equals

from unity.tests import UnityTestCase

class GcinTestCase(UnityTestCase):
    def setUp(self):
        super(GcinTestCase, self).setUp()
        if environ['XMODIFIERS'] != "@im=gcin":
            raise EnvironmentError("Please make sure XMODIFIERS is set to @im=gcin. Set it using 'im-switch'.")

class GcinTestHangul(GcinTestCase):
    scenarios = [
            ('hangul', {'input': 'han geul ', 'result': u'\ud55c\uae00'}),
            ('morning letters', {'input': 'a chimgeul ', 'result': u'\uc544\uce68\uae00'}),
            ('national script', {'input': 'gug mun ', 'result': u'\uad6d\ubb38'}),
        ]

    def setUp(self):
        super(GcinTestHangul, self).setUp()

    def enter_hangul_mode(self):
        self.keyboard.press_and_release("Ctrl+Space")
        self.keyboard.press_and_release("Ctrl+Alt+/")

    def test_dash_input(self):
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        self.enter_hangul_mode()

        self.keyboard.type(self.input)
        self.assertThat(self.dash.search_string, Eventually(Equals(self.result)))

    def test_hud_input(self):
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        self.enter_hangul_mode()

        self.keyboard.type(self.input)
        self.assertThat(self.hud.search_string, Eventually(Equals(self.result)))
