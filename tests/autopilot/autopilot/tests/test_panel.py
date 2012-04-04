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

    def open_new_application_window(self, app_name, maximize=False):
        """Opens a new instance of the requested application, ensuring
        that only one window is opened.

        Returns the opened BamfWindow
        
        """
        self.close_all_app(app_name)
        app = self.start_app(app_name)

        wins = app.get_windows()
        self.assertThat(len(wins), Equals(1))
        app_win = wins[0]

        app_win.set_focus()
        self.assertTrue(app.is_active)
        self.assertTrue(app_win.is_focused)
        self.assertThat(app.desktop_file, Equals(app_win.application.desktop_file))

        if app_win.monitor != self.panel_monitor:
            if app_win.is_maximized:
                self.addCleanup(self.keybinding, "window/maximize")
                self.keybinding("window/restore")
                sleep(.1)

            self.addCleanup(self.screen_geo.drag_window_to_monitor, app_win, app_win.monitor)
            self.screen_geo.drag_window_to_monitor(app_win, self.panel_monitor)
            sleep(.25)
            self.assertThat(app_win.monitor, Equals(self.panel_monitor))

        if maximize:
            self.keybinding("window/maximize")
            self.addCleanup(self.keybinding, "window/restore")

        app_win.set_focus()

        self.assertThat(app_win.is_maximized, Equals(maximize))

        return app_win

class PanelTitleTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()
    #panel_monitor = 0

    def setUp(self):
        super(PanelTitleTests, self).setUp()
        self.screen_geo.move_mouse_to_monitor(self.panel_monitor)
        self.panel = self.panels.get_panel_for_monitor(self.panel_monitor)

    def test_panel_title_on_empty_desktop(self):
        sleep(1)
        self.keybinding("window/show_desktop")
        self.addCleanup(self.keybinding, "window/show_desktop")
        sleep(1)

        self.assertTrue(self.panel.desktop_is_active)

    def test_panel_title_with_restored_application(self):
        """Tests the title shown in the panel with a restored application"""
        calc_win = self.open_new_application_window("Calculator")

        self.assertFalse(calc_win.is_maximized)
        self.assertThat(self.panel.title, Equals(calc_win.application.name))

    def test_panel_title_with_maximized_application(self):
        """Tests the title shown in the panel with a maximized application"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))

    def test_panel_title_with_maximized_window_restored_child(self):
        """Tests the title shown in the panel with a maximized application"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))

        self.keyboard.press_and_release("Ctrl+S")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(.25)
        self.assertThat(self.panel.title, Equals(text_win.application.name))

    def test_panel_title_switching_active_window(self):
        """Tests the title shown in the panel with a maximized application"""
        # Locked Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', 0)

        text_win = self.open_new_application_window("Text Editor", maximize=True)

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))
        sleep(.25)

        calc_win = self.open_new_application_window("Calculator")
        self.assertThat(self.panel.title, Equals(calc_win.application.name))

        icon = self.launcher.model.get_icon_by_desktop_id(text_win.application.desktop_file)
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
        calc_win = self.open_new_application_window("Calculator")

        prev_monitor = -1
        for monitor in range(0, self.screen_geo.get_num_monitors()):
            if calc_win.monitor != monitor:
                self.addCleanup(self.screen_geo.drag_window_to_monitor, calc_win, calc_win.monitor)
                self.screen_geo.drag_window_to_monitor(calc_win, monitor)

            if prev_monitor >= 0:
                self.assertFalse(self.panels.get_panel_for_monitor(prev_monitor).active)

            panel = self.panels.get_panel_for_monitor(monitor)
            self.assertTrue(panel.active)
            self.assertThat(panel.title, Equals(calc_win.application.name))

            prev_monitor = monitor

