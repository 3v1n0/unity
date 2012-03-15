# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards,
#          Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import logging
from testtools.matchers import Equals, NotEquals, LessThan, GreaterThan
from time import sleep

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.X11 import ScreenGeometry

logger = logging.getLogger(__name__)

def _make_scenarios():
    """Make scenarios for launcher test cases based on the number of configured
    monitors.
    """
    screen_geometry = ScreenGeometry()
    num_monitors = screen_geometry.get_num_monitors()
    if num_monitors == 1:
        return [('Single Monitor', {'launcher_num': 0})]
    else:
        return [('Monitor %d' % (i), {'launcher_num': i}) for i in range(num_monitors)]

class LauncherTestCase(AutopilotTestCase):
    """A base class for all launcher tests that uses scenarios to run on
    each launcher (for multi-monitor setups).
    """
    scenarios = _make_scenarios()

    def setUp(self):
        super(LauncherTestCase, self).setUp()
        self.set_unity_log_level("unity.launcher", "DEBUG")
        self.launcher_instance = self._get_launcher()

    def tearDown(self):
        super(LauncherTestCase, self).tearDown()
        self.set_unity_log_level("unity.launcher", "INFO")

    def _get_launcher(self):
        """Get the launcher for the current scenario."""
        return self.launcher.get_launcher_for_monitor(self.launcher_num)

class LauncherSwitcherTests(LauncherTestCase):
    """ Tests the functionality of the launcher's switcher capability"""
    
    def setUp(self):
        super(LauncherSwitcherTests, self).setUp()
        self.launcher_instance.switcher_start()
        sleep(0.5)

    def tearDown(self):
        super(LauncherSwitcherTests, self).tearDown()
        if self.launcher_instance.in_switcher_mode:
            self.launcher_instance.switcher_cancel()

    def test_launcher_switcher_cancel(self):
        """Test that ending the launcher switcher actually works."""
        self.launcher_instance.switcher_cancel()
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_launcher_switcher_starts_at_index_zero(self):
        """Test that starting the Launcher switcher puts the keyboard focus on item 0."""
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(False))
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_switcher_next(self):
        """Moving to the next launcher item while switcher is activated must work."""
        self.launcher_instance.switcher_next()
        self.assertThat(self.launcher.key_nav_selection, Equals(1))

    def test_launcher_switcher_prev(self):
        """Moving to the previous launcher item while switcher is activated must work."""
        self.launcher_instance.switcher_prev()
        self.assertThat(self.launcher.key_nav_selection, NotEquals(0))

    def test_launcher_switcher_down(self):
        """Pressing the down arrow key while switcher is activated must work."""
        self.launcher_instance.switcher_down()
        self.assertThat(self.launcher.key_nav_selection, Equals(1))

    def test_launcher_switcher_up(self):
        """Pressing the up arrow key while switcher is activated must work."""
        self.launcher_instance.switcher_up()
        self.assertThat(self.launcher.key_nav_selection, NotEquals(0))

    def test_launcher_switcher_next_doesnt_show_shortcuts(self):
        """Moving forward in launcher switcher must not show launcher shortcuts."""
        self.launcher_instance.switcher_next()
        # sleep so that the shortcut timeout could be triggered
        sleep(2)
        self.assertThat(self.launcher_instance.are_shortcuts_showing(), Equals(False))

    def test_launcher_switcher_prev_doesnt_show_shortcuts(self):
        """Moving backward in launcher switcher must not show launcher shortcuts."""
        self.launcher_instance.switcher_prev()
        # sleep so that the shortcut timeout could be triggered
        sleep(2)
        self.assertThat(self.launcher_instance.are_shortcuts_showing(), Equals(False))

    def test_launcher_switcher_cycling_forward(self):
        """Launcher Switcher must loop through icons when cycling forwards"""
        prev_icon = 0
        num_icons = self.launcher.model.num_launcher_icons()
        logger.info("This launcher has %d icons", num_icons)
        for icon in range(1, num_icons):
            self.launcher_instance.switcher_next()
            sleep(.25)
            # FIXME We can't directly check for selection/icon number equalty
            # since the launcher model also contains "hidden" icons that aren't
            # shown, so the selection index can increment by more than 1.
            self.assertThat(prev_icon, LessThan(self.launcher.key_nav_selection))
            prev_icon = self.launcher.key_nav_selection

        sleep(.5)
        self.launcher_instance.switcher_next()
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_switcher_cycling_backward(self):
        """Launcher Switcher must loop through icons when cycling backwards"""
        self.launcher_instance.switcher_prev()
        # FIXME We can't directly check for self.launcher.num_launcher_icons - 1
        self.assertThat(self.launcher.key_nav_selection, GreaterThan(1))

    def test_launcher_switcher_using_shorcuts(self):
        """Using some other shortcut while switcher is active must cancel switcher."""
        self.keyboard.press_and_release("s")
        sleep(.25)
        self.keyboard.press_and_release("Escape")
        sleep(.25)
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

