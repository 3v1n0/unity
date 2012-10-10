# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from time import sleep

from unity.tests import UnityTestCase


class ShowDesktopTests(UnityTestCase):
    """Test the 'Show Desktop' functionality."""

    def setUp(self):
        super(ShowDesktopTests, self).setUp()
        self.set_unity_log_level("unity.plugin", "DEBUG")
        # we need this to let the unity models update after we shutdown apps
        # before we start the next test.
        sleep(2)

    def launch_test_apps(self):
        """Launch character map and calculator apps, and return their windows."""
        char_win = self.start_app_window('Character Map', locale='C')
        calc_win = self.start_app_window('Calculator', locale='C')
        return (char_win, calc_win)

    def test_showdesktop_hides_apps(self):
        """Show Desktop keyboard shortcut must hide applications."""
        test_windows = self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.window_manager.enter_show_desktop()
        self.addCleanup(self.window_manager.leave_show_desktop)

        for win in test_windows:
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=True)

    def test_showdesktop_unhides_apps(self):
        """Show desktop shortcut must re-show all hidden apps."""
        test_windows = self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.window_manager.enter_show_desktop()
        self.addCleanup(self.window_manager.leave_show_desktop)

        for win in test_windows:
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=True)

        # un-show desktop, verify all windows are shown:
        self.window_manager.leave_show_desktop()

        for win in test_windows:
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=False)

    def test_unhide_single_app(self):
        """Un-hide a single app from launcher after hiding all apps."""
        charmap, calc = self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.window_manager.enter_show_desktop()
        self.addCleanup(self.window_manager.leave_show_desktop)

        for win in (charmap, calc):
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=True)

        # We'll un-minimise the character map - find it's launcherIcon in the launcher:
        charmap_icon = self.launcher.model.get_icon(desktop_id="gucharmap.desktop")
        if charmap_icon:
            self.launcher.get_launcher_for_monitor(0).click_launcher_icon(charmap_icon)
        else:
            self.fail("Could not find launcher icon in launcher.")

        self.assertProperty(charmap, is_hidden=False)
        self.assertProperty(calc, is_hidden=True)

        # hide desktop - now all windows should be visible:
        self.window_manager.leave_show_desktop()

        for win in (charmap, calc):
            self.assertProperty(win, is_hidden=False)

    def test_showdesktop_switcher(self):
        """Show desktop item in switcher should hide all hidden apps."""
        test_windows = self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.switcher.initiate()
        self.switcher.select_icon(self.switcher.DIRECTION_BACKWARDS, tooltip_text="Show Desktop")
        self.addCleanup(self.window_manager.leave_show_desktop)
        self.switcher.select()

        for win in test_windows:
            self.assertProperty(win, is_valid=True)
            self.assertProperty(win, is_hidden=True)
