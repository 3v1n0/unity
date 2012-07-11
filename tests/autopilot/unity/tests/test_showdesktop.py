# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from time import sleep

from unity.emulators.icons import DesktopLauncherIcon
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
        """Launch character map and calculator apps."""
        self.start_app('Character Map', locale='C')
        self.start_app('Calculator', locale='C')
        sleep(1)

    def test_showdesktop_hides_apps(self):
        """Show Desktop keyboard shortcut must hide applications."""
        self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.window_manager.enter_show_desktop()
        self.addCleanup(self.window_manager.leave_show_desktop)

        open_wins = self.bamf.get_open_windows()
        self.assertGreaterEqual(len(open_wins), 2)
        for win in open_wins:
            self.assertTrue(win.is_valid)
            self.assertTrue(win.is_hidden, "Window '%s' is not hidden after show desktop activated." % (win.title))

    def test_showdesktop_unhides_apps(self):
        """Show desktop shortcut must re-show all hidden apps."""
        self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.window_manager.enter_show_desktop()
        self.addCleanup(self.window_manager.leave_show_desktop)

        open_wins = self.bamf.get_open_windows()
        self.assertGreaterEqual(len(open_wins), 2)
        for win in open_wins:
            self.assertTrue(win.is_valid)
            self.assertTrue(win.is_hidden, "Window '%s' is not hidden after show desktop activated." % (win.title))

        # un-show desktop, verify all windows are shown:
        self.window_manager.leave_show_desktop()

        for win in self.bamf.get_open_windows():
            self.assertTrue(win.is_valid)
            self.assertFalse(win.is_hidden, "Window '%s' is shown after show desktop deactivated." % (win.title))

    def test_unhide_single_app(self):
        """Un-hide a single app from launcher after hiding all apps."""
        self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.window_manager.enter_show_desktop()
        self.addCleanup(self.window_manager.leave_show_desktop)

        open_wins = self.bamf.get_open_windows()
        self.assertGreaterEqual(len(open_wins), 2)
        for win in open_wins:
            self.assertTrue(win.is_valid)
            self.assertTrue(win.is_hidden, "Window '%s' is not hidden after show desktop activated." % (win.title))

        # We'll un-minimise the character map - find it's launcherIcon in the launcher:
        charmap_icon = self.launcher.model.get_icon(desktop_id="gucharmap.desktop")
        if charmap_icon:
            self.launcher.get_launcher_for_monitor(0).click_launcher_icon(charmap_icon)
        else:
            self.fail("Could not find launcher icon in launcher.")

        sleep(3)
        for win in self.bamf.get_open_windows():
            if win.is_valid:
                if win.title == 'Character Map':
                    self.assertFalse(win.is_hidden, "Character map did not un-hide from launcher.")
                else:
                    self.assertTrue(win.is_hidden, "Window '%s' should still be hidden." % (win.title))

        # hide desktop - now all windows should be visible:
        self.window_manager.leave_show_desktop()

        for win in self.bamf.get_open_windows():
            if win.is_valid:
                self.assertFalse(win.is_hidden, "Window '%s' is not shown after show desktop deactivated." % (win.title))

    def test_showdesktop_switcher(self):
        """Show desktop item in switcher should hide all hidden apps."""
        self.launch_test_apps()

        # show desktop, verify all windows are hidden:
        self.switcher.initiate()
        sleep(0.5)
        found = False
        switcher_model = self.switcher.controller.model
        for i in switcher_model.icons:
            current_icon = self.switcher.current_icon
            self.assertIsNotNone(current_icon)
            if isinstance(current_icon, DesktopLauncherIcon):
                found = True
                break
            self.switcher.previous_icon()
            sleep(0.25)
        self.assertTrue(found, "Could not find 'Show Desktop' entry in switcher.")
        self.addCleanup(self.window_manager.leave_show_desktop)

        self.switcher.select()

        sleep(3)
        open_wins = self.bamf.get_open_windows()
        self.assertGreaterEqual(len(open_wins), 2)
        for win in open_wins:
            self.assertTrue(win.is_valid)
            self.assertTrue(win.is_hidden, "Window '%s' is not hidden after show desktop activated." % (win.title))
