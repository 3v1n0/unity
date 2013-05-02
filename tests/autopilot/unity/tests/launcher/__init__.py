# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Autopilot test case class for all Launcher tests"""

from autopilot.display import Display
from autopilot.testcase import multiply_scenarios

from unity.tests import UnityTestCase
from unity.emulators.X11 import set_primary_monitor


def _make_scenarios():
    """Make scenarios for launcher test cases based on the number of configured
    monitors.
    """
    num_monitors = Display.create().get_num_screens()

    # it doesn't make sense to set only_primary when we're running in a single-monitor setup.
    if num_monitors == 1:
        return [('Single Monitor', {'launcher_monitor': 0, 'only_primary': False})]

    monitor_scenarios = [('Monitor %d' % (i), {'launcher_monitor': i}) for i in range(num_monitors)]
    launcher_mode_scenarios = [('launcher_on_primary', {'only_primary': True}),
                                ('launcher on all', {'only_primary': False})]
    return multiply_scenarios(monitor_scenarios, launcher_mode_scenarios)


class LauncherTestCase(UnityTestCase):
    """A base class for all launcher tests that uses scenarios to run on
    each launcher (for multi-monitor setups).
    """
    scenarios = _make_scenarios()

    def setUp(self):
        super(LauncherTestCase, self).setUp()
        self.set_unity_log_level("unity.launcher", "DEBUG")
        self.addCleanup(self.set_unity_log_level, "unity.launcher", "INFO")

        self.set_unity_option('num_launchers', int(self.only_primary))
        self.launcher_instance = self.get_launcher()

        if self.only_primary:
            try:
                old_primary_screen = self.display.get_primary_screen()
                set_primary_monitor(self.launcher_monitor)
                self.addCleanup(set_primary_monitor, old_primary_screen)
            except autopilot.display.BlacklistedDriverError:
                self.skipTest("Impossible to set the monitor %d as primary" % self.launcher_monitor)

    def get_launcher(self):
        """Get the launcher for the current scenario."""
        return self.unity.launcher.get_launcher_for_monitor(self.launcher_monitor)
