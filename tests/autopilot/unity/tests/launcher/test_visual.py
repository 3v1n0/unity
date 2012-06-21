# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards,
#          Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
import logging
from testtools.matchers import Equals
from time import sleep

from unity.emulators.icons import BFBLauncherIcon
from unity.tests.launcher import LauncherTestCase

logger = logging.getLogger(__name__)


class LauncherVisualTests(LauncherTestCase):
    """Tests for visual aspects of the launcher (icon saturation etc.)."""

    def test_keynav_from_dash_saturates_icons(self):
        """Starting super+tab switcher from the dash must resaturate launcher icons.

        Tests fix for bug #913569.
        """
        bfb = self.launcher.model.get_bfb_icon()
        self.mouse.move(bfb.center_x, bfb.center_y)
        self.dash.ensure_visible()
        sleep(1)
        # We can't use 'launcher_instance.switcher_start()' since it moves the mouse.
        self.keybinding_hold_part_then_tap("launcher/switcher")
        self.addCleanup(self.keybinding_release, "launcher/switcher")
        self.addCleanup(self.keybinding, "launcher/switcher/exit")

        self.keybinding_tap("launcher/switcher/next")
        for icon in self.launcher.model.get_launcher_icons():
            self.assertThat(icon.desaturated, Eventually(Equals(False)))

    def test_opening_dash_desaturates_icons(self):
        """Opening the dash must desaturate all the launcher icons."""
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)

        for icon in self.launcher.model.get_launcher_icons():
            if isinstance(icon, BFBLauncherIcon):
                self.assertThat(icon.desaturated, Eventually(Equals(False)))
            else:
                self.assertThat(icon.desaturated, Eventually(Equals(True)))

    def test_opening_dash_with_mouse_over_launcher_keeps_icon_saturation(self):
        """Opening dash with mouse over launcher must not desaturate icons."""
        launcher_instance = self.get_launcher()
        x,y,w,h = launcher_instance.geometry
        self.mouse.move(x + w/2, y + h/2)
        sleep(.5)
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        for icon in self.launcher.model.get_launcher_icons():
            self.assertThat(icon.desaturated, Eventually(Equals(False)))

    def test_mouse_over_with_dash_open_desaturates_icons(self):
        """Moving mouse over launcher with dash open must saturate icons."""
        launcher_instance = self.get_launcher()
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(.5)
        x,y,w,h = launcher_instance.geometry
        self.mouse.move(x + w/2, y + h/2)
        sleep(.5)
        for icon in self.launcher.model.get_launcher_icons():
            self.assertThat(icon.desaturated, Eventually(Equals(False)))
