# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards, Martin Mrazik
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"Tests to ensure unity is compatible with ibus input method."

from autopilot.emulators.ibus import set_active_engines
from autopilot.emulators.unity.dash import Dash
from autopilot.emulators.X11 import Keyboard
from autopilot.tests import AutopilotTestCase
from time import sleep

class IBusTests(AutopilotTestCase):

    def setUp(self):
        super(IBusTests, self).setUp()
        self.old_engines = set_active_engines(["pinyin"])
        self.kb = Keyboard()
        self.kb.press_and_release('Ctrl+Space')
        self.dash = Dash()

    def tearDown(self):
        set_active_engines(self.old_engines)
        super(IBusTests, self).tearDown()

    def activate_ibus(self):
        # it would be nice to be able to tell if it's currently active or not.
        self.kb.press_and_release('Ctrl+Space')

    def deactivate_ibus(self):
        # it would be nice to be able to tell if it's currently active or not.
        self.kb.press_and_release('Ctrl+Space')

    def test_simple_dash(self):
        """Entering 'abc1' in the dash with ibus enabled should result in u'\u963f\u5e03\u4ece'."""
        self.dash.ensure_visible()
        self.activate_ibus()
        self.kb.type('abc1')
        dash_search_string = self.dash.get_search_string()
        self.deactivate_ibus()
        self.dash.ensure_hidden()

        self.assertEqual(u'\u963f\u5e03\u4ece', dash_search_string)

    def test_photo(self):
        """Enter 照片 (Photo) to dash and check the search result"""
        self.dash.ensure_visible()
        self.activate_ibus()
        self.kb.type("zhaopian ")
        self.deactivate_ibus()
        self.assertEqual(self.dash.get_search_string(), u'照片')
        self.dash.ensure_hidden()

    def test_internet(self):
        """Enter 互联网 (internet) to dash and check the search result"""
        self.dash.ensure_visible()
        self.activate_ibus()
        self.kb.type("hulianwang ")
        self.deactivate_ibus()
        self.assertEqual(self.dash.get_search_string(), u'互联网')
        self.dash.ensure_hidden()

    def test_disk(self):
        """Enter 磁盘 (disk) to dash and check the search result"""
        self.dash.ensure_visible()
        self.activate_ibus()
        self.kb.type("cipan ")
        self.deactivate_ibus()
        self.assertEqual(self.dash.get_search_string(), u'磁盘')
        self.dash.ensure_hidden()

    def test_disk_management(self):
        """Enter 磁盘管理 (disk management) to dash and check the search result"""
        self.dash.ensure_visible()
        self.activate_ibus()
        self.kb.type(u"cipan guanli ")
        self.deactivate_ibus()
        self.assertEqual(self.dash.get_search_string(), u'磁盘管理')
        self.dash.ensure_hidden()

