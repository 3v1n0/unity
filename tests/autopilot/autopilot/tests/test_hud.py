# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from autopilot.emulators.unity.hud import HudController
from autopilot.emulators.unity.launcher import Launcher
from autopilot.tests import AutopilotTestCase
from autopilot.glibrunner import GlibRunner


class HudTests(AutopilotTestCase):

    run_test_with = GlibRunner

    def setUp(self):
        super(HudTests, self).setUp()
        self.hud_controller = self.get_hud_controller()

    def tearDown(self):
        super(HudTests, self).tearDown()
        self.hud_controller.ensure_visible()

    def get_hud_controller(self):
        controllers = HudController.get_all_instances()
        self.assertEqual(1, len(controllers))
        return controllers[0]

    def get_num_active_launcher_icons(self, launcher):
        num_active = 0
        for icon in launcher.get_launcher_icons():
            if icon.quirk_active:
                num_active += 1
        return num_active

    def test_initially_hidden(self):
        self.assertFalse(self.hud_controller.is_visible())

    def test_reveal_hud(self):
        self.hud_controller.toggle_reveal()
        self.addCleanup(self.hud_controller.toggle_reveal)
        self.assertTrue(self.hud_controller.is_visible())

    def test_slow_tap_not_reveal_hud(self):
        self.hud_controller.toggle_reveal(tap_delay=0.3)
        self.assertFalse(self.hud_controller.is_visible())

    def test_alt_f4_doesnt_show_hud(self):
        self.start_app('Calculator')
        sleep(1)
        # Do a very fast Alt+F4
        self.keyboard.press_and_release("Alt+F4", 0.05)
        self.assertFalse(self.hud_controller.is_visible())

    def test_reveal_hud_with_no_apps(self):
        """Hud must show even with no visible applications."""
        self.keyboard.press_and_release("Ctrl+Alt+d")
        self.addCleanup(self.keyboard.press_and_release, "Ctrl+Alt+d")
        sleep(1)

        self.hud_controller.toggle_reveal()
        sleep(1)
        self.assertTrue(self.hud_controller.is_visible())

        self.hud_controller.toggle_reveal()
        sleep(1)
        self.assertFalse(self.hud_controller.is_visible())

    def test_multiple_hud_reveal_does_not_break_launcher(self):
        """Multiple Hud reveals must not cause the launcher to set multiple
        apps as active.

        """
        launcher = Launcher()

        # We need an app to switch to:
        self.start_app('Character Map')
        # We need an application to play with - I'll use the calculator.
        self.start_app('Calculator')
        sleep(1)

        # before we start, make sure there's only one active icon:
        num_active = self.get_num_active_launcher_icons(launcher)
        self.assertEqual(num_active, 1, "Invalid number of launcher icons active before test has run!")

        # reveal and hide hud several times over:
        for i in range(3):
            self.hud_controller.ensure_visible()
            sleep(0.5)
            self.hud_controller.ensure_hidden()
            sleep(0.5)

        # click application icons for running apps in the launcher:
        icon = launcher.get_icon_by_tooltip_text("Character Map")
        launcher.click_launcher_icon(icon)

        # see how many apps are marked as being active:
        num_active = self.get_num_active_launcher_icons(launcher)
        self.assertLessEqual(num_active, 1, "More than one launcher icon active after test has run!")
