# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2013 Canonical
# Authors: Chris Townsend
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
import logging
from testtools.matchers import Equals, GreaterThan, LessThan
from time import sleep

from unity.tests.launcher import LauncherTestCase

logger = logging.getLogger(__name__)

class LauncherScrollTests(LauncherTestCase):
    """Tests for scrolling behavior of the Launcher"""

    def open_apps_in_launcher(self):
        """Opens some apps in order to get icon stacking in the Launcher"""

        # Add some additional applications, since we need a lot of those on big screens
        if "System Monitor" not in self.process_manager.KNOWN_APPS:
            self.process_manager.register_known_application("System Monitor", "gnome-system-monitor.desktop", "gnome-system-monitor")
        if "Archive Manager" not in self.process_manager.KNOWN_APPS:
            self.process_manager.register_known_application("Archive Manager", "file-roller.desktop", "file-roller")

        apps = ("Calculator", "Mahjongg", "Text Editor", "Character Map", "Terminal", "Remmina", "System Monitor", "Archive Manager")
        
        for app in apps:
            self.process_manager.start_app_window(app)
   
    def test_autoscrolling_from_bottom(self):
        """Tests the autoscrolling from the bottom of the Launcher"""
        self.open_apps_in_launcher()

        # Set the autoscroll_offset to 10 (this is arbitrary for this test).
        autoscroll_offset = 10

        launcher_instance = self.get_launcher()
        (x, y, w, h) = launcher_instance.geometry

        icons = self.unity.launcher.model.get_launcher_icons()
        num_icons = self.unity.launcher.model.num_launcher_icons()

        last_icon = icons[num_icons - 1]

        # Move mouse to the middle of the Launcher in order to expand all
        # of the icons for scrolling
        launcher_instance.move_mouse_over_launcher()

        # Make sure the last icon is off the screen or else there is no
        # scrolling.
        self.assertThat(last_icon.center_y, Eventually(GreaterThan(h)))

        # Autoscroll to the last icon
        launcher_instance.move_mouse_to_icon(last_icon, autoscroll_offset)
        
        (x_fin, y_fin) = self.mouse.position()

        # Make sure we ended up in the center of the last icon
        self.assertThat(x_fin, Equals(last_icon.center_x))
        self.assertThat(y_fin, Equals(last_icon.center_y))

    def test_autoscrolling_from_top(self):
        """Test the autoscrolling from the top of the Launcher"""
        self.open_apps_in_launcher()

        # Set the autoscroll_offset to 10 (this is arbitrary for this test).
        autoscroll_offset = 10

        launcher_instance = self.get_launcher()
        (x, y, w, h) = launcher_instance.geometry

        icons = self.unity.launcher.model.get_launcher_icons()
        num_icons = self.unity.launcher.model.num_launcher_icons()

        first_icon = icons[0]
        last_icon = icons[num_icons - 1]

        # Move to the last icon in order to expand the top of the Launcher
        launcher_instance.move_mouse_to_icon(last_icon)

        # Make sure the first icon is off the screen or else there is no
        # scrolling.
        self.assertThat(first_icon.center_y, Eventually(LessThan(y)))
        
        # Autoscroll to the first icon
        launcher_instance.move_mouse_to_icon(first_icon, autoscroll_offset)

        (x_fin, y_fin) = self.mouse.position()

        # Make sure we ended up in the center of the first icon
        self.assertThat(x_fin, Equals(first_icon.center_x))
        self.assertThat(y_fin, Equals(first_icon.center_y))
