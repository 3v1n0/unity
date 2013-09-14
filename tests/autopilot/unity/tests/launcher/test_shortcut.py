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

from unity.tests.launcher import LauncherTestCase

logger = logging.getLogger(__name__)


class LauncherShortcutTests(LauncherTestCase):
    """Tests for the shortcut hint window."""

    def setUp(self):
        super(LauncherShortcutTests, self).setUp()
        self.launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(self.launcher_instance.keyboard_unreveal_launcher)
        sleep(2)

    def test_launcher_keyboard_reveal_shows_shortcut_hints(self):
        """Launcher icons must show shortcut hints after revealing with keyboard."""
        self.assertThat(self.launcher_instance.shortcuts_shown, Eventually(Equals(True)))

    def test_launcher_switcher_keeps_shorcuts(self):
        """Initiating launcher switcher after showing shortcuts must not hide shortcuts"""
        self.launcher_instance.switcher_start()
        self.addCleanup(self.launcher_instance.switcher_cancel)

        self.assertThat(self.unity.launcher.key_nav_is_active, Eventually(Equals(True)))
        self.assertThat(self.launcher_instance.shortcuts_shown, Eventually(Equals(True)))

    def test_launcher_switcher_next_keeps_shortcuts(self):
        """Launcher switcher next action must keep shortcuts after they've been shown."""
        self.launcher_instance.switcher_start()
        self.addCleanup(self.launcher_instance.switcher_cancel)
        self.launcher_instance.switcher_next()
        self.assertThat(self.launcher_instance.shortcuts_shown, Eventually(Equals(True)))

    def test_launcher_switcher_prev_keeps_shortcuts(self):
        """Launcher switcher prev action must keep shortcuts after they've been shown."""
        self.launcher_instance.switcher_start()
        self.addCleanup(self.launcher_instance.switcher_cancel)
        self.launcher_instance.switcher_prev()
        self.assertThat(self.launcher_instance.shortcuts_shown, Eventually(Equals(True)))

    def test_tooltip_not_shown(self):
        """Tooltip must not be shown after revealing the launcher with keyboard
        and mouse is not on the launcher.
        """
        self.assertThat(self.launcher_instance.tooltip_shown, Eventually(Equals(False)))