class LauncherShortcutTests(LauncherTestCase):
    def setUp(self):
        super(LauncherShortcutTests, self).setUp()
        self.launcher_instance.keyboard_reveal_launcher()
        sleep(2)

    def tearDown(self):
        super(LauncherShortcutTests, self).tearDown()
        self.launcher_instance.keyboard_unreveal_launcher()

    def test_launcher_keyboard_reveal_shows_shortcut_hints(self):
        """Launcher icons must show shortcut hints after revealing with keyboard."""
        self.assertThat(self.launcher_instance.are_shortcuts_showing(), Equals(True))

    def test_launcher_switcher_keeps_shorcuts(self):
        """Initiating launcher switcher after showing shortcuts must not hide shortcuts"""
        self.addCleanup(self.launcher_instance.switcher_cancel)
        self.launcher_instance.switcher_start()
        sleep(.5)

        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher_instance.are_shortcuts_showing(), Equals(True))

    def test_launcher_switcher_next_keeps_shortcuts(self):
        """Launcher switcher next action must keep shortcuts after they've been shown."""
        self.addCleanup(self.launcher_instance.switcher_cancel)
        self.launcher_instance.switcher_start()
        sleep(.5)

        self.launcher_instance.switcher_next()
        sleep(.5)
        self.assertThat(self.launcher_instance.are_shortcuts_showing(), Equals(True))

    def test_launcher_switcher_prev_keeps_shortcuts(self):
        """Launcher switcher prev action must keep shortcuts after they've been shown."""
        self.addCleanup(self.launcher_instance.switcher_cancel)
        self.launcher_instance.switcher_start()
        sleep(.5)

        self.launcher_instance.switcher_prev()
        sleep(.5)
        self.assertThat(self.launcher_instance.are_shortcuts_showing(), Equals(True))

