# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Alex Launi,
#         Marco Trevisan
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools.matchers import Equals, NotEquals, LessThan, GreaterThan
from time import sleep

from autopilot.emulators.X11 import ScreenGeometry
from autopilot.emulators.unity.icons import HudLauncherIcon
from autopilot.tests import AutopilotTestCase, multiply_scenarios
from os import remove


def _make_monitor_scenarios():
    num_monitors = ScreenGeometry().get_num_monitors()
    scenarios = []

    if num_monitors == 1:
        scenarios = [('Single Monitor', {'hud_monitor': 0})]
    else:
        for i in range(num_monitors):
            scenarios += [('Monitor %d' % (i), {'hud_monitor': i})]

    return scenarios


class HudTestsBase(AutopilotTestCase):

    def setUp(self):
        super(HudTestsBase, self).setUp()
        sleep(0.25)

    def tearDown(self):
        self.hud.ensure_hidden()
        super(HudTestsBase, self).tearDown()

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

    def reveal_hud(self):
        self.hud.toggle_reveal()
        for counter in range(10):
            sleep(1)
            if self.hud.visible:
                break
        self.assertTrue(self.hud.visible, "HUD did not appear.")


class HudBehaviorTests(HudTestsBase):

    def setUp(self):
        super(HudBehaviorTests, self).setUp()

        self.hud_monitor = self.screen_geo.get_primary_monitor()
        self.screen_geo.move_mouse_to_monitor(self.hud_monitor)

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
        self.start_app('Text Editor', files=['/tmp/autopilot_gedit_undo_test_temp_file.txt'], locale='C')

        sleep(1)
        self.keyboard.type("0")
        self.keyboard.type(" ")
        self.keyboard.type("1")

        self.hud.toggle_reveal()
        sleep(1)

        self.keyboard.type("undo")
        sleep(1)
        self.keyboard.press_and_release('Return')
        sleep(.5)

        self.keyboard.press_and_release("Ctrl+s")
        sleep(1)

        contents = open("/tmp/autopilot_gedit_undo_test_temp_file.txt").read().strip('\n')
        self.assertEqual("0 ", contents)

    def test_disabled_alt_f1(self):
        """This test shows that Alt+F1 mode is disabled for the hud."""
        self.hud.toggle_reveal()

        launcher = self.launcher.get_launcher_for_monitor(0)
        launcher.key_nav_start()

        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_hud_to_dash_disabled_alt_f1(self):
        """When switching from the hud to the dash alt+f1 is disabled."""
        self.hud.toggle_reveal()
        sleep(1)

        self.dash.ensure_visible()

        launcher = self.launcher.get_launcher_for_monitor(0)
        launcher.key_nav_start()

        self.dash.ensure_hidden()
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))

    def test_hud_to_dash_has_key_focus(self):
        """When switching from the hud to the dash you don't lose key focus."""
        self.hud.ensure_visible()
        sleep(1)

        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)

        self.keyboard.type('focus1')

        self.assertEqual(self.dash.search_string, 'focus1')

    def test_dash_to_hud_has_key_focus(self):
        """When switching from the dash to the hud you don't lose key focus."""
        self.dash.ensure_visible()
        self.hud.ensure_visible()
        sleep(1)

        self.keyboard.type('focus2')
       
        self.assertEqual(self.hud.search_string, 'focus2')

    def test_hud_closes_on_workspace_switch(self):
        """This test shows that when you switch to another workspace the hud closes."""
        self.hud.toggle_reveal()
        sleep(1)

        self.workspace.switch_to(1)
        self.workspace.switch_to(2)


class HudLauncherInteractionsTests(HudTestsBase):

    launcher_modes = [('Launcher autohide', {'launcher_autohide': False}),
                      ('Launcher never hide', {'launcher_autohide': True})]

    scenarios = multiply_scenarios(_make_monitor_scenarios(), launcher_modes)

    def setUp(self):
        super(HudLauncherInteractionsTests, self).setUp()
        # Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', int(self.launcher_autohide))

        self.screen_geo.move_mouse_to_monitor(self.hud_monitor)
        sleep(0.5)

    def test_multiple_hud_reveal_does_not_break_launcher(self):
        """Multiple Hud reveals must not cause the launcher to set multiple
        apps as active.

        """
        launcher = self.launcher.get_launcher_for_monitor(self.hud_monitor)

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

    def test_hud_does_not_change_launcher_status(self):
        """Tests if the HUD reveal keeps the launcher in the status it was"""

        launcher = self.launcher.get_launcher_for_monitor(self.hud_monitor)

        launcher_shows_pre = launcher.is_showing()
        sleep(.25)

        self.reveal_hud()
        sleep(1)

        launcher_shows_post = launcher.is_showing()
        self.assertThat(launcher_shows_pre, Equals(launcher_shows_post))


