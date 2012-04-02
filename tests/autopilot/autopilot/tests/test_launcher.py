# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards,
#          Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import logging
from testtools.matchers import Equals, NotEquals, LessThan, GreaterThan, Not, Is
from time import sleep

from autopilot.tests import AutopilotTestCase, multiply_scenarios
from autopilot.emulators.X11 import ScreenGeometry

logger = logging.getLogger(__name__)


def _make_scenarios():
    """Make scenarios for launcher test cases based on the number of configured
    monitors.
    """
    screen_geometry = ScreenGeometry()
    num_monitors = screen_geometry.get_num_monitors()

    if num_monitors == 1:
        monitor_scenarios =  [('Single Monitor', {'launcher_monitor': 0})]
    else:
        monitor_scenarios = [('Monitor %d' % (i), {'launcher_monitor': i}) for i in range(num_monitors)]

    launcher_mode_scenarios = [('launcher_on_primary', {'only_primary': True}),
                                ('launcher on all', {'only_primary': False})]
    return multiply_scenarios(monitor_scenarios, launcher_mode_scenarios)


class ScenariodLauncherTests(AutopilotTestCase):
    """A base class for all launcher tests that want to use scenarios to run on
    each launcher (for multi-monitor setups).
    """
    screen_geo = ScreenGeometry()
    scenarios = _make_scenarios()

    def get_launcher(self):
        """Get the launcher for the current scenario."""
        return self.launcher.get_launcher_for_monitor(self.launcher_monitor)

    def setUp(self):
        super(ScenariodLauncherTests, self).setUp()
        self.set_unity_log_level("unity.launcher", "DEBUG")
        self.set_unity_option('num_launchers', int(self.only_primary))

        if self.only_primary:
            try:
                old_primary_screen = self.screen_geo.get_primary_monitor()
                self.screen_geo.set_primary_monitor(self.launcher_monitor)
                self.addCleanup(self.screen_geo.set_primary_monitor, old_primary_screen)
            except ScreenGeometry.BlacklistedDriverError:
                self.skipTest("Impossible to set the monitor %d as primary" % self.launcher_monitor)


