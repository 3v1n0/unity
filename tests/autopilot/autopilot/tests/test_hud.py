# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Alex Launi,
#         Marco Trevisan
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools.matchers import Equals, LessThan, GreaterThan
from time import sleep

from autopilot.emulators.X11 import ScreenGeometry
from autopilot.emulators.unity.hud import HudController
from autopilot.emulators.unity.icons import BFBLauncherIcon, HudLauncherIcon
from autopilot.tests import AutopilotTestCase
from os import remove

def _make_scenarios():
    screen_geometry = ScreenGeometry()
    num_monitors = screen_geometry.get_num_monitors()

    scenarios = []

    if num_monitors == 1:
        scenarios = [
            ('Single Monitor, Launcher never hide', {'hud_monitor': 0, 'launcher_hide_mode': 0, 'launcher_primary_only': False}),
            ('Single Monitor, Launcher autohide', {'hud_monitor': 0, 'launcher_hide_mode': 1, 'launcher_primary_only': False}),
            ]
    else:
        for i in range(num_monitors):
            scenario_setting = {'hud_monitor': i, 'launcher_hide_mode': 0, 'launcher_primary_only': False}
            scenarios.append(('Monitor %d, Launcher never hide, on all monitors' % (i), scenario_setting))

            scenario_setting = {'hud_monitor': i, 'launcher_hide_mode': 0, 'launcher_primary_only': True}
            scenarios.append(('Monitor %d, Launcher never hide, only on primary monitor' % (i), scenario_setting))

            scenario_setting = {'hud_monitor': i, 'launcher_hide_mode': 1, 'launcher_primary_only': True}
            scenarios.append(('Monitor %d, Launcher autohide, on all monitors' % (i), scenario_setting))

            scenario_setting = {'hud_monitor': i, 'launcher_hide_mode': 1, 'launcher_primary_only': True}
            scenarios.append(('Monitor %d, Launcher autohide, only on primary monitor' % (i), scenario_setting))

    return scenarios

