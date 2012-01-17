# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License version 3, as published 
# by the Free Software Foundation.

from testtools import TestCase
from time import sleep
from subprocess import call

from autopilot.emulators.bamf import Bamf
from autopilot.emulators.X11 import Keyboard
from autopilot.emulators.unity import Launcher
from autopilot.glibrunner import GlibRunner


class ShowDesktopTests(TestCase):
    """Test the 'Show Desktop' functionality."""
    run_test_with = GlibRunner

    def test_showdesktop(self):
        """This test shows that the "show desktop" mode works correctly

        #. Open two applications
        #. Use either alt-tab or ctrl-alt-d to activate "show desktop" mode
        #. Use either alt-tab or ctrl-alt-d to deactivate "show desktop" mode
        #. Use either alt-tab or ctrl-alt-d to activate "show desktop" mode
        #. Select an active application from the launcher
        #. Use either alt-tab or ctrl-alt-d to deactivate "show desktop" mode

        Outcome
          Both windows will fade out, both windows will fade in, both windows
          will fade out, the clicked application will fade in only, all other
          windows will fade in.

        """
        bamf = Bamf()
        
        bamf.launch_application("gucharmap.desktop")
        self.addCleanup(call, ["killall", "gcalctool"])
        bamf.launch_application("gcalctool.desktop")
        self.addCleanup(call, ["killall", "gucharmap"])
        
        open_app_names = [a.name for a in bamf.get_running_applications() if a.user_visible]
        self.assertIn('Character Map', open_app_names)
        self.assertIn('Calculator', open_app_names)

        # show desktop, verify all windows are hidden:
        kb = Keyboard()
        kb.press_and_release(['Control_L','Meta_L','d'])
        sleep(1)
        open_wins = bamf.get_open_windows() 
        self.assertGreaterEqual(len(open_wins), 2)
        for win in open_wins:
            self.assertTrue(win.is_valid)
            self.assertTrue(win.is_hidden, "Window '%s' is not hidden after show desktop activated." % (win.title))

        # hide desktop, verify all windows are shown:
        kb.press_and_release(['Control_L','Meta_L','d'])
        sleep(1)
        for win in bamf.get_open_windows():
            self.assertTrue(win.is_valid)
            self.assertFalse(win.is_hidden, "Window '%s' is shown after show desktop deactivated." % (win.title))

        # show desktop again, verify all windows are hidden.
        kb.press_and_release(['Control_L','Meta_L','d'])
        sleep(1)
        for win in bamf.get_open_windows():
            self.assertTrue(win.is_valid)
            self.assertTrue(win.is_hidden, "Window '%s' is not hidden after show desktop activated." % (win.title))

        # We'll un-minimise the character map - find it's launcherIcon in the launcher:
        l = Launcher()

        launcher_icons = l.get_launcher_icons()
        found = False
        for icon in launcher_icons:
            if icon.tooltip_text == 'Character Map':
                found = True
                l.click_launcher_icon(icon)
        self.assertTrue(found, "Could not find launcher icon in launcher.")

        sleep(1)
        for win in bamf.get_open_windows():
            if win.is_valid:
                if win.title == 'Character Map':
                    self.assertFalse(win.is_hidden, "Character map did not un-hide from launcher.")
                else:
                    self.assertTrue(win.is_hidden, "Window '%s' should still be hidden." % (win.title))

        # hide desktop - now all windows should be visible:
        kb.press_and_release(['Control_L','Meta_L','d'])
        sleep(1)
        for win in bamf.get_open_windows():
            if win.is_valid:
                self.assertFalse(win.is_hidden, "Window '%s' is shown after show desktop deactivated." % (win.title))