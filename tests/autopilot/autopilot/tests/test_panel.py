# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import logging
import os
from testtools.matchers import Equals, NotEquals, LessThan, GreaterThan
from time import sleep

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.bamf import BamfWindow
from autopilot.emulators.X11 import ScreenGeometry

logger = logging.getLogger(__name__)

def _make_monitor_scenarios():
    num_monitors = ScreenGeometry().get_num_monitors()
    scenarios = []

    if num_monitors == 1:
        scenarios = [('Single Monitor', {'panel_monitor': 0})]
    else:
        for i in range(num_monitors):
            scenarios += [('Monitor %d' % (i), {'panel_monitor': i})]

    return scenarios


class PanelTestsBase(AutopilotTestCase):

    panel_monitor = 0

    def setUp(self):
        super(PanelTestsBase, self).setUp()

    def move_window_to_monitor(self, window, monitor):
        if not isinstance(window, BamfWindow):
            raise TypeError("Window must be a BamfWindow")
        (win_x, win_y, win_w, win_h) = window.geometry
        (t_x, t_y, t_w, t_h) = self.screen_geo.get_monitor_geometry(monitor)

        logger.debug("Dragging window to monitor %r." % monitor)

        self.mouse.move(win_x + win_w/2, win_y + win_h/2)
        self.keyboard.press("Alt")
        self.mouse.press()
        self.keyboard.release("Alt")

        # We do the movements in two steps, to reduce the risk of being
        # blocked by the pointer barrier
        self.mouse.move(win_x, t_y + t_h/2, rate=20, time_between_events=0.005)
        self.mouse.move(t_x + t_w/2, t_y + t_h/2, rate=20, time_between_events=0.005)
        self.mouse.release()
        sleep(.25)
        self.assertThat(window.monitor, Equals(monitor))


class PanelTitleTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()
    #panel_monitor = 1

    def setUp(self):
        super(PanelTitleTests, self).setUp()
        self.screen_geo.move_mouse_to_monitor(self.panel_monitor)
        self.panel = self.panels.get_panel_for_monitor(self.panel_monitor)

    def test_panel_title_on_empty_desktop(self):
        sleep(.5)
        self.keybinding("window/show_desktop")
        self.addCleanup(self.keybinding, "window/show_desktop")
        sleep(1)

        self.assertTrue(self.panel.desktop_is_active)

    def test_panel_title_with_restored_application(self):
        """Tests the title shown in the panel with a restored application"""
        self.close_all_app("Calculator")
        calc = self.start_app("Calculator")
        self.assertTrue(calc.is_active)

        wins = calc.get_windows()
        self.assertThat(len(wins), Equals(1))
        calc_win = wins[0]
        self.assertTrue(calc_win.is_focused)

        if calc_win.monitor != self.panel_monitor:
            self.addCleanup(self.move_window_to_monitor, calc_win, calc_win.monitor)
            self.move_window_to_monitor(calc_win, self.panel_monitor)

        self.assertFalse(calc_win.is_maximized)
        self.assertThat(self.panel.title, Equals(calc.name))

    def test_panel_title_with_maximized_application(self):
        """Tests the title shown in the panel with a maximized application"""
        self.close_all_app("Text Editor")
        text = self.start_app("Text Editor")
        self.assertTrue(text.is_active)

        wins = text.get_windows()
        self.assertThat(len(wins), Equals(1))
        text_win = wins[0]
        self.assertTrue(text_win.is_focused)

        if text_win.monitor != self.panel_monitor:
            if text_win.is_maximized:
                self.addCleanup(self.keybinding, "window/maximize")
                self.keybinding("window/restore")
                sleep(.1)

            self.addCleanup(self.move_window_to_monitor, text_win, text_win.monitor)
            self.move_window_to_monitor(text_win, self.panel_monitor)

        self.keybinding("window/maximize")
        self.addCleanup(self.keybinding, "window/restore")

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))

    def test_panel_title_with_maximized_window_restored_child(self):
        """Tests the title shown in the panel with a maximized application"""
        self.close_all_app("Text Editor")
        text = self.start_app("Text Editor")
        self.assertTrue(text.is_active)

        wins = text.get_windows()
        self.assertThat(len(wins), Equals(1))
        text_win = wins[0]
        self.assertTrue(text_win.is_focused)

        if text_win.monitor != self.panel_monitor:
            if text_win.is_maximized:
                self.addCleanup(self.keybinding, "window/maximize")
                self.keybinding("window/restore")
                sleep(.1)

            self.addCleanup(self.move_window_to_monitor, text_win, text_win.monitor)
            self.move_window_to_monitor(text_win, self.panel_monitor)

        self.keybinding("window/maximize")
        self.addCleanup(self.keybinding, "window/restore")

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))

        self.keyboard.press_and_release("Ctrl+S")
        sleep(.25)
        self.assertThat(self.panel.title, Equals(text.name))
        self.keyboard.press_and_release("Escape")

    def test_panel_title_switching_active_window(self):
        """Tests the title shown in the panel with a maximized application"""
        self.set_unity_option('num_launchers', 0)
        self.close_all_app("Text Editor")
        text = self.start_app("Text Editor")
        self.assertTrue(text.is_active)

        wins = text.get_windows()
        self.assertThat(len(wins), Equals(1))
        text_win = wins[0]
        self.assertTrue(text_win.is_focused)

        if text_win.monitor != self.panel_monitor:
            if text_win.is_maximized:
                self.addCleanup(self.keybinding, "window/maximize")
                self.keybinding("window/restore")
                sleep(.1)

            self.addCleanup(self.move_window_to_monitor, text_win, text_win.monitor)
            self.move_window_to_monitor(text_win, self.panel_monitor)

        self.keybinding("window/maximize")
        self.addCleanup(self.keybinding, "window/restore")

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))
        sleep(.25)

        self.close_all_app("Calculator")
        calc = self.start_app("Calculator")
        self.assertTrue(calc.is_active)

        wins = calc.get_windows()
        self.assertThat(len(wins), Equals(1))
        calc_win = wins[0]
        self.assertTrue(calc_win.is_focused)

        if calc_win.monitor != self.panel_monitor:
            self.addCleanup(self.move_window_to_monitor, calc_win, calc_win.monitor)
            self.move_window_to_monitor(calc_win, self.panel_monitor)

        sleep(.25)
        self.assertThat(self.panel.title, Equals(calc.name))

        icon = self.launcher.model.get_icon_by_desktop_id(text.desktop_file)
        launcher = self.launcher.get_launcher_for_monitor(self.panel_monitor)
        launcher.click_launcher_icon(icon)

        self.assertTrue(text_win.is_focused)
        self.assertThat(self.panel.title, Equals(text_win.title))


class PanelTitleCrossMonitors(PanelTestsBase):

    scenarios = []
    if ScreenGeometry().get_num_monitors() > 1:
        scenarios = [("Multimonitor", {})]

    def setUp(self):
        super(PanelTitleCrossMonitors, self).setUp()
        self.screen_geo.move_mouse_to_monitor(self.panel_monitor)
        self.panel = self.panels.get_panel_for_monitor(self.panel_monitor)

    def test_panel_title_updates_moving_window(self):
        """Tests the title shown in the panel, moving a restored window around them"""
        self.close_all_app("Calculator")
        calc = self.start_app("Calculator")
        self.assertTrue(calc.is_active)

        wins = calc.get_windows()
        self.assertThat(len(wins), Equals(1))
        calc_win = wins[0]
        self.assertTrue(calc_win.is_focused)

        prev_monitor = -1
        for monitor in range(0, self.screen_geo.get_num_monitors()):
            if calc_win.monitor != monitor:
                self.addCleanup(self.move_window_to_monitor, calc_win, calc_win.monitor)
                self.move_window_to_monitor(calc_win, monitor)

            if prev_monitor >= 0:
                self.assertFalse(self.panels.get_panel_for_monitor(prev_monitor).active)

            panel = self.panels.get_panel_for_monitor(monitor)
            self.assertTrue(panel.active)
            self.assertThat(panel.title, Equals(calc.name))

            prev_monitor = monitor