class HudLockedLauncherInteractionsTests(HudTestsBase):

    scenarios = _make_monitor_scenarios()

    def setUp(self):
        super(HudLockedLauncherInteractionsTests, self).setUp()
        # Locked Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', 0)

        self.screen_geo.move_mouse_to_monitor(self.hud_monitor)
        sleep(0.5)

    def test_hud_launcher_icon_hides_bfb(self):
        """Tests that the BFB icon is hidden when the HUD launcher icon is shown"""

        hud_icon = self.get_hud_launcher_icon()
        bfb_icon = self.launcher.model.get_bfb_icon()

        self.assertTrue(bfb_icon.is_visible_on_monitor(self.hud_monitor))
        self.assertFalse(hud_icon.is_visible_on_monitor(self.hud_monitor))
        sleep(.25)

        self.reveal_hud()
        sleep(.5)

        self.assertTrue(hud_icon.is_visible_on_monitor(self.hud_monitor))
        self.assertFalse(bfb_icon.is_visible_on_monitor(self.hud_monitor))

    def test_hud_desaturates_launcher_icons(self):
        """Tests that the launcher icons are desaturates when HUD is open"""

        self.reveal_hud()
        sleep(.5)

        for icon in self.launcher.model.get_launcher_icons_for_monitor(self.hud_monitor):
            if isinstance(icon, HudLauncherIcon):
                self.assertFalse(icon.desaturated)
            else:
                self.assertTrue(icon.desaturated)


class HudVisualTests(HudTestsBase):

    launcher_modes = [('Launcher autohide', {'launcher_autohide': False}),
                      ('Launcher never hide', {'launcher_autohide': True})]

    launcher_screen = [('Launcher on primary monitor', {'launcher_primary_only': False}),
                       ('Launcher on all monitors', {'launcher_primary_only': True})]

    scenarios = multiply_scenarios(_make_monitor_scenarios(), launcher_modes, launcher_screen)

    def setUp(self):
        super(HudVisualTests, self).setUp()
        self.screen_geo.move_mouse_to_monitor(self.hud_monitor)
        self.set_unity_option('launcher_hide_mode', int(self.launcher_autohide))
        self.set_unity_option('num_launchers', int(self.launcher_primary_only))
        self.hud_monitor_is_primary = (self.screen_geo.get_primary_monitor() == self.hud_monitor)
        self.hud_locked = (not self.launcher_autohide and (not self.launcher_primary_only or self.hud_monitor_is_primary))
        sleep(0.5)

    def test_initially_hidden(self):
        self.assertFalse(self.hud.visible)

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

    def test_hud_is_locked_to_launcher(self):
        """Tests if the HUD is locked to launcher as we expect or not"""
        self.reveal_hud()
        sleep(.25)

        self.assertThat(self.hud.is_locked_launcher, Equals(self.hud_locked))

    def test_hud_icon_is_shown(self):
        """Tests that the correct HUD icon is shown"""
        self.reveal_hud()
        self.hud.visible
        sleep(.5)

        hud_launcher_icon = self.get_hud_launcher_icon()
        hud_embedded_icon = self.hud.get_embedded_icon()

        if self.hud.is_locked_launcher:
            self.assertTrue(hud_launcher_icon.is_visible_on_monitor(self.hud_monitor))
            self.assertTrue(hud_launcher_icon.active)
            self.assertThat(hud_launcher_icon.monitor, Equals(self.hud_monitor))
            self.assertFalse(hud_launcher_icon.desaturated)
            self.assertThat(hud_embedded_icon, Equals(None))
        else:
            self.assertFalse(hud_launcher_icon.is_visible_on_monitor(self.hud_monitor))
            self.assertFalse(hud_launcher_icon.active)
            self.assertThat(hud_embedded_icon, NotEquals(None))

    def test_hud_icon_show_the_focused_application_emblem(self):
        """Tests that the correct HUD icon is shown"""
        self.start_app("Calculator")
        calctools = self.get_app_instances("Calculator")
        self.assertThat(len(calctools), GreaterThan(0))
        calc = calctools[0]
        self.assertTrue(calc.is_active)

        self.reveal_hud()
        sleep(.5)

        if self.hud.is_locked_launcher:
            hud_launcher_icon = self.get_hud_launcher_icon()
            self.assertThat(hud_launcher_icon.icon_name, Equals(calc.icon))
        else:
            hud_embedded_icon = self.hud.get_embedded_icon()
            self.assertThat(hud_embedded_icon.icon_name, Equals(calc.icon))
