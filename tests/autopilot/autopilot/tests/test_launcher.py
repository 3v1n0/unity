# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards,
#          Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import dbus
import logging
from testtools.matchers import Equals, LessThan, GreaterThan
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


class ScenariodLauncherTests(AutopilotTestCase):
    """A base class for all launcher tests that want to use scenarios to run on
    each launcher (for multi-monitor setups).
    """

    scenarios = _make_scenarios()

    def get_launcher(self):
        """Get the launcher for the current scenario."""
        return self.launcher.get_launcher_for_monitor(self.launcher_num)


class LauncherTests(ScenariodLauncherTests):
    """Test the launcher."""

    def setUp(self):
        super(LauncherTests, self).setUp()
        sleep(1)

    def test_launcher_switcher_starts_at_index_zero(self):
        """Test that starting the Launcher switcher puts the keyboard focus on item 0."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)

        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(False))
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_switcher_end_works(self):
        """Test that ending the launcher switcher actually works."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.start_switcher()
        sleep(.5)
        launcher_instance.end_switcher(cancel=True)
        sleep(.5)
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_launcher_switcher_next_works(self):
        """Moving to the next launcher item while switcher is activated must work."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)

        launcher_instance.switcher_next()
        sleep(.5)
        self.assertThat(self.launcher.key_nav_selection, Equals(1))

    def test_launcher_switcher_prev_works(self):
        """Moving to the previous launcher item while switcher is activated must work."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)

        launcher_instance.switcher_next()
        sleep(.5)
        launcher_instance.switcher_prev()
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_switcher_next_doesnt_show_shortcuts(self):
        """Moving forward in launcher switcher must not show launcher shortcuts."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)
        launcher_instance.switcher_next()
        sleep(2)
        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(False))

    def test_launcher_switcher_prev_doesnt_show_shortcuts(self):
        """Moving backward in launcher switcher must not show launcher shortcuts."""
        sleep(.5)
        launcher_instance = self.get_launcher()
        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
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
        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
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
        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
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

        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
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

        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)

        launcher_instance.switcher_next()
        sleep(.5)
        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(True))

        launcher_instance.switcher_prev()
        sleep(.5)
        self.assertThat(launcher_instance.are_shortcuts_showing(), Equals(True))

    def test_launcher_switcher_ungrabbed_using_shorcuts(self):
        """Using some other shortcut while switcher is active must cancel switcher."""
        sleep(.5)

        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(launcher_instance.keyboard_unreveal_launcher)
        sleep(1)
        launcher_instance.start_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
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
        launcher_instance.grab_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)

        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(True))

    def test_launcher_keynav_end_works(self):
        """Test that we can exit keynav mode."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.grab_switcher()
        sleep(.5)
        launcher_instance.end_switcher(cancel=True)
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(False))

    def test_launcher_keynav_starts_at_index_zero(self):
        """Test keynav mode starts at index 0."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.grab_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)

        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_keynav_forward_works(self):
        """Must be able to move forwards while in keynav mode."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.grab_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)
        launcher_instance.switcher_next()
        sleep(.5)
        self.assertThat(self.launcher.key_nav_selection, Equals(1))

    def test_launcher_keynav_prev_works(self):
        """Must be able to move backwards while in keynav mode."""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.grab_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)
        launcher_instance.switcher_next()
        sleep(.5)
        launcher_instance.switcher_prev()
        sleep(.5)
        self.assertThat(self.launcher.key_nav_selection, Equals(0))

    def test_launcher_keynav_cycling_forward(self):
        """Launcher keynav must loop through icons when cycling forwards"""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.grab_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.25)

        prev_icon = 0
        for icon in range(1, self.launcher.model.num_launcher_icons()):
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

    def test_launcher_keynav_cycling_backward(self):
        """Launcher keynav must loop through icons when cycling backwards"""
        launcher_instance = self.get_launcher()
        sleep(.5)
        launcher_instance.grab_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.25)

        launcher_instance.switcher_prev()
        # FIXME We can't directly check for self.launcher.num_launcher_icons - 1
        self.assertThat(self.launcher.key_nav_selection, GreaterThan(1))

    def test_launcher_keynav_can_open_quicklist(self):
        """Tests that we can open a quicklist from keynav mode."""
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        sleep(.5)
        launcher_instance.grab_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)
        launcher_instance.switcher_next()
        sleep(.5)
        launcher_instance.switcher_enter_quicklist()
        self.addCleanup(launcher_instance.switcher_exit_quicklist)
        sleep(.5)
        self.assertThat(launcher_instance.is_quicklist_open(), Equals(True))

    def test_launcher_keynav_can_close_quicklist(self):
        """Tests that we can close a quicklist from keynav mode."""
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        sleep(.5)
        launcher_instance.grab_switcher()
        self.addCleanup(launcher_instance.end_switcher, True)
        sleep(.5)
        launcher_instance.switcher_next()
        sleep(.5)
        launcher_instance.switcher_enter_quicklist()
        sleep(.5)
        launcher_instance.switcher_exit_quicklist()

        self.assertThat(launcher_instance.is_quicklist_open(), Equals(False))
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.launcher.key_nav_is_grabbed, Equals(True))

