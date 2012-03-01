# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools.matchers import Equals, LessThan, GreaterThan

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.unity.launcher import Launcher
from autopilot.emulators.unity.shortcut_hint import ShortcutHint
from autopilot.emulators.X11 import ScreenGeometry
from autopilot.glibrunner import GlibRunner

from time import sleep

class ShortcutHintTests(AutopilotTestCase):
    """Test the launcher."""
    run_test_with = GlibRunner

    def setUp(self):
        super(ShortcutHintTests, self).setUp()
        self.shortcut_hint = ShortcutHint()
        self.set_unity_option('shortcut_overlay', True)
        self.DEFAULT_WIDTH = 970;
        self.DEFAULT_HEIGHT = 680;

        screen = ScreenGeometry();
        monitor = screen.get_primary_monitor()
        monitor_geo = screen.get_monitor_geometry(monitor);
        monitor_w = monitor_geo[2];
        monitor_h = monitor_geo[3];
        launcher_width = Launcher().launcher_geometry(monitor)[2];
        panel_heigth = 24 # TODO get it from panel

        if ((monitor_w - launcher_width) <= self.DEFAULT_WIDTH or
            (monitor_h - panel_heigth) <= self.DEFAULT_HEIGHT):
            self.skipTest("This test requires a bigger screen, to show the ShortcutHint")

        sleep(1)

    def test_shortcut_hint_reveal(self):
        """Test that the shortcut hint is shown."""
        sleep(.5)
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)
        sleep(2)

        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

    def test_shortcut_hint_reveal_timeout(self):
        """Test that the shortcut hint is shown when it should."""
        sleep(.5)
        timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)

        sleep(timeout/2.0)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

        sleep(timeout/2.0)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

    def test_shortcut_hint_unreveal(self):
        """Test that the shortcut hint is hidden when it should."""
        sleep(.5)
        self.shortcut_hint.show()
        sleep(self.shortcut_hint.get_show_timeout())
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))
        sleep(.25)

        self.shortcut_hint.hide()
        sleep(.25)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

    def test_shortcut_hint_cancel(self):
        """Test that the shortcut hint is shown when requested."""
        sleep(.5)
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)
        sleep(self.shortcut_hint.get_show_timeout())
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))
        sleep(.25)

        self.shortcut_hint.cancel()
        sleep(.25)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))
        sleep(self.shortcut_hint.get_show_timeout())
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

    def test_shortcut_hint_geometries(self):
        """Test that the shortcut hint has the wanted geometries."""
        sleep(.5)
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)
        sleep(self.shortcut_hint.get_show_timeout())

        (x, y, w, h) = self.shortcut_hint.get_geometry()
        self.assertThat(w, Equals(self.DEFAULT_WIDTH))
        self.assertThat(h, Equals(self.DEFAULT_HEIGHT))
