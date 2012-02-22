# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards, Martin Mrazik
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"Tests to ensure unity is compatible with ibus input method."

from autopilot.emulators.ibus import (
    set_active_engines,
    get_available_input_engines,
    )
from autopilot.emulators.unity.dash import Dash
from autopilot.emulators.X11 import Keyboard
from autopilot.tests import AutopilotTestCase

from time import sleep

class IBusTests(AutopilotTestCase):
    """Base class for IBus tests."""

    def setUp(self):
        super(IBusTests, self).setUp()
        self.kb = Keyboard()
        self.dash = Dash()

    def tearDown(self):
        if self._old_engines:
            set_active_engines(self._old_engines)
        super(IBusTests, self).tearDown()

    def activate_input_engine_or_skip(self, engine_name):
        available_engines = get_available_input_engines()
        if engine_name in available_engines:
            self._old_engines = set_active_engines([engine_name])
        else:
            self.skipTest("This test requires the '%s' engine to be installed." % (engine_name))

    def activate_ibus(self):
        # it would be nice to be able to tell if it's currently active or not.
        self.kb.press_and_release('Ctrl+Space')

    def deactivate_ibus(self):
        # it would be nice to be able to tell if it's currently active or not.
        self.kb.press_and_release('Ctrl+Space')


class IBusTestsPinyin(IBusTests):
    """Tests for the Pinyin input engine."""

    scenarios = [
        ('basic', {'input': 'abc1', 'result': u'\u963f\u5e03\u4ece'}),
        ('photo', {'input': 'zhaopian ', 'result': u'照片'}),
        ('internet', {'input': 'hulianwang ', 'result': u'互联网'}),
        ('disk', {'input': 'cipan ', 'result': u'磁盘'}),
        ('disk_management', {'input': 'cipan guanli ', 'result': u'磁盘管理'}),
    ]

    def test_pinyin(self):
        self.activate_input_engine("pinyin")
        self.dash.ensure_visible()
        sleep(0.5)
        self.activate_ibus()
        sleep(0.5)
        self.kb.type(self.input)
        dash_search_string = self.dash.get_search_string()
        self.deactivate_ibus()
        self.dash.ensure_hidden()

        self.assertEqual(self.result, dash_search_string)

