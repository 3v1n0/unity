# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from time import sleep

from unity.emulators.shortcut_hint import ShortcutController
from unity.tests import UnityTestCase


class BaseShortcutHintTests(UnityTestCase):
    """Base class for the shortcut hint tests"""

    def setUp(self):
        super(BaseShortcutHintTests, self).setUp()

        self.DEFAULT_WIDTH = 970;
        self.DEFAULT_HEIGHT = 680;

        #self.shortcut_hint = self.get_shortcut_controller()
        self.set_unity_option('shortcut_overlay', True)
        self.set_unity_log_level("unity.shell.compiz", "DEBUG")
        self.skip_if_monitor_too_small()
        sleep(1)

    def skip_if_monitor_too_small(self):
        monitor = self.display.get_primary_screen()
        monitor_geo = self.display.get_screen_geometry(monitor)
        monitor_w = monitor_geo[2]
        monitor_h = monitor_geo[3]
        launcher_width = self.unity.launcher.get_launcher_for_monitor(monitor).geometry[2]
        panel_height = self.unity.panels.get_panel_for_monitor(monitor).geometry[3]

        if ((monitor_w - launcher_width) <= self.DEFAULT_WIDTH or
            (monitor_h - panel_height) <= self.DEFAULT_HEIGHT):
            self.skipTest("This test requires a bigger screen, to show the ShortcutHint")

    # def get_shortcut_controller(self):
    #     controllers = ShortcutController.get_all_instances()
    #     self.assertThat(len(controllers), Equals(1))
    #     return controllers[0]

    def get_launcher(self):
        # We could parameterise this so all tests run on both monitors (if MM is
        # set up), but I think it's fine to just always use monitor primary monitor:
        monitor = self.display.get_primary_screen()
        return self.unity.launcher.get_launcher_for_monitor(monitor)


class ShortcutHintTests(BaseShortcutHintTests):
    """Tests for the shortcut hint functionality in isolation."""

    def test_shortcut_hint_reveal(self):
        """Test that the shortcut hint is shown."""
        self.unity.shortcut_hint.show()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)
        self.assertThat(self.unity.shortcut_hint.visible, Eventually(Equals(True)))

    def test_shortcut_hint_reveal_timeout(self):
        """Shortcut hint must be shown after a sufficient timeout."""
        timeout = self.unity.shortcut_hint.get_show_timeout()
        self.unity.shortcut_hint.show()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)

        sleep(timeout/2.0)
        self.assertThat(self.unity.shortcut_hint.visible, Equals(False))
        # This should happen after 3/4 of 'timeout':
        self.assertThat(self.unity.shortcut_hint.visible, Eventually(Equals(True)))

    def test_shortcut_hint_unreveal(self):
        """Shortcut hint must hide when keys are released."""
        self.unity.shortcut_hint.ensure_visible()
        self.unity.shortcut_hint.hide()
        self.assertThat(self.unity.shortcut_hint.visible, Eventually(Equals(False)))

    def test_shortcut_hint_cancel(self):
        """Shortcut hint must hide when cancelled."""
        self.unity.shortcut_hint.ensure_visible()
        self.unity.shortcut_hint.cancel()
        self.assertThat(self.unity.shortcut_hint.visible, Eventually(Equals(False)))

    def test_shortcut_hint_no_blur(self):
        """"""
        self.unity.shortcut_hint.ensure_visible()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)

        self.assertThat(self.unity.shortcut_hint.get_shortcut_view().bg_texture_is_valid, Eventually(Equals(True)))


class ShortcutHintInteractionsTests(BaseShortcutHintTests):
    """Test the shortcuthint interactions with other Unity parts."""

    def test_shortcut_hint_hide_using_unity_shortcuts(self):
        """Unity shortcuts (like expo) must hide the shortcut hint."""
        self.unity.shortcut_hint.ensure_visible()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)

        self.keybinding_tap("expo/start")
        self.addCleanup(self.keybinding, "expo/cancel")

    def test_shortcut_hint_hide_pressing_modifiers(self):
        """Pressing a modifer key must hide the shortcut hint."""
        self.unity.shortcut_hint.ensure_visible()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)

        self.keyboard.press('Control')

        self.assertThat(self.unity.shortcut_hint.visible, Eventually(Equals(False)))

    def test_launcher_switcher_next_doesnt_show_shortcut_hint(self):
        """Super+Tab switcher cycling forward must not show shortcut hint."""
        switcher_timeout = self.unity.shortcut_hint.get_show_timeout()
        self.unity.shortcut_hint.show()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)

        self.keybinding("launcher/switcher/next")
        self.keybinding("launcher/switcher/next")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(switcher_timeout * 2)

        self.assertThat(self.unity.shortcut_hint.visible, Equals(False))

    def test_launcher_switcher_prev_doesnt_show_shortcut_hint(self):
        """Super+Tab switcher cycling backwards must not show shortcut hint."""
        switcher_timeout = self.unity.shortcut_hint.get_show_timeout()
        self.unity.shortcut_hint.show()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)

        self.keybinding("launcher/switcher/next")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.keybinding("launcher/switcher/next")
        self.keybinding("launcher/switcher/prev")
        sleep(switcher_timeout * 2)

        self.assertThat(self.unity.shortcut_hint.visible, Equals(False))

    def test_launcher_icons_hints_show_with_shortcut_hint(self):
        """When the shortcut hint is shown also the launcer's icons hints should
        be shown.

        """
        launcher = self.get_launcher()
        self.unity.shortcut_hint.ensure_visible()
        self.addCleanup(self.unity.shortcut_hint.ensure_hidden)

        self.assertThat(self.unity.shortcut_hint.visible, Equals(True))
        self.assertThat(launcher.shortcuts_shown, Equals(True))

    def test_shortcut_hint_shows_with_launcher_icons_hints(self):
        """When the launcher icons hints are shown also the shortcut hint should
        be shown.

        """
        launcher = self.get_launcher()
        launcher.keyboard_reveal_launcher()
        self.addCleanup(launcher.keyboard_unreveal_launcher)

        self.assertThat(launcher.shortcuts_shown, Eventually(Equals(True)))
        self.assertThat(self.unity.shortcut_hint.visible, Eventually(Equals(True)))
