# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards,
#          Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
import logging
from testtools.matchers import Equals, GreaterThan
from time import sleep

from unity.tests.launcher import LauncherTestCase

logger = logging.getLogger(__name__)


class LauncherRevealTests(LauncherTestCase):
    """Test the launcher reveal behavior when in autohide mode."""

    def setUp(self):
        super(LauncherRevealTests, self).setUp()
        # these automatically reset to the original value, as implemented in AutopilotTestCase
        self.set_unity_option('launcher_capture_mouse', True)
        self.set_unity_option('launcher_hide_mode', 1)
        launcher = self.get_launcher()
        self.assertThat(launcher.hidemode, Eventually(Equals(1)))

    def test_launcher_keyboard_reveal_works(self):
        """Revealing launcher with keyboard must work."""
        self.launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(self.launcher_instance.keyboard_unreveal_launcher)
        self.assertThat(self.launcher_instance.is_showing, Eventually(Equals(True)))

    def test_reveal_on_mouse_to_edge(self):
        """Tests reveal of launchers by mouse pressure."""
        self.launcher_instance.move_mouse_to_right_of_launcher()
        self.launcher_instance.mouse_reveal_launcher()
        self.assertThat(self.launcher_instance.is_showing, Eventually(Equals(True)))

    def test_reveal_with_mouse_under_launcher(self):
        """The Launcher must hide properly if the mouse is under the launcher."""

        self.launcher_instance.move_mouse_over_launcher()
        # we can't use "launcher_instance.keyboard_reveal_launcher()"
        # since it moves the mouse out of the way, invalidating the test.
        self.keybinding_hold("launcher/reveal")
        sleep(1)
        self.keybinding_release("launcher/reveal")
        self.assertThat(self.launcher_instance.is_showing, Eventually(Equals(False)))

    def test_reveal_does_not_hide_again(self):
        """Tests reveal of launchers by mouse pressure to ensure it doesn't
        automatically hide again.
        """
        self.launcher_instance.move_mouse_to_right_of_launcher()
        self.launcher_instance.mouse_reveal_launcher()
        self.assertThat(self.launcher_instance.is_showing, Eventually(Equals(True)))

    def test_launcher_does_not_reveal_with_mouse_down(self):
        """Launcher must not reveal if have mouse button 1 down."""
        self.display.move_mouse_to_display(self.launcher_instance.monitor)
        self.mouse.press(1)
        self.addCleanup(self.mouse.release, 1)
        #FIXME: This is really bad API. it says reveal but it's expected to fail. bad bad bad!!
        self.launcher_instance.mouse_reveal_launcher()
        # Need a sleep here otherwise this test would pass even if the code failed.
        # THis test needs to be rewritten...
        sleep(5)
        self.assertThat(self.launcher_instance.is_showing, Equals(False))

    def test_launcher_stays_open_after_spread(self):
        """Clicking on the launcher to close an active spread must not hide the launcher."""
        char_win1 = self.process_manager.start_app_window("Character Map")
        char_win2 = self.process_manager.start_app_window("Character Map")
        char_app = char_win1.application

        char_icon = self.unity.launcher.model.get_icon(desktop_id=char_app.desktop_file)

        self.launcher_instance.click_launcher_icon(char_icon, move_mouse_after=False)
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))
        self.launcher_instance.click_launcher_icon(char_icon, move_mouse_after=False)

        self.assertThat(self.launcher_instance.is_showing, Eventually(Equals(True)))
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(False)))

    def test_launcher_stays_open_after_icon_click(self):
        """Clicking on a launcher icon must not hide the launcher."""
        char_win = self.process_manager.start_app_window("Character Map")
        char_app = char_win.application

        char_icon = self.unity.launcher.model.get_icon(desktop_id=char_app.desktop_file)
        self.launcher_instance.click_launcher_icon(char_icon, move_mouse_after=False)

        # Have to sleep to give the launcher time to hide (what the old behavior was)
        sleep(5)

        self.assertThat(self.launcher_instance.is_showing, Eventually(Equals(True)))

    def test_new_icon_has_the_shortcut(self):
         """New icons should have an associated shortcut"""
         if self.unity.launcher.model.num_bamf_launcher_icons() >= 10:
             self.skip("There are already more than 9 icons in the launcher")

         desktop_file = self.process_manager.KNOWN_APPS['Calculator']['desktop-file']
         if self.unity.launcher.model.get_icon(desktop_id=desktop_file) != None:
             self.skip("Calculator icon is already on the launcher.")

         self.process_manager.start_app('Calculator')
         icon = self.unity.launcher.model.get_icon(desktop_id=desktop_file)
         self.assertThat(icon.shortcut, GreaterThan(0))
