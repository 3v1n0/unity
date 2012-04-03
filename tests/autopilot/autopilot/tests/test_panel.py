# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import logging
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

class PanelTitleTests(AutopilotTestCase):

    scenarios = _make_monitor_scenarios()

    def setUp(self):
        super(PanelTitleTests, self).setUp()
        #self.active_panel = self.panel.get_active_panel()
        self.screen_geo.move_mouse_to_monitor(self.panel_monitor)
        self.target_panel = self.panel.get_panel_for_monitor(self.panel_monitor)

    #FIXME, remove it as soon as the better implementation goes in trunk
    def get_bamf_application(self, name):
        apps = self.get_app_instances(name)
        self.assertThat(len(apps), Equals(1))
        return apps[0]

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

        self.mouse.move(win_x, t_y + t_h/2)
        self.mouse.move(t_x + t_w/2, t_y + t_h/2)
        self.mouse.release()
        sleep(.25)
        self.assertThat(window.monitor, Equals(monitor))

    def test_panel_title_on_empty_desktop(self):
        sleep(.5)
        self.keybinding("window/show_desktop")
        self.addCleanup(self.keybinding, "window/show_desktop")
        sleep(1)

        self.assertTrue(self.target_panel.desktop_is_active)

    def test_panel_title_with_restored_application(self):
        """Tests the title shown in the panel with a restored application"""
        self.close_all_app("Calculator")
        self.start_app("Calculator")
        sleep(2)
        calc = self.get_bamf_application("Calculator")
        self.assertTrue(calc.is_active)

        wins = calc.get_windows()
        self.assertThat(len(wins), Equals(1))
        calc_win = wins[0]
        self.assertTrue(calc_win.is_focused)

        if calc_win.is_maximized:
            self.keybinding("window/restore")
            sleep(.1)

        if calc_win.monitor != self.panel_monitor:
            self.move_window_to_monitor(calc_win, self.panel_monitor)

        self.assertFalse(calc_win.is_maximized)
        self.assertThat(self.target_panel.title, Equals(calc.name))
