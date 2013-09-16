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

    scenarios = [('Restored Window', {'start_restored': True}),
                 ('Maximized Window', {'start_restored': False})]

    def setUp(self):
        super(WindowManagerKeybindings, self).setUp()
        self.start_test_window()

    def keybinding_if_not_minimized(self, keybinding):
        if not self.screen_win.minimized:
            self.keybinding(keybinding)

    def keybinding_if_not_restored(self, keybinding):
        if self.screen_win.vertically_maximized or self.screen_win.horizontally_maximized:
            self.keybinding(keybinding)

    def start_test_window(self, app_name="Character Map"):
        """Start a restored/maximized window of the requested application"""
        self.process_manager.close_all_app(app_name)
        self.bamf_win = self.process_manager.start_app_window(app_name)
        win_fn = lambda: self.unity.screen.window(self.bamf_win.x_id)
        self.assertThat(win_fn, Eventually(NotEquals(None)))
        self.screen_win = win_fn()

        if self.start_restored:
            if self.screen_win.vertically_maximized or self.screen_win.horizontally_maximized:
                self.addCleanup(self.keybinding_if_not_minimized, "window/maximize")
                self.keybinding("window/restore")
        else:
            if not self.screen_win.vertically_maximized and not self.screen_win.horizontally_maximized:
                self.addCleanup(self.keybinding_if_not_restored, "window/restore")
                self.keybinding("window/maximize")

    def get_window_workarea(self):
        monitor = self.bamf_win.monitor
        monitor_geo = self.display.get_screen_geometry(monitor)
        launcher = self.unity.launcher.get_launcher_for_monitor(monitor)
        launcher_w = 0 if launcher.hidemode else launcher.geometry[2]
        panel_h = self.unity.panels.get_panel_for_monitor(monitor).geometry[3]
        return (monitor_geo[0] + launcher_w, monitor_geo[1] + panel_h,
                monitor_geo[2] - launcher_w, monitor_geo[3] - panel_h)

    def test_maximize_window(self):
        if self.start_restored:
            self.addCleanup(self.keybinding, "window/restore")
        self.keybinding("window/maximize")
        self.assertThat(self.screen_win.maximized, Eventually(Equals(True)))

    def test_restore_maximized_window(self):
        if self.start_restored:
            self.keybinding("window/maximize")
        self.keybinding("window/restore")
        self.assertThat(self.screen_win.maximized, Eventually(Equals(False)))
        self.assertThat(self.screen_win.minimized, Eventually(Equals(False)))

    def test_restore_vertically_maximized_window(self):
        if not self.start_restored:
            self.addCleanup(self.keybinding, "window/maximize")
            self.keybinding("window/restore")
        self.keyboard.press_and_release("Ctrl+Super+Right")
        self.keybinding("window/restore")
        self.assertThat(self.screen_win.vertically_maximized, Eventually(Equals(False)))
        self.assertThat(self.screen_win.minimized, Eventually(Equals(False)))

    def test_minimize_restored_window(self):
        if not self.start_restored:
            self.addCleanup(self.keybinding_if_not_minimized, "window/maximize")
            self.keybinding("window/restore")
        self.keybinding("window/restore")
        self.assertThat(self.screen_win.minimized, Eventually(Equals(True)))

    def test_left_maximize(self):
        self.addCleanup(self.keybinding, "window/restore" if self.start_restored else "window/maximize")
        self.keyboard.press_and_release("Ctrl+Super+Left")
        self.assertThat(self.screen_win.vertically_maximized, Eventually(Equals(True)))
        self.assertThat(self.screen_win.horizontally_maximized, Eventually(Equals(False)))

        workarea_geo = self.get_window_workarea()
        self.assertThat(self.screen_win.x, Eventually(Equals(workarea_geo[0])))
        self.assertThat(self.screen_win.y, Eventually(Equals(workarea_geo[1])))
        self.assertThat(self.screen_win.width, Eventually(Equals(workarea_geo[2]/2)))
        self.assertThat(self.screen_win.height, Eventually(Equals(workarea_geo[3])))

    def test_right_maximize(self):
        self.addCleanup(self.keybinding, "window/restore" if self.start_restored else "window/maximize")
        self.keyboard.press_and_release("Ctrl+Super+Right")
        self.assertThat(self.screen_win.vertically_maximized, Eventually(Equals(True)))
        self.assertThat(self.screen_win.horizontally_maximized, Eventually(Equals(False)))

        workarea_geo = self.get_window_workarea()
        self.assertThat(self.screen_win.x, Eventually(Equals(workarea_geo[0]+workarea_geo[2]/2)))
        self.assertThat(self.screen_win.y, Eventually(Equals(workarea_geo[1])))
        self.assertThat(self.screen_win.width, Eventually(Equals(workarea_geo[2]/2)))
        self.assertThat(self.screen_win.height, Eventually(Equals(workarea_geo[3])))
