# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Jacob Edwards,
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from time import sleep

from unity.tests.launcher import LauncherTestCase, _make_scenarios

class LauncherTooltipTests(LauncherTestCase):
    """Tests whether tooltips display only at appropriate times."""

    def test_launcher_tooltip_show(self):
        """Tests whether icon tooltips delay showing themselves and,
        once shown, whether subsequent icons show them instantly."""
        # no tooltips before entering launcher
        self.set_unity_option('launcher_hide_mode', 0)
        self.launcher_instance.move_mouse_to_right_of_launcher()
        icons = self.unity.launcher.model.get_launcher_icons(visible_only=True)
        for i in icons:
            tooltip = i.get_tooltip()
            if not tooltip:
                continue
            self.assertThat(tooltip.active, Eventually(Equals(False)))

        # only reveal tooltips after short wait
        self.assertEqual(self.get_reveal_behavior(icons[0]), self.DELAYED)

        # subsequent tooltips reveal instantly, but hide on exit
        a, b = 0, 1
        while b < len(icons):
            self.mouse.move(icons[b].center_x, icons[b].center_y)
            self.assertTrue(icons[b].get_tooltip().active)
            self.assertFalse(icons[a].get_tooltip().active)
            a, b = a + 1, b + 1
        b -= 1

        # leaving launcher clears tooltips, and instant reveal
        self.launcher_instance.move_mouse_to_right_of_launcher()
        self.assertEqual(self.get_reveal_behavior(icons[b]), self.DELAYED)

    def test_launcher_tooltip_disabling(self):
        """Tests whether clicking on an icon hides its tooltip."""
        self.set_unity_option('launcher_hide_mode', 0)
        self.launcher_instance.move_mouse_to_right_of_launcher()
        icons = self.unity.launcher.model.get_launcher_icons(visible_only=True)
        bfb, other = icons[0], icons[1]
        self.assertEqual(self.get_reveal_behavior(bfb), self.DELAYED)

        # clicking icon hides its launcher until further input
        self.mouse.click()
        self.assertEqual(self.get_reveal_behavior(bfb), self.NEVER)
        self.mouse.click()

        # normal behavior resumes on moving away from icon
        self.assertEqual(self.get_reveal_behavior(other), self.DELAYED)
        self.assertEqual(self.get_reveal_behavior(bfb), self.INSTANT)

    # Tooltip reveal types
    (INSTANT, DELAYED, NEVER) = range(3)

    def get_reveal_behavior(self, icon):
        self.mouse.move(icon.center_x, icon.center_y)
        tooltip = icon.get_tooltip()
        if tooltip and tooltip.active:
            return self.INSTANT
        sleep(1.2)
        tooltip = icon.get_tooltip()
        if tooltip and tooltip.active:
            return self.DELAYED
        return self.NEVER
