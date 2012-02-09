# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This script is designed to run unity in a test drive manner. It will drive
# X and test the GL calls that Unity makes, so that we can easily find out if
# we are triggering graphics driver/X bugs.

from subprocess import call
from testtools import TestCase
from time import sleep

from autopilot.utilities import make_window_skip_taskbar
from autopilot.emulators.unity.launcher import Launcher
from autopilot.emulators.unity.switcher import Switcher
from autopilot.emulators.bamf import Bamf

class InvisibleWindowTests(TestCase):
    """Test unity's handling of windows with the Skip-Tasklist flag set."""

    def test_invisible_windows_ignored(self):
        """Windows with the '_NET_WM_STATE_SKIP_TASKBAR' flag set must not appear
        in the launcher or the switcher.

        """
        b = Bamf()
        self.assertFalse(b.application_is_running('Calculator'))
        b.launch_application("gcalctool.desktop")
        self.addCleanup(call, ["killall", "gcalctool"])
        sleep(1)

        switcher = Switcher()
        launcher = Launcher()
        # calculator should be in both launcher AND switcher:
        icon_names = [i.tooltip_text for i in launcher.get_launcher_icons()]
        self.assertIn('Calculator', icon_names)

        switcher.initiate()
        icon_names = [i.tooltip_text for i in switcher.get_switcher_icons()]
        switcher.terminate()
        self.assertIn('Calculator', icon_names)
        # now set skip_tasklist:
        apps = b.get_running_applications_by_title('Calculator')
        self.assertEqual(1, len(apps))
        windows = apps[0].get_windows()
        self.assertEqual(1, len(windows))
        make_window_skip_taskbar(windows[0].x_win)
        self.addCleanup(make_window_skip_taskbar, windows[0].x_win, False)
        sleep(2)

        # calculator should now NOT be in both launcher AND switcher:
        icon_names = [i.tooltip_text for i in launcher.get_launcher_icons()]
        self.assertNotIn('Calculator', icon_names)

        switcher.initiate()
        icon_names = [i.tooltip_text for i in switcher.get_switcher_icons()]
        switcher.terminate()
        self.assertNotIn('Calculator', icon_names)

    def test_pinned_invisible_window(self):
        """Test behavior of an app with an invisible window that's pinned to the launher."""
        b = Bamf()
        self.assertFalse(b.application_is_running('Calculator'))
        b.launch_application("gcalctool.desktop")
        self.addCleanup(call, ["killall", "gcalctool"])
        # need to pin the app to the launcher - this could be tricky.
        launcher = Launcher()
        launcher.reveal_launcher(0)
        icons = launcher.get_launcher_icons()
        # launcher.grab_switcher()
        calc_icon = None
        for icon in icons:
            if icon.tooltip_text == 'Calculator':
                calc_icon = icon
                break

        self.assertIsNotNone(calc_icon, "Could not find calculator in launcher.")
        launcher.lock_to_launcher(calc_icon)
        self.addCleanup(launcher.unlock_from_launcher, calc_icon)

        # make calc window skip the taskbar:
        apps = b.get_running_applications_by_title('Calculator')
        self.assertEqual(1, len(apps))
        windows = apps[0].get_windows()
        self.assertEqual(1, len(windows))
        make_window_skip_taskbar(windows[0].x_win)

        # clicking on launcher icon should start a new instance:
        launcher.click_launcher_icon(calc_icon)
        sleep(1)

        # should now be one app with two windows:
        apps = b.get_running_applications_by_title('Calculator')
        self.assertEqual(1, len(apps))
        windows = apps[0].get_windows()
        self.assertEqual(2, len(windows))
