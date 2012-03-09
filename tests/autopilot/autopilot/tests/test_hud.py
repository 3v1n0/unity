# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from testtools.matchers import Equals, LessThan

from autopilot.emulators.bamf import Bamf
from autopilot.emulators.unity.hud import HudController
from autopilot.tests import AutopilotTestCase


class HudTests(AutopilotTestCase):

    def setUp(self):
        super(HudTests, self).setUp()
        self.hud = self.get_hud_controller()

    def tearDown(self):
        self.hud.ensure_hidden()
        super(HudTests, self).tearDown()

    def get_hud_controller(self):
        controllers = HudController.get_all_instances()
        self.assertEqual(1, len(controllers))
        return controllers[0]

    def get_num_active_launcher_icons(self):
        num_active = 0
        for icon in self.launcher.model.get_launcher_icons():
            if icon.quirk_active and icon.quirk_visible:
                num_active += 1
        return num_active

    def test_initially_hidden(self):
        self.assertFalse(self.hud.is_visible())

    def test_reveal_hud(self):
        self.hud.toggle_reveal()
        self.assertTrue(self.hud.is_visible())

    def test_no_initial_values(self):
        self.hud.toggle_reveal()
        self.assertThat(self.hud.num_buttons, Equals(0))
        self.assertThat(self.hud.selected_button, Equals(0))

    def test_check_a_values(self):
        self.hud.toggle_reveal()
        self.keyboard.type('a')
        self.assertThat(self.hud.num_buttons, Equals(5))
        self.assertThat(self.hud.selected_button, Equals(1))

    def test_up_down_arrows(self):
        self.hud.toggle_reveal()
        self.keyboard.type('a')
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
        self.assertFalse(self.hud.is_visible())

    def test_alt_f4_doesnt_show_hud(self):
        self.start_app('Calculator')
        sleep(1)
        # Do a very fast Alt+F4
        self.keyboard.press_and_release("Alt+F4", 0.05)
        self.assertFalse(self.hud.is_visible())

    def test_reveal_hud_with_no_apps(self):
        """Hud must show even with no visible applications."""
        self.keyboard.press_and_release("Ctrl+Alt+d")
        self.addCleanup(self.keyboard.press_and_release, "Ctrl+Alt+d")
        sleep(1)

        self.hud.toggle_reveal()
        sleep(1)
        self.assertTrue(self.hud.is_visible())

        self.hud.toggle_reveal()
        sleep(1)
        self.assertFalse(self.hud.is_visible())

    def test_multiple_hud_reveal_does_not_break_launcher(self):
        """Multiple Hud reveals must not cause the launcher to set multiple
        apps as active.

        """
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
        icon = self.launcher.model.get_icon_by_tooltip_text("Character Map")
        self.launcher.get_launcher_for_monitor(0).click_launcher_icon(icon)

        # see how many apps are marked as being active:
        num_active = self.get_num_active_launcher_icons()
        self.assertLessEqual(num_active, 1, "More than one launcher icon active after test has run!")

    def test_restore_focus(self):
        """Ensures that once the hud is dismissed, the same application 
        that was focused before hud invocation is refocused
        """
        b = Bamf();
        app_desktop_file = 'gcalctool.desktop'
        b.launch_application(app_desktop_file)

        # first ensure that the application has started and is focused
        self.assertEqual(b.application_is_focused(app_desktop_file), True)

        self.hud.toggle_reveal()
        sleep(1)
        self.hud.toggle_reveal()
        sleep(1)

        # again ensure that the application we started is focused
        self.assertEqual(b.application_is_focused(app_desktop_file), True)
        
        #test return
        self.hud.toggle_reveal()
        sleep(1)
        
        #test return
        self.hud.toggle_reveal()
        sleep(1)
        self.keyboard.press_and_release('Return')
        sleep(1)

        self.assertEqual(b.application_is_focused(app_desktop_file), True)

