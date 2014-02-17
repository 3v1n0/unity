# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from testtools import skip
from time import sleep

from unity.emulators.switcher import SwitcherDirection
from unity.tests import UnityTestCase


class ShowDesktopTests(UnityTestCase):
    """Test the 'Show Desktop' functionality."""

    def setUp(self):
        super(ShowDesktopTests, self).setUp()
        self.set_unity_log_level("unity.wm.compiz", "DEBUG")
        # we need this to let the unity models update after we shutdown apps
        # before we start the next test.
        sleep(2)

    def launch_test_apps(self):
        """Launch character map and calculator apps, and return their windows."""
        char_win = self.process_manager.start_app_window('Character Map', locale='C')
        calc_win = self.process_manager.start_app_window('Calculator', locale='C')
        return (char_win, calc_win)

    def test_showdesktop_hides_apps(self):
        """Show Desktop keyboard shortcut must hide applications."""
        test_windows = self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        for win in test_windows:
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=True)

    def test_showdesktop_unhides_apps(self):
        """Show desktop shortcut must re-show all hidden apps."""
        test_windows = self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        for win in test_windows:
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=True)

        # un-show desktop, verify all windows are shown:
        self.unity.window_manager.leave_show_desktop()

        for win in test_windows:
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=False)

    def test_unhide_single_app(self):
        """Un-hide a single app from launcher after hiding all apps."""
        charmap, calc = self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        for win in (charmap, calc):
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=True)

        # We'll un-minimise the character map - find it's launcherIcon in the launcher:
        charmap_icon = self.unity.launcher.model.get_icon(desktop_id="gucharmap.desktop")
        if charmap_icon:
            self.unity.launcher.get_launcher_for_monitor(0).click_launcher_icon(charmap_icon)
        else:
            self.fail("Could not find launcher icon in launcher.")

        self.assertProperty(charmap, is_hidden=False)
        self.assertProperty(calc, is_hidden=True)

        # Need to re-enter show desktop since the CharMap is visible so the cleanup handlers 
        # get the correct show desktop state
        self.unity.window_manager.enter_show_desktop()

    def test_showdesktop_closes_dash(self):
        """Show Desktop must close Dash if it's open"""
        test_windows = self.launch_test_apps()
        self.unity.dash.ensure_visible()

        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))

    def test_showdesktop_closes_hud(self):
        """Show Desktop must close Hud if it's open"""
        test_windows = self.launch_test_apps()
        self.unity.hud.ensure_visible()

        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    @skip("Breaks following tests due to SDM bug")
    def test_showdesktop_switcher(self):
        """Show desktop item in switcher should hide all hidden apps."""
        test_windows = self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.unity.switcher.initiate()
        self.unity.switcher.select_icon(SwitcherDirection.BACKWARDS, tooltip_text="Show Desktop")
        self.addCleanup(self.unity.window_manager.leave_show_desktop)
        self.switcher.select()

        for win in test_windows:
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=True)