class LauncherRevealTests(ScenariodLauncherTests):
    """Test the launcher reveal bahavior when in autohide mode."""

    def setUp(self):
        super(LauncherRevealTests, self).setUp()
        self.set_unity_option('launcher_hide_mode', True)
        sleep(1)

    def test_reveal_on_mouse_to_edge(self):
        """Tests reveal of launchers by mouse pressure."""
        launcher_instance = self.get_launcher()
        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.reveal_launcher()
        self.assertThat(launcher_instance.is_showing(), Equals(True))

    def test_reveal_with_mouse_under_launcher(self):
        """Tests that the launcher hides properly if the
        mouse is under the launcher when it is revealed.

        """
        launcher_instance = self.get_launcher()

        launcher_instance.move_mouse_over_launcher()
        launcher_instance.keyboard_reveal_launcher()
        launcher_instance.keyboard_unreveal_launcher()
        sleep(1)
        self.assertThat(launcher_instance.is_showing(), Equals(False))

    def test_reveal_does_not_hide_again(self):
        """Tests reveal of launchers by mouse pressure to ensure it doesn't
        automatically hide again.

        """
        launcher_instance = self.get_launcher()

        launcher_instance.move_mouse_to_right_of_launcher()
        launcher_instance.reveal_launcher()
        sleep(2)
        self.assertThat(launcher_instance.is_showing(), Equals(True))

    def test_launcher_does_not_reveal_with_mouse_down(self):
        """Launcher must not reveal if have mouse button 1 down."""
        launcher_instance = self.get_launcher()
        screens = ScreenGeometry()

        screens.move_mouse_to_monitor(launcher_instance.monitor)
        self.mouse.press(1)
        launcher_instance.reveal_launcher()
        self.assertThat(launcher_instance.is_showing(), Equals(False))
        self.mouse.release(1)

class SoftwareCenterIconTests(ScenariodLauncherTests):
    """ Test integration with Software Center """
    def setUp(self):
        super(SoftwareCenterIconTests, self).setUp()
        sleep(1)

    def test_software_center_add_icon(self):
        """ Test the ability to add a SoftwareCenterLauncherIcon """
        sleep(.5)
        
        launcher_instance = self.get_launcher()
        bus = dbus.SessionBus()
        launcher_object = bus.get_object('com.canonical.Unity.Launcher',
                                      '/com/canonical/Unity/Launcher')
        launcher_iface = dbus.Interface(launcher_object, 'com.canonical.Unity.Launcher')

        # Using Ibus as an example because it's something which is
        # not usually pinned to people's launchers, and still
        # has an icon 
        
        original_num_launcher_icons = self.launcher.model.num_launcher_icons()
        launcher_iface.AddLauncherItemFromPosition("Unity Test",
                                                   "ibus",
                                                   100,
                                                   100,
                                                   32,
                                                   "/usr/share/applications/ibus.desktop",
                                                   "")
        
        sleep(.5)

        icon = self.launcher.model.get_icon_by_desktop_file("/usr/share/applications/ibus.desktop")

        # Check for 2 things:
        #    1) More launcher icons in the end
        #    2) The new launcher icon has a 'Waiting to install' tooltip
        self.assertThat(self.launcher.model.num_launcher_icons() > original_num_launcher_icons,
                        Equals(True))
        self.assertThat(icon[0].tooltip_text == "Waiting to install", Equals(True))
        sleep(.5)
        
        self.addCleanup(launcher_instance.unlock_from_launcher(icon[0]))

