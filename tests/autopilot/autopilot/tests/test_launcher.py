# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards,
#          Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools.matchers import Equals, LessThan, GreaterThan

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.unity.launcher import Launcher
from autopilot.emulators.X11 import ScreenGeometry
from autopilot.glibrunner import GlibRunner

from time import sleep


class LauncherTests(AutopilotTestCase):
    """Test the launcher."""
    run_test_with = GlibRunner

    def setUp(self):
        super(LauncherTests, self).setUp()
        self.server = Launcher()
        sleep(1)

    def test_launcher_switcher_ungrabbed_starts_on_zero(self):
        """Tests basic key nav integration without keyboard grabs."""
        sleep(.25)

        self.server.start_switcher()
        sleep(.25)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        self.server.end_switcher(True)
        sleep(.25)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))

    def test_launcher_switcher_ungrabbed(self):
        """Tests basic key nav integration without keyboard grabs."""
        sleep(.5)

        self.server.start_switcher()
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        self.server.switcher_next()
        sleep(.5)
        self.assertThat(0, LessThan(self.server.key_nav_selection()))

        sleep(2)
        launcher = self.server.key_nav_monitor()
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(False))

        self.server.switcher_prev()
        sleep(.5)
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        self.server.end_switcher(True)
        sleep(.5)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))

    def test_launcher_switcher_ungrabbed_cycling_forward(self):
        """Tests basic key nav without keyboard grabs, cycling over items."""
        sleep(.5)

        self.server.start_switcher()
        sleep(.25)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        prev_icon = 0
        for icon in range(1, self.server.num_launcher_icons()):
            self.server.switcher_next()
            sleep(.25)
            self.assertThat(prev_icon, LessThan(self.server.key_nav_selection()))
            prev_icon = self.server.key_nav_selection()

        sleep(.5)
        self.server.switcher_next()
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        self.server.end_switcher(True)

    def test_launcher_switcher_ungrabbed_cycling_backward(self):
        """Tests basic key nav without keyboard grabs, cycling over items."""
        sleep(.5)

        self.server.start_switcher()
        sleep(.25)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        self.server.switcher_prev()
        self.assertThat(self.server.key_nav_selection(), GreaterThan(0))

        self.server.end_switcher(True)

    def test_launcher_switcher_ungrabbed_showing_shorcuts(self):
        """Tests basic key nav integration without keyboard grabs but making
        the launcher show the icon shortcuts.

        """
        sleep(.5)

        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        self.server.keyboard_reveal_launcher()
        sleep(1)

        self.assertThat(self.server.is_showing(launcher), Equals(True))
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

        self.server.start_switcher()
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))
        self.assertThat(self.server.key_nav_selection(), Equals(0))
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

        self.server.switcher_next()
        sleep(.5)
        self.assertThat(0, LessThan(self.server.key_nav_selection()))
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

        self.server.switcher_prev()
        sleep(.5)
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        self.server.end_switcher(True)
        sleep(.5)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(False))

    def test_launcher_switcher_ungrabbed_using_shorcuts(self):
        """Tests basic key nav integration without keyboard grabs but making
        the launcher show the icon shortcuts, and using them to disable the switcher.

        """
        sleep(.5)

        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        self.server.keyboard_reveal_launcher()
        sleep(1)

        self.assertThat(self.server.is_showing(launcher), Equals(True))
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

        self.server.start_switcher()
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))
        self.assertThat(self.server.key_nav_selection(), Equals(0))
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

        # Launch the workspace switcher, it should also terminate the key navigation
        self.keyboard.press_and_release("s")
        sleep(.25)
        self.keyboard.press_and_release("Escape")
        sleep(.25)

        self.assertThat(self.server.key_nav_is_active(), Equals(False))
        self.server.keyboard_unreveal_launcher()

    def test_launcher_switcher_grabbed(self):
        """Tests basic key nav integration via keyboard grab."""
        sleep(.5)

        self.server.grab_switcher()
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(True))
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        self.server.switcher_next()
        sleep(.5)
        self.assertThat(0, LessThan(self.server.key_nav_selection()))

        self.server.switcher_prev()
        sleep(.5)
        self.assertThat(self.server.key_nav_selection(), Equals(0))

        self.server.end_switcher(True)
        sleep(.5)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))

    def test_launcher_switcher_quicklist_interaction(self):
        """Tests that the key nav opens and closes quicklists properly and
        regrabs afterwards.

        """
        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        sleep(.5)

        self.server.grab_switcher()
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(True))

        self.server.switcher_next()
        sleep(.5)

        self.server.switcher_enter_quicklist()
        sleep(.5)
        self.assertThat(self.server.is_quicklist_open(launcher), Equals(True))
        self.server.switcher_exit_quicklist()
        sleep(.5)

        self.assertThat(self.server.is_quicklist_open(launcher), Equals(False))
        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(True))

        self.server.end_switcher(True)
        sleep(.5)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))

    def test_launcher_keyboard_shortcuts_showing(self):
        """Test if the key to reveal the launcher if pressed enough makes
        the keyboard-controlled launcher to show the shortcuts.

        """
        sleep(.5)
        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        self.server.keyboard_reveal_launcher()
        sleep(1)

        self.assertThat(self.server.is_showing(launcher), Equals(True))
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

        self.server.keyboard_unreveal_launcher()
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(False))


class LauncherRevealTests(AutopilotTestCase):
    """Test the launcher reveal bahavior when in autohide mode."""
    run_test_with = GlibRunner

    def setUp(self):
        super(LauncherRevealTests, self).setUp()
        self.launcher = Launcher()
        self.set_unity_option('launcher_hide_mode', True)
        sleep(1)

    def test_reveal_on_mouse_to_edge(self):
        """Tests reveal of launchers by mouse pressure."""
        num_launchers = self.launcher.num_launchers()

        for x in range(num_launchers):
            self.launcher.move_mouse_to_right_of_launcher(x)
            self.launcher.reveal_launcher(x)
            self.assertThat(self.launcher.is_showing(x), Equals(True))

    def test_reveal_with_mouse_under_launcher(self):
        """Tests that the launcher hides properly if the
        mouse is under the launcher when it is revealed.

        """
        num_launchers = self.launcher.num_launchers()

        for x in range(num_launchers):
            self.launcher.move_mouse_over_launcher(x)
            self.launcher.keyboard_reveal_launcher()
            self.launcher.keyboard_unreveal_launcher()
            sleep(1)
            self.assertThat(self.launcher.is_showing(x), Equals(False))

    def test_reveal_does_not_hide_again(self):
        """Tests reveal of launchers by mouse pressure to ensure it doesn't
        automatically hide again.

        """
        num_launchers = self.launcher.num_launchers()

        for x in range(num_launchers):
            self.launcher.move_mouse_to_right_of_launcher(x)
            self.launcher.reveal_launcher(x)
            sleep(2)
            self.assertThat(self.launcher.is_showing(x), Equals(True))

    def test_launcher_does_not_reveal_with_mouse_down(self):
        """Launcher must not reveal if have mouse button 1 down."""
        num_launchers = self.launcher.num_launchers()
        screens = ScreenGeometry()

        for x in range(num_launchers):
            screens.move_mouse_to_monitor(x)
            self.mouse.press(1)
            self.launcher.reveal_launcher(x)
            self.assertThat(self.launcher.is_showing(x), Equals(False))
            self.mouse.release(1)

