# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards,
#          Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from testtools.matchers import Equals, LessThan, GreaterThan

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.unity.launcher import Launcher
from autopilot.emulators.X11 import ScreenGeometry, Keyboard


class LauncherTests(AutopilotTestCase):
    """Test the launcher."""

    def setUp(self):
        super(LauncherTests, self).setUp()
        self.server = Launcher()
        sleep(1)

    def test_launcher_switcher_starts_at_index_zero(self):
        """Test that starting the Launcher switcher puts the keyboard focus on item 0."""
        sleep(.5)
        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))
        self.assertThat(self.server.key_nav_selection(), Equals(0))

    def test_launcher_switcher_end_works(self):
        """Test that ending the launcher switcher actually works."""
        sleep(.5)
        self.server.start_switcher()
        sleep(.5)
        self.server.end_switcher(cancel=True)
        sleep(.5)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))

    def test_launcher_switcher_next_works(self):
        """Moving to the next launcher item while switcher is activated must work."""
        sleep(.5)
        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.server.switcher_next()
        sleep(.5)
        self.assertThat(self.server.key_nav_selection(), Equals(1))

    def test_launcher_switcher_prev_works(self):
        """Moving to the previous launcher item while switcher is activated must work."""
        sleep(.5)
        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.server.switcher_next()
        sleep(.5)
        self.server.switcher_prev()
        self.assertThat(self.server.key_nav_selection(), Equals(0))

    def test_launcher_switcher_next_doesnt_show_shortcuts(self):
        """Moving forward in launcher switcher must not show launcher shortcuts."""
        sleep(.5)
        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)
        self.server.switcher_next()
        sleep(2)
        launcher = self.server.key_nav_monitor()
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(False))

    def test_launcher_switcher_prev_doesnt_show_shortcuts(self):
        """Moving backward in launcher switcher must not show launcher shortcuts."""
        sleep(.5)
        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)
        self.server.switcher_next()
        sleep(2)
        self.server.switcher_prev()
        sleep(2)
        launcher = self.server.key_nav_monitor()
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(False))


    def test_launcher_switcher_cycling_forward(self):
        """Launcher Switcher must loop through icons when cycling forwards"""
        sleep(.5)
        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        prev_icon = 0
        for icon in range(1, self.server.num_launcher_icons()):
            self.server.switcher_next()
            sleep(.25)
            # FIXME We can't directly check for selection/icon number equalty
            self.assertThat(prev_icon, LessThan(self.server.key_nav_selection()))
            prev_icon = self.server.key_nav_selection()

        sleep(.5)
        self.server.switcher_next()
        self.assertThat(self.server.key_nav_selection(), Equals(0))

    def test_launcher_switcher_cycling_backward(self):
        """Launcher Switcher must loop through icons when cycling backwards"""
        sleep(.5)
        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.server.switcher_prev()
        # FIXME We can't directly check for self.server.num_launcher_icons - 1
        self.assertThat(self.server.key_nav_selection(), GreaterThan(1))

    def test_launcher_keyboard_reveal_works(self):
        """Revealing launcher with keyboard must work."""
        self.server.keyboard_reveal_launcher()
        self.addCleanup(self.server.keyboard_unreveal_launcher)
        sleep(0.5)
        launcher = self.server.get_keyboard_controlled_launcher()
        self.assertThat(self.server.is_showing(launcher), Equals(True))

    def test_launcher_keyboard_reveal_shows_shortcut_hints(self):
        """Launcher icons must show shortcut hints after revealing with keyboard."""
        launcher = self.server.get_keyboard_controlled_launcher()
        self.server.move_mouse_to_right_of_launcher(launcher)
        self.server.keyboard_reveal_launcher()
        self.addCleanup(self.server.keyboard_unreveal_launcher)
        sleep(1)

        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

    def test_launcher_switcher_keeps_shorcuts(self):
        """Initiating launcher switcher after showing shortcuts must not hide shortcuts"""
        sleep(.5)
        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        self.server.keyboard_reveal_launcher()
        self.addCleanup(self.server.keyboard_unreveal_launcher)
        sleep(1)

        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

    def test_launcher_switcher_next_and_prev_keep_shortcuts(self):
        """Launcher switcher nedxt and prev actions must keep shortcuts after they've been shown."""
        sleep(.5)
        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        self.server.keyboard_reveal_launcher()
        self.addCleanup(self.server.keyboard_unreveal_launcher)
        sleep(1)

        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.server.switcher_next()
        sleep(.5)
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

        self.server.switcher_prev()
        sleep(.5)
        self.assertThat(self.server.are_shortcuts_showing(launcher), Equals(True))

    def test_launcher_switcher_ungrabbed_using_shorcuts(self):
        """Using some other shortcut while switcher is active must cancel switcher."""
        sleep(.5)

        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        self.server.keyboard_reveal_launcher()
        self.addCleanup(self.server.keyboard_unreveal_launcher)
        sleep(1)
        self.server.start_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.keyboard.press_and_release("s")
        sleep(.25)
        self.keyboard.press_and_release("Escape")
        sleep(.25)

        self.assertThat(self.server.key_nav_is_active(), Equals(False))

    def test_launcher_keynav_initiate_works(self):
        """Tests we can initiate keyboard navigation on the launcher."""
        sleep(.5)
        self.server.grab_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(True))

    def test_launcher_keynav_end_works(self):
        """Test that we can exit keynav mode."""
        sleep(.5)
        self.server.grab_switcher()
        sleep(.5)
        self.server.end_switcher(cancel=True)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))

    def test_launcher_keynav_starts_at_index_zero(self):
        """Test keynav mode starts at index 0."""
        sleep(.5)
        self.server.grab_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)

        self.assertThat(self.server.key_nav_selection(), Equals(0))

    def test_launcher_keynav_forward_works(self):
        """Must be able to move forwards while in keynav mode."""
        sleep(.5)
        self.server.grab_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)
        self.server.switcher_next()
        sleep(.5)
        self.assertThat(self.server.key_nav_selection(), Equals(1))

    def test_launcher_keynav_prev_works(self):
        """Must be able to move backwards while in keynav mode."""
        sleep(.5)
        self.server.grab_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)
        self.server.switcher_next()
        sleep(.5)
        self.server.switcher_prev()
        sleep(.5)
        self.assertThat(self.server.key_nav_selection(), Equals(0))

    def test_launcher_keynav_cycling_forward(self):
        """Launcher keynav must loop through icons when cycling forwards"""
        sleep(.5)
        self.server.grab_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.25)

        prev_icon = 0
        for icon in range(1, self.server.num_launcher_icons()):
            self.server.switcher_next()
            sleep(.25)
            # FIXME We can't directly check for selection/icon number equalty
            self.assertThat(prev_icon, LessThan(self.server.key_nav_selection()))
            prev_icon = self.server.key_nav_selection()

        sleep(.5)
        self.server.switcher_next()
        self.assertThat(self.server.key_nav_selection(), Equals(0))

    def test_launcher_keynav_cycling_backward(self):
        """Launcher keynav must loop through icons when cycling backwards"""
        sleep(.5)
        self.server.grab_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.25)

        self.server.switcher_prev()
        # FIXME We can't directly check for self.server.num_launcher_icons - 1
        self.assertThat(self.server.key_nav_selection(), GreaterThan(1))

    def test_launcher_keynav_can_open_quicklist(self):
        """Tests that we can open a quicklist from keynav mode."""
        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        sleep(.5)
        self.server.grab_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)
        self.server.switcher_next()
        sleep(.5)
        self.server.switcher_enter_quicklist()
        self.addCleanup(self.server.switcher_exit_quicklist)
        sleep(.5)
        self.assertThat(self.server.is_quicklist_open(launcher), Equals(True))

    def test_launcher_keynav_can_close_quicklist(self):
        """Tests that we can close a quicklist from keynav mode."""
        launcher = self.server.get_keyboard_controlled_launcher();
        self.server.move_mouse_to_right_of_launcher(launcher)
        sleep(.5)
        self.server.grab_switcher()
        self.addCleanup(self.server.end_switcher, True)
        sleep(.5)
        self.server.switcher_next()
        sleep(.5)
        self.server.switcher_enter_quicklist()
        sleep(.5)
        self.server.switcher_exit_quicklist()

        self.assertThat(self.server.is_quicklist_open(launcher), Equals(False))
        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(True))

class LauncherRevealTests(AutopilotTestCase):
    """Test the launcher reveal bahavior when in autohide mode."""

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

