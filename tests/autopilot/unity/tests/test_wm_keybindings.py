# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2013 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import Equals, NotEquals, GreaterThan
from unity.tests import UnityTestCase
from unity.emulators import switcher

class WindowManagerKeybindings(UnityTestCase):
    """Window Manager keybindings tests"""

    def setUp(self):
        super(WindowManagerKeybindings, self).setUp()

    def open_panel_menu(self):
        panel = self.unity.panels.get_panel_for_monitor(0)
        self.assertThat(lambda: len(panel.menus.get_entries()), Eventually(GreaterThan(0)))
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.keybinding("panel/open_first_menu")
        self.assertThat(self.unity.panels.get_active_indicator, Eventually(NotEquals(None)))

    def test_dash_shows_on_menus_opened(self):
        self.process_manager.start_app_window("Calculator")
        self.open_panel_menu()
        self.addCleanup(self.unity.dash.ensure_hidden)
        self.unity.dash.ensure_visible()
        self.assertThat(self.unity.panels.get_active_indicator, Eventually(Equals(None)))

    def test_hud_shows_on_menus_opened(self):
        self.process_manager.start_app_window("Calculator")
        self.open_panel_menu()
        self.addCleanup(self.unity.hud.ensure_hidden)
        self.unity.hud.ensure_visible()
        self.assertThat(self.unity.panels.get_active_indicator, Eventually(Equals(None)))

    def test_switcher_shows_on_menus_opened(self):
        self.process_manager.start_app_window("Calculator")
        self.open_panel_menu()
        self.addCleanup(self.unity.switcher.terminate)
        self.unity.switcher.initiate()
        self.assertProperty(self.unity.switcher, mode=switcher.SwitcherMode.NORMAL)
        self.assertThat(self.unity.panels.get_active_indicator, Eventually(Equals(None)))

    def test_shortcut_hints_shows_on_menus_opened(self):
        self.process_manager.start_app_window("Calculator")
        self.open_panel_menu()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)
        self.unity.shortcut_hint.show()
        self.assertThat(self.unity.shortcut_hint.visible, Eventually(Equals(True)))
        self.assertThat(self.unity.panels.get_active_indicator, Eventually(Equals(None)))

    def test_spread_shows_on_menus_opened(self):
        self.process_manager.start_app_window("Calculator")
        self.open_panel_menu()
        self.addCleanup(self.unity.window_manager.terminate_spread)
        self.unity.window_manager.initiate_spread()
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))
        self.assertThat(self.unity.panels.get_active_indicator, Eventually(Equals(None)))


class WindowManagerKeybindingsForWindowHandling(UnityTestCase):
    """Window Manager keybindings tests for handling a window"""

    scenarios = [('Restored Window', {'start_restored': True}),
                 ('Maximized Window', {'start_restored': False})]

    def setUp(self):
        super(WindowManagerKeybindingsForWindowHandling, self).setUp()
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
        (x1, y1, w1, h1) = self.screen_win.geometry
        self.keyboard.press_and_release("Ctrl+Super+Right")
        self.keybinding("window/restore")
        (x2, y2, w2, h2) = self.screen_win.geometry

        self.assertThat(self.screen_win.vertically_maximized, Eventually(Equals(False)))
        self.assertThat(self.screen_win.minimized, Eventually(Equals(False)))
        # Make sure the window restored to its same geometry before vertically maximizing
        self.assertThat(x1, Equals(x2))
        self.assertThat(y1, Equals(y2))
        self.assertThat(w1, Equals(w2))
        self.assertThat(h1, Equals(h2))

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
