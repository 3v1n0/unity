# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from autopilot.emulators.unity.hud import HudController
from autopilot.emulators.X11 import Keyboard
from autopilot.tests import AutopilotTestCase
from autopilot.glibrunner import GlibRunner


class HudTests(AutopilotTestCase):

    run_test_with = GlibRunner

    def test_reveal_hud_with_no_apps(self):
        """Hud must show even with no visible applications."""
        controllers = HudController.get_all_instances()
        self.assertEqual(1, len(controllers))
        controller = controllers[0]

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
