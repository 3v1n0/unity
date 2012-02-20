# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from subprocess import call
from time import sleep

from autopilot.emulators.bamf import Bamf
from autopilot.emulators.unity.hud import HudController
from autopilot.emulators.unity.launcher import Launcher
from autopilot.emulators.X11 import Keyboard
from autopilot.tests import AutopilotTestCase
from autopilot.glibrunner import GlibRunner


class HudTests(AutopilotTestCase):

    run_test_with = GlibRunner

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

    def test_reveal_hud_with_no_apps(self):
        """Hud must show even with no visible applications."""
        controller = self.get_hud_controller()

        self.assertFalse(controller.visible)
        kb = Keyboard()
        kb.press_and_release("Ctrl+Alt+d")
        self.addCleanup(kb.press_and_release, "Ctrl+Alt+d")
        sleep(1)

        # we need a *fast* keypress to reveal the hud:
        kb.press_and_release("Alt", delay=0.1)
        sleep(1)
        controller.refresh_state()
        self.assertTrue(controller.visible)

        kb.press_and_release("Alt", delay=0.1)
        sleep(1)
        controller.refresh_state()
        self.assertFalse(controller.visible)

    def test_multiple_hud_reveal_does_not_break_launcher(self):
        """Multiple Hud reveals must not cause the launcher to set multiple
        apps as active.

        """
        hud_controller = self.get_hud_controller()
        launcher = Launcher()
        bamf = Bamf()

        # We need an app to switch to:
        bamf.launch_application("gucharmap.desktop")
        self.addCleanup(call, ["killall", "gucharmap"])

        # We need an application to play with - I'll use the calculator.
        bamf.launch_application("gcalctool.desktop")
        self.addCleanup(call, ["killall", "gcalctool"])

        # before we start, make sure there's only one active icon:
        num_active = self.get_num_active_launcher_icons(launcher)
        self.assertEqual(num_active, 1, "Invalid number of launcher icons active before test has run!")

        # reveal and hide hud several times over:
        for i in range(3):
            hud_controller.ensure_visible()
            sleep(0.5)
            hud_controller.ensure_hidden()
            sleep(0.5)

        # click application icons for running apps in the launcher:
        icon = launcher.get_icon_by_tooltip_text("Character Map")
        launcher.click_launcher_icon(icon)

        # see how many apps are marked as being active:
        num_active = self.get_num_active_launcher_icons(launcher)
        self.assertLessEqual(num_active, 1, "More than one launcher icon active after test has run!")