class HudTests(AutopilotTestCase):

    screen_geo = ScreenGeometry()
    scenarios = _make_scenarios()

    def setUp(self):
        super(HudTests, self).setUp()
        self.set_unity_option('launcher_hide_mode', self.launcher_hide_mode)
        self.set_unity_option('num_launchers', int(self.launcher_primary_only))
        self.hud_monitor_is_primary = (self.screen_geo.get_primary_monitor() == self.hud_monitor)
        self.hud_locked = (self.launcher_hide_mode == 0 and (not self.launcher_primary_only or self.hud_monitor_is_primary))
        self.screen_geo.move_mouse_to_monitor(self.hud_monitor)

        sleep(0.5)
        self.hud = self.get_hud_controller()

    def tearDown(self):
        self.hud.ensure_hidden()
        super(HudTests, self).tearDown()

    def get_hud_controller(self):
        controllers = HudController.get_all_instances()
        self.assertEqual(1, len(controllers))
        return controllers[0]

    def get_hud_launcher_icon(self):
        icons = HudLauncherIcon.get_all_instances()
        self.assertEqual(1, len(icons))
        return icons[0]

    def get_num_active_launcher_icons(self):
        num_active = 0
        for icon in self.launcher.model.get_launcher_icons():
            if icon.active and icon.visible:
                num_active += 1
        return num_active

    def test_initially_hidden(self):
        self.assertFalse(self.hud.visible)

    def reveal_hud(self):
        self.hud.toggle_reveal()
        for counter in range(10):
            sleep(1)
            if self.hud.visible:
                break

        self.assertTrue(self.hud.visible, "HUD did not appear.")

    def test_hud_is_on_right_monitor(self):
        """Tests if the hud is shown and fits the monitor where it should be"""
        self.reveal_hud()
        self.assertThat(self.hud_monitor, Equals(self.hud.monitor))
        self.assertTrue(self.screen_geo.is_rect_on_monitor(self.hud.monitor, self.hud.geometry))

    def test_hud_geometries(self):
        """Tests the HUD geometries for the given monitor and status"""
        self.reveal_hud()
        monitor_geo = self.screen_geo.get_monitor_geometry(self.hud_monitor)
        monitor_x = monitor_geo[0]
        monitor_w = monitor_geo[2]
        hud_x = self.hud.geometry[0]
        hud_w = self.hud.geometry[2]

        if self.hud_locked:
            self.assertThat(hud_x, GreaterThan(monitor_x))
            self.assertThat(hud_x, LessThan(monitor_x + monitor_w))
            self.assertThat(hud_w, Equals(monitor_x + monitor_w - hud_x))
        else:
            self.assertThat(hud_x, Equals(monitor_x))
            self.assertThat(hud_w, Equals(monitor_w))

    def test_no_initial_values(self):
        self.reveal_hud()
        self.assertThat(self.hud.num_buttons, Equals(0))
        self.assertThat(self.hud.selected_button, Equals(0))

    def test_check_a_values(self):
        self.reveal_hud()
        self.keyboard.type('a')
        # Give the HUD a second to get values.
        sleep(1)
        self.assertThat(self.hud.num_buttons, Equals(5))
        self.assertThat(self.hud.selected_button, Equals(1))

    def test_up_down_arrows(self):
        self.reveal_hud()
        self.keyboard.type('a')
        # Give the HUD a second to get values.
        sleep(1)
        self.keyboard.press_and_release('Down')
        self.assertThat(self.hud.selected_button, Equals(2))
        self.keyboard.press_and_release('Down')
        self.assertThat(self.hud.selected_button, Equals(3))
        self.keyboard.press_and_release('Down')
        self.assertThat(self.hud.selected_button, Equals(4))
        self.keyboard.press_and_release('Down')
        self.assertThat(self.hud.selected_button, Equals(5))
        # Down again stays on 5.
        self.keyboard.press_and_release('Down')
        self.assertThat(self.hud.selected_button, Equals(5))
        self.keyboard.press_and_release('Up')
        self.assertThat(self.hud.selected_button, Equals(4))
        self.keyboard.press_and_release('Up')
        self.assertThat(self.hud.selected_button, Equals(3))
        self.keyboard.press_and_release('Up')
        self.assertThat(self.hud.selected_button, Equals(2))
        self.keyboard.press_and_release('Up')
        self.assertThat(self.hud.selected_button, Equals(1))
        # Up again stays on 1.
        self.keyboard.press_and_release('Up')
        self.assertThat(self.hud.selected_button, Equals(1))

    def test_slow_tap_not_reveal_hud(self):
        self.hud.toggle_reveal(tap_delay=0.3)
        sleep(1)
        self.assertFalse(self.hud.visible)

    def test_alt_f4_doesnt_show_hud(self):
        self.start_app('Calculator')
        sleep(1)
        # Do a very fast Alt+F4
        self.keyboard.press_and_release("Alt+F4", 0.05)
        sleep(1)
        self.assertFalse(self.hud.visible)

    def test_reveal_hud_with_no_apps(self):
        """Hud must show even with no visible applications."""
        self.keyboard.press_and_release("Ctrl+Alt+d")
        self.addCleanup(self.keyboard.press_and_release, "Ctrl+Alt+d")
        sleep(1)

        self.hud.toggle_reveal()
        sleep(1)
        self.assertTrue(self.hud.visible)

        self.hud.toggle_reveal()
        sleep(1)
        self.assertFalse(self.hud.visible)

    def test_multiple_hud_reveal_does_not_break_launcher(self):
        """Multiple Hud reveals must not cause the launcher to set multiple
        apps as active.

        """
        launcher = self.launcher.get_launcher_for_monitor(self.hud_monitor)

        if not launcher:
            self.skipTest("The monitor %d has not a launcher" % self.hud_monitor)

        # We need an app to switch to:
        self.start_app('Character Map')
        # We need an application to play with - I'll use the calculator.
        self.start_app('Calculator')
        sleep(1)

        # before we start, make sure there's zero or one active icon:
        num_active = self.get_num_active_launcher_icons()
        self.assertThat(num_active, LessThan(2), "Invalid number of launcher icons active before test has run!")

        # reveal and hide hud several times over:
        for i in range(3):
            self.hud.ensure_visible()
            sleep(0.5)
            self.hud.ensure_hidden()
            sleep(0.5)

        # click application icons for running apps in the launcher:
        icon = self.launcher.model.get_icon_by_desktop_id("gucharmap.desktop")
        launcher.click_launcher_icon(icon)

        # see how many apps are marked as being active:
        num_active = self.get_num_active_launcher_icons()
        self.assertLessEqual(num_active, 1, "More than one launcher icon active after test has run!")

    def test_restore_focus(self):
        """Ensures that once the hud is dismissed, the same application
        that was focused before hud invocation is refocused
        """
        self.start_app("Calculator")
        calc = self.get_app_instances("Calculator")
        self.assertThat(len(calc), Equals(1))
        calc = calc[0]

        # first ensure that the application has started and is focused
        self.assertEqual(calc.is_active, True)

        self.hud.toggle_reveal()
        sleep(1)
        self.hud.toggle_reveal()
        sleep(1)

        # again ensure that the application we started is focused
        self.assertEqual(calc.is_active, True)

        #test return
        self.hud.toggle_reveal()
        sleep(1)

        #test return
        self.hud.toggle_reveal()
        sleep(1)
        self.keyboard.press_and_release('Return')
        sleep(1)

        self.assertEqual(calc.is_active, True)

    def test_gedit_undo(self):
        """Test undo in gedit"""
        """Type "0 1" into gedit."""
        """Activate the Hud, type "undo" then enter."""
        """Save the file in gedit and close gedit."""
        """Read the saved file. The content should be "0 "."""

        self.addCleanup(remove, '/tmp/autopilot_gedit_undo_test_temp_file.txt')
        self.start_app('Text Editor', files=['/tmp/autopilot_gedit_undo_test_temp_file.txt'])

        sleep(1)
        self.keyboard.type("0")
        self.keyboard.type(" ")
        self.keyboard.type("1")

        self.hud.toggle_reveal()
        sleep(1)

        self.keyboard.type("undo")
        self.keyboard.press_and_release('Return')
        sleep(1)

        self.keyboard.press_and_release("Ctrl+s")
        sleep(1)

        contents = open("/tmp/autopilot_gedit_undo_test_temp_file.txt").read().strip('\n')
        self.assertEqual("0 ", contents)

    def test_hud_does_not_change_launcher_status(self):
        """Tests if the HUD reveal keeps the launcher in the status it was"""

        launcher = self.launcher.get_launcher_for_monitor(self.hud_monitor)

        if not launcher:
            self.skipTest("The monitor %d has not a launcher" % self.hud_monitor)

        launcher_shows_pre = launcher.is_showing()
        sleep(.25)

        self.reveal_hud()
        sleep(1)

        launcher_shows_post = launcher.is_showing()
        self.assertThat(launcher_shows_pre, Equals(launcher_shows_post))

    def test_hud_is_locked_to_launcher(self):
        """Tests if the HUD is locked to launcher as we expect or not"""
        self.reveal_hud()
        sleep(.25)

        self.assertThat(self.hud.is_locked_to_launcher, Equals(self.hud_locked))

    def test_hud_icon_is_shown(self):
        """Tests that the correct HUD icon is shown"""
        self.reveal_hud()
        sleep(.5)

        hud_icon = self.get_hud_launcher_icon()

        # FIXME this should check self.hud.is_locked_to_launcher
        # but the HUD icon is currently shared between launchers.
        if self.launcher_hide_mode == 0:
            self.assertTrue(hud_icon.visible)
            self.assertFalse(hud_icon.desaturated)

            # FIXME remove this once the issue above has been resolved
            if self.hud.is_locked_to_launcher:
                self.assertFalse(self.hud.show_embedded_icon)
        else:
            self.assertTrue(self.hud.show_embedded_icon)
            self.assertFalse(hud_icon.visible)

    def test_hud_launcher_icon_hides_bfb(self):
        """Tests that the BFB icon is hidden when the HUD launcher icon is shown"""
        if not self.hud.is_locked_to_launcher:
            self.skipTest("This test needs a locked launcher")

        hud_icon = self.get_hud_launcher_icon()

        icons = BFBLauncherIcon.get_all_instances()
        self.assertEqual(1, len(icons))
        bfb_icon = icons[0]

        self.assertTrue(bfb_icon.visible)
        self.assertFalse(hud_icon.visible)
        sleep(.25)

        self.reveal_hud()
        sleep(.5)

        self.assertTrue(hud_icon.visible)
        self.assertFalse(bfb_icon.visible)

    def test_hud_desaturates_launcher_icons(self):
        """Tests that the launcher icons are desaturates when HUD is open"""
        if not self.hud.is_locked_to_launcher:
            self.skipTest("This test needs a locked launcher")

        self.reveal_hud()
        sleep(.5)

        for icon in self.launcher.model.get_launcher_icons():
            if not isinstance(icon, HudLauncherIcon):
                self.assertTrue(icon.desaturated)