class LauncherKeyNavTests(LauncherTestCase):
    """Test the launcher key navigation"""
    def setUp(self):
        super(LauncherKeyNavTests, self).setUp()
        self.launcher_instance.key_nav_start()

    def tearDown(self):
        super(LauncherKeyNavTests, self).tearDown()
        if self.launcher.key_nav_is_active:
            self.launcher_instance.key_nav_cancel()

    def test_launcher_keynav_initiate(self):
        """Tests we can initiate keyboard navigation on the launcher."""
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(True))

    def test_launcher_keynav_cancel(self):
        """Test that we can exit keynav mode."""
        self.launcher_instance.key_nav_cancel()
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(False))

    def test_launcher_keynav_starts_at_index_zero(self):
        """Test keynav mode starts at index 0."""
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_keynav_forward(self):
        """Must be able to move forwards while in keynav mode."""
        self.launcher_instance.key_nav_next()
        self.assertThat(self.launcher.key_nav_selection, Equals(1))

    def test_launcher_keynav_prev_works(self):
        """Must be able to move backwards while in keynav mode."""
        self.launcher_instance.key_nav_next()
        sleep(.5)
        self.launcher_instance.key_nav_prev()
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_keynav_cycling_forward(self):
        """Launcher keynav must loop through icons when cycling forwards"""
        prev_icon = 0
        for icon in range(1, self.launcher.model.num_launcher_icons()):
            self.launcher_instance.key_nav_next()
            sleep(.25)
            # FIXME We can't directly check for selection/icon number equalty
            # since the launcher model also contains "hidden" icons that aren't
            # shown, so the selection index can increment by more than 1.
            self.assertThat(prev_icon, LessThan(self.launcher.key_nav_selection))
            prev_icon = self.launcher.key_nav_selection

        sleep(.5)
        self.launcher_instance.key_nav_next()
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_keynav_cycling_backward(self):
        """Launcher keynav must loop through icons when cycling backwards"""
        self.launcher_instance.key_nav_prev()
        # FIXME We can't directly check for self.launcher.num_launcher_icons - 1
        self.assertThat(self.launcher.key_nav_selection, GreaterThan(1))
        
    def test_launcher_keynav_can_open_and_close_quicklist(self):
        """Tests that we can open and close a quicklist from keynav mode."""
        self.launcher_instance.key_nav_next()
        sleep(.5)
        self.launcher_instance.key_nav_enter_quicklist()
        self.assertThat(self.launcher_instance.is_quicklist_open(), Equals(True))
        sleep(.5)
        self.launcher_instance.key_nav_exit_quicklist()
        self.assertThat(self.launcher_instance.is_quicklist_open(), Equals(False))
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(True))

class LauncherRevealTests(LauncherTestCase):
    """Test the launcher reveal bahavior when in autohide mode."""

    def setUp(self):
        super(LauncherRevealTests, self).setUp()
        self.set_unity_option('launcher_hide_mode', True)
        sleep(1)

    def test_launcher_keyboard_reveal_works(self):
        """Revealing launcher with keyboard must work."""
        launcher_instance = self._get_launcher()
        launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(launcher_instance.keyboard_unreveal_launcher)
        sleep(0.5)
        self.assertThat(launcher_instance.is_showing(), Equals(True))

    def test_reveal_on_mouse_to_edge(self):
        """Tests reveal of launchers by mouse pressure."""
        launcher_instance = self._get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.mouse_reveal_launcher()
        self.assertThat(launcher_instance.is_showing(), Equals(True))

    def test_reveal_with_mouse_under_launcher(self):
        """Tests that the launcher hides properly if the
        mouse is under the launcher when it is revealed.

        """
        launcher_instance = self._get_launcher()

        launcher_instance.move_mouse_over_launcher()
        # we can't use "launcher_instance.keyboard_reveal_launcher()"
        # since it moves the mouse out of the way, invalidating the test.
        self.keybinding_hold("launcher/reveal")
        sleep(1)
        self.keybinding_release("launcher/reveal")
        sleep(2)
        self.assertThat(launcher_instance.is_showing(), Equals(False))

    def test_reveal_does_not_hide_again(self):
        """Tests reveal of launchers by mouse pressure to ensure it doesn't
        automatically hide again.

        """
        launcher_instance = self._get_launcher()

        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.mouse_reveal_launcher()
        sleep(2)
        self.assertThat(launcher_instance.is_showing(), Equals(True))

    def test_launcher_does_not_reveal_with_mouse_down(self):
        """Launcher must not reveal if have mouse button 1 down."""
        launcher_instance = self._get_launcher()
        screens = ScreenGeometry()

        screens.move_mouse_to_monitor(launcher_instance.monitor)
        self.mouse.press(1)
        launcher_instance.mouse_reveal_launcher()
        self.assertThat(launcher_instance.is_showing(), Equals(False))
        self.mouse.release(1)