class LauncherTests(ScenariodLauncherTests):
    """Test the launcher."""

    def setUp(self):
        super(LauncherTests, self).setUp()
        sleep(1)

    def test_number_of_launchers(self):
        """Tests that the number of available launchers matches the current settings"""
        launchers_number = len(self.launcher.get_launchers())

        if self.only_primary:
            self.assertThat(launchers_number, Equals(1))
        else:
            monitors_number = self.screen_geo.get_num_monitors()
            self.assertThat(launchers_number, Equals(monitors_number))

    def test_launcher_for_monitor(self):
        """Tests that the launcher for monitor matches the current settings"""
        launcher = self.get_launcher()
        self.assertThat(launcher, NotEquals(None))

        if self.only_primary:
            for monitor in range(0, self.screen_geo.get_num_monitors()):
                if (monitor != self.launcher_monitor):
                    launcher = self.launcher.get_launcher_for_monitor(monitor)
                    self.assertThat(launcher, Equals(None))

    def test_launcher_switcher_starts_at_index_zero(self):
        """Test that starting the Launcher switcher puts the keyboard focus on item 0."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)

        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(False))
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_switcher_cancel_works(self):
        """Test that ending the launcher switcher actually works."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        sleep(.5)
        launcher_instance.switcher_cancel()
        sleep(.5)
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_launcher_switcher_escape_works(self):
        """Test that ending the launcher switcher actually works."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.25)
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        sleep(.25)
        self.keyboard.press_and_release("Escape")
        sleep(.25)
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_launcher_switcher_next_works(self):
        """Moving to the next launcher item while switcher is activated must work."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)
        logger.info("After starting, keynav selection is %d", self.launcher.key_nav_selection)

        launcher_instance.switcher_next()
        sleep(.5)
        logger.info("After next, keynav selection is %d", self.launcher.key_nav_selection)
        # The launcher model has hidden items, so the keynav indexes do not
        # increase by 1 each time. This test was failing because the 2nd icon
        # had an index of 2, not 1 as expected. The best we can do here is to
        # make sure that the index has increased. This opens us to the
        # possibility that the launcher really is skipping forward more than one
        # icon at a time, but we can't do much about that.
        self.assertThat(self.launcher.key_nav_selection, GreaterThan(0))

    def test_launcher_switcher_prev_works(self):
        """Moving to the previous launcher item while switcher is activated must work."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.25)
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

        launcher_instance.switcher_prev()
        sleep(.25)
        self.assertThat(self.launcher.key_nav_selection, NotEquals(0))

    def test_launcher_switcher_down_works(self):
        """Pressing the down arrow key while switcher is activated must work."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.25)
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

        launcher_instance.switcher_down()
        sleep(.25)
        # The launcher model has hidden items, so the keynav indexes do not
        # increase by 1 each time. This test was failing because the 2nd icon
        # had an index of 2, not 1 as expected. The best we can do here is to
        # make sure that the index has increased. This opens us to the
        # possibility that the launcher really is skipping forward more than one
        # icon at a time, but we can't do much about that.
        self.assertThat(self.launcher.key_nav_selection, GreaterThan(0))

    def test_launcher_switcher_up_works(self):
        """Pressing the up arrow key while switcher is activated must work."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.25)
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

        launcher_instance.switcher_up()
        sleep(.25)
        self.assertThat(self.launcher.key_nav_selection, NotEquals(0))

    def test_launcher_switcher_next_doesnt_show_shortcuts(self):
        """Moving forward in launcher switcher must not show launcher shortcuts."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)
        launcher_instance.switcher_next()
        sleep(2)
        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(False))

    def test_launcher_switcher_prev_doesnt_show_shortcuts(self):
        """Moving backward in launcher switcher must not show launcher shortcuts."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)
        launcher_instance.switcher_next()
        sleep(2)
        launcher_instance.switcher_prev()
        sleep(2)
        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(False))


    def test_launcher_switcher_cycling_forward(self):
        """Launcher Switcher must loop through icons when cycling forwards"""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)

        prev_icon = 0
        num_icons = self.launcher.model.num_launcher_icons()
        logger.info("This launcher has %d icons", num_icons)
        for icon in range(1, num_icons):
            launcher_instance.switcher_next()
            sleep(.25)
            # FIXME We can't directly check for selection/icon number equalty
            # since the launcher model also contains "hidden" icons that aren't
            # shown, so the selection index can increment by more than 1.
            self.assertThat(prev_icon, LessThan(self.launcher.key_nav_selection))
            prev_icon = self.launcher.key_nav_selection

        sleep(.5)
        launcher_instance.switcher_next()
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_switcher_cycling_backward(self):
        """Launcher Switcher must loop through icons when cycling backwards"""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)

        launcher_instance.switcher_prev()
        # FIXME We can't directly check for self.launcher.num_launcher_icons - 1
        self.assertThat(self.launcher.key_nav_selection, GreaterThan(1))

    def test_launcher_keyboard_reveal_works(self):
        """Revealing launcher with keyboard must work."""
        launcher_instance = self.get_launcher()
        launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(launcher_instance.keyboard_unreveal_launcher)
        sleep(0.5)
        self.assertThat(launcher_instance.is_showing(), Equals(True))

    def test_launcher_keyboard_reveal_shows_shortcut_hints(self):
        """Launcher icons must show shortcut hints after revealing with keyboard."""
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(launcher_instance.keyboard_unreveal_launcher)
        sleep(1)

        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(True))

    def test_launcher_switcher_keeps_shorcuts(self):
        """Initiating launcher switcher after showing shortcuts must not hide shortcuts"""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(launcher_instance.keyboard_unreveal_launcher)
        sleep(1)

        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)

        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(True))

    def test_launcher_switcher_next_and_prev_keep_shortcuts(self):
        """Launcher switcher next and prev actions must keep shortcuts after they've been shown."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(launcher_instance.keyboard_unreveal_launcher)
        sleep(1)

        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)

        launcher_instance.switcher_next()
        sleep(.5)
        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(True))

        launcher_instance.switcher_prev()
        sleep(.5)
        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(True))

    def test_launcher_switcher_using_shorcuts(self):
        """Using some other shortcut while switcher is active must cancel switcher."""
        sleep(.5)

        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(launcher_instance.keyboard_unreveal_launcher)
        sleep(1)
        launcher_instance.switcher_start()
        self.addCleanup(launcher_instance.switcher_cancel)
        sleep(.5)

        self.keyboard.press_and_release("s")
        sleep(.25)
        self.keyboard.press_and_release("Escape")
        sleep(.25)

        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_launcher_keynav_initiate_works(self):
        """Tests we can initiate keyboard navigation on the launcher."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        self.addCleanup(launcher_instance.key_nav_cancel)
        sleep(.5)

        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(True))

    def test_launcher_keynav_end_works(self):
        """Test that we can exit keynav mode."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        sleep(.5)
        launcher_instance.key_nav_cancel()
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(False))

    def test_launcher_keynav_starts_at_index_zero(self):
        """Test keynav mode starts at index 0."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        self.addCleanup(launcher_instance.key_nav_cancel)
        sleep(.5)

        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_keynav_forward_works(self):
        """Must be able to move forwards while in keynav mode."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        self.addCleanup(launcher_instance.key_nav_cancel)
        sleep(.5)
        launcher_instance.key_nav_next()
        sleep(.5)
        # The launcher model has hidden items, so the keynav indexes do not
        # increase by 1 each time. This test was failing because the 2nd icon
        # had an index of 2, not 1 as expected. The best we can do here is to
        # make sure that the index has increased. This opens us to the
        # possibility that the launcher really is skipping forward more than one
        # icon at a time, but we can't do much about that.
        self.assertThat(self.launcher.key_nav_selection, GreaterThan(0))

    def test_launcher_keynav_prev_works(self):
        """Must be able to move backwards while in keynav mode."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        self.addCleanup(launcher_instance.key_nav_cancel)
        sleep(.5)
        launcher_instance.key_nav_next()
        sleep(.5)
        launcher_instance.key_nav_prev()
        sleep(.5)
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_keynav_cycling_forward(self):
        """Launcher keynav must loop through icons when cycling forwards"""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        self.addCleanup(launcher_instance.key_nav_cancel)
        sleep(.25)

        prev_icon = 0
        for icon in range(1, self.launcher.model.num_launcher_icons()):
            launcher_instance.key_nav_next()
            sleep(.25)
            # FIXME We can't directly check for selection/icon number equalty
            # since the launcher model also contains "hidden" icons that aren't
            # shown, so the selection index can increment by more than 1.
            self.assertThat(prev_icon, LessThan(self.launcher.key_nav_selection))
            prev_icon = self.launcher.key_nav_selection

        sleep(.5)
        launcher_instance.key_nav_next()
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_keynav_cycling_backward(self):
        """Launcher keynav must loop through icons when cycling backwards"""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        self.addCleanup(launcher_instance.key_nav_cancel)
        sleep(.25)

        launcher_instance.key_nav_prev()
        # FIXME We can't directly check for self.launcher.num_launcher_icons - 1
        self.assertThat(self.launcher.key_nav_selection, GreaterThan(1))

    def test_launcher_keynav_can_open_quicklist(self):
        """Tests that we can open a quicklist from keynav mode."""
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        self.addCleanup(launcher_instance.key_nav_cancel)
        sleep(.5)
        launcher_instance.key_nav_next()
        sleep(.5)
        launcher_instance.key_nav_enter_quicklist()
        self.addCleanup(launcher_instance.key_nav_exit_quicklist)
        sleep(.5)
        self.assertThat(launcher_instance.is_quicklist_open(), Equals(True))

    def test_launcher_keynav_can_close_quicklist(self):
        """Tests that we can close a quicklist from keynav mode."""
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        sleep(.5)
        launcher_instance.key_nav_start()
        self.addCleanup(launcher_instance.key_nav_cancel)
        sleep(.5)
        launcher_instance.key_nav_next()
        sleep(.5)
        launcher_instance.key_nav_enter_quicklist()
        sleep(.5)
        launcher_instance.key_nav_exit_quicklist()

        self.assertThat(launcher_instance.is_quicklist_open(), Equals(False))
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(True))

    def test_launcher_keynav_mode_toggles(self):
        """Tests that keynav mode toggles with Alt+F1."""
        launcher_instance = self.get_launcher()

        launcher_instance.key_nav_start()
        launcher_instance.key_nav_start()

        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_launcher_keynav_alt_tab_quits(self):
        """Tests that alt+tab exits keynav mode."""
        launcher_instance = self.get_launcher()
        launcher_instance.key_nav_start()

        self.switcher.initiate()
        sleep(1)
        self.switcher.stop()

        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_launcher_keynav_alt_grave_quits(self):
        """Tests that alt+` exits keynav mode."""
        launcher_instance = self.get_launcher()
        launcher_instance.key_nav_start()

        self.switcher.initiate_detail_mode()
        sleep(1)
        self.switcher.stop()

        self.assertThat(self.launcher.key_nav_is_active, Equals(False))


    def test_software_center_add_icon(self):
        """ Test the ability to add a SoftwareCenterLauncherIcon """

        launcher_instance = self.get_launcher()
        sc_desktop_file = "/usr/share/applications/ubuntu-software-center.desktop"

        def cleanup():
            if icon is not None:
                launcher_instance.unlock_from_launcher(icon[0])

        # Check if SC is pinned to the launcher already
        icon = self.launcher.model.get_icon_by_desktop_file(sc_desktop_file)
        if icon != None:
            launcher_instance.unlock_from_launcher(icon[0])
            sleep(2.0) # Animation of removing icon can take over a second
        else:
            self.addCleanup(cleanup)

        self.launcher.add_launcher_item_from_position("Unity Test",
                                                   "softwarecenter",
                                                   100,
                                                   100,
                                                   32,
                                                   sc_desktop_file,
                                                   "")

        sleep(1.0)

        icon = self.launcher.model.get_icon_by_desktop_file(sc_desktop_file)
        self.assertThat(icon, Not(Is(None)))

        # Check for whether:
        # The new launcher icon has a 'Waiting to install' tooltip
        self.assertThat(icon.tooltip_text, Equals("Waiting to install"))


class LauncherRevealTests(ScenariodLauncherTests):
    """Test the launcher reveal bahavior when in autohide mode."""

    def setUp(self):
        super(LauncherRevealTests, self).setUp()
        self.set_unity_option('launcher_capture_mouse', True)
        self.set_unity_option('launcher_hide_mode', 1)
        launcher = self.get_launcher()
        for counter in range(10):
            sleep(1)
            if launcher.hidemode == 1:
                break
        self.assertThat(launcher.hidemode, Equals(1),
                        "Launcher did not enter auto-hide mode.")

    def test_reveal_on_mouse_to_edge(self):
        """Tests reveal of launchers by mouse pressure."""
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.mouse_reveal_launcher()
        self.assertThat(launcher_instance.is_showing(), Equals(True))

    def test_reveal_with_mouse_under_launcher(self):
        """Tests that the launcher hides properly if the
        mouse is under the launcher when it is revealed.

        """
        launcher_instance = self.get_launcher()

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
        launcher_instance = self.get_launcher()

        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.mouse_reveal_launcher()
        sleep(2)
        self.assertThat(launcher_instance.is_showing(), Equals(True))

    def test_launcher_does_not_reveal_with_mouse_down(self):
        """Launcher must not reveal if have mouse button 1 down."""
        launcher_instance = self.get_launcher()

        self.screen_geo.move_mouse_to_monitor(launcher_instance.monitor)
        self.mouse.press(1)
        launcher_instance.mouse_reveal_launcher()
        self.assertThat(launcher_instance.is_showing(), Equals(False))
        self.mouse.release(1)
