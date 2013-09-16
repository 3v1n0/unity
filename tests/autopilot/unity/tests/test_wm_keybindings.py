# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2013 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import Equals, NotEquals
from unity.tests import UnityTestCase

class WindowManagerKeybindings(UnityTestCase):
    """Window Manager keybindings tests"""

    def setUp(self):
        super(WindowManagerKeybindings, self).setUp()
        self.start_test_window()

    def keybinding_if_not_minimized(self, keybinding):
        if not self.screen_win.minimized:
            self.keybinding(keybinding)

    def start_test_window(self, app_name="Character Map"):
        """Start a restored window of the requested application"""
        self.process_manager.close_all_app(app_name)
        self.bamf_win = self.process_manager.start_app_window(app_name)
        win_fn = lambda: self.unity.screen.window(self.bamf_win.x_id)
        self.assertThat(win_fn, Eventually(NotEquals(None)))
        self.screen_win = win_fn()
        if self.screen_win.vertically_maximized or self.screen_win.horizontally_maximized:
            self.addCleanup(self.keybinding_if_not_minimized, "window/maximize")
            self.keybinding("window/restore")

    def test_maximize_window(self):
        self.addCleanup(self.keybinding, "window/restore")
        self.keybinding("window/maximize")
        self.assertThat(self.screen_win.maximized, Eventually(Equals(True)))

    def test_restore_window(self):
        self.keybinding("window/restore")
        self.assertThat(self.screen_win.maximized, Eventually(Equals(False)))
        self.assertThat(self.screen_win.minimized, Eventually(Equals(False)))

    def test_minimize_window(self):
        self.keybinding("window/restore")
        self.assertThat(self.screen_win.minimized, Eventually(Equals(True)))

    def test_left_maximize(self):
        self.addCleanup(self.keybinding, "window/restore")
        self.keyboard.press_and_release("Ctrl+Super+Left")
        self.assertThat(self.screen_win.vertically_maximized, Eventually(Equals(True)))
        self.assertThat(self.screen_win.horizontally_maximized, Eventually(Equals(False)))

