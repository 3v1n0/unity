# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2013 Canonical
# Authors: Jacob Edwards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from autopilot.matchers import Eventually
from testtools.matchers import Equals, NotEquals
from time import sleep

from unity.tests.launcher import LauncherTestCase, _make_scenarios

class LauncherTooltipTests(LauncherTestCase):
    """Tests whether tooltips display only at appropriate times."""

    def setUp(self):
        super(LauncherTooltipTests, self).setUp()
        self.set_unity_option('launcher_hide_mode', 0)
        self.launcher_instance.move_mouse_to_right_of_launcher()
        self.icons = self.unity.launcher.model.get_launcher_icons(visible_only=True)

    def test_launcher_tooltip_show(self):
        """Tests whether icon tooltips delay showing themselves and,
        once shown, whether subsequent icons show them instantly."""
        for i in self.icons:
            tooltip = i.get_tooltip()
            if not tooltip:
                continue
            self.assertThat(tooltip.active, Eventually(Equals(False)))

        # only reveal tooltips after short wait
        self.assertEqual(self.get_reveal_behavior(self.icons[0]), self.DELAYED)

        # must avoid the accordion effect, as the icons start to pass to quickly
        size = len(self.icons)
        if size > 5:
          size = 5

        # subsequent tooltips reveal instantly, but hide on exit
        a, b = 0, 1
        while b < size:
            self.mouse.move(self.icons[b].center_x, self.icons[b].center_y)
            self.assertThat(lambda: self.icons[b].get_tooltip(), Eventually(NotEquals(None)))
            self.assertThat(self.icons[b].get_tooltip().active, Eventually(Equals(True)))
            self.assertThat(lambda: self.icons[a].get_tooltip(), Eventually(NotEquals(None)))
            self.assertThat(self.icons[a].get_tooltip().active, Eventually(Equals(False)))
            a, b = a + 1, b + 1
        b -= 1

        # leaving launcher clears tooltips, and instant reveal
        self.launcher_instance.move_mouse_to_right_of_launcher()
        self.assertEqual(self.get_reveal_behavior(self.icons[b]), self.DELAYED)

    def test_launcher_tooltip_disabling(self):
        """Tests whether clicking on an icon hides its tooltip."""
        bfb, other = self.icons[0], self.icons[1]
        self.assertEqual(self.get_reveal_behavior(bfb), self.DELAYED)

        # clicking icon hides its launcher until further input
        self.mouse.click()
        self.assertEqual(self.get_reveal_behavior(bfb), self.NEVER)
        self.mouse.click()

        # normal behavior resumes on moving away from icon
        self.assertEqual(self.get_reveal_behavior(other), self.DELAYED)
        self.assertEqual(self.get_reveal_behavior(bfb), self.INSTANT)

    def test_launcher_bfb_tooltip_when_open(self):
        """Tests whether hovering over the active BFB starts a tooltip timer"""
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)
        bfb, other = self.icons[0], self.icons[1]

        # hovering an open dash's BFB does not show a tooltip ...
        self.assertEqual(self.get_reveal_behavior(bfb), self.NEVER)

        # ... nor did it timeout instant tooltips for other icons
        self.assertEqual(self.get_reveal_behavior(other), self.DELAYED)

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
