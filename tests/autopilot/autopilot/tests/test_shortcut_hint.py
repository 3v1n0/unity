# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools.matchers import Equals
from time import sleep

from autopilot.emulators.unity.shortcut_hint import ShortcutController
from autopilot.matchers import Eventually
from autopilot.tests import AutopilotTestCase



class BaseShortcutHintTests(AutopilotTestCase):
    """Base class for the shortcut hint tests"""

    def setUp(self):
        super(BaseShortcutHintTests, self).setUp()

        self.DEFAULT_WIDTH = 970;
        self.DEFAULT_HEIGHT = 680;

        self.shortcut_hint = self.get_shortcut_controller()
        self.set_unity_option('shortcut_overlay', True)
        self.skip_if_monitor_too_small()
        sleep(1)

    def skip_if_monitor_too_small(self):
        monitor = self.screen_geo.get_primary_monitor()
        monitor_geo = self.screen_geo.get_monitor_geometry(monitor);
        monitor_w = monitor_geo[2];
        monitor_h = monitor_geo[3];
        launcher_width = self.launcher.get_launcher_for_monitor(monitor).geometry[2];
        panel_height = 24 # TODO get it from panel

        if ((monitor_w - launcher_width) <= self.DEFAULT_WIDTH or
            (monitor_h - panel_height) <= self.DEFAULT_HEIGHT):
            self.skipTest("This test requires a bigger screen, to show the ShortcutHint")

    def get_shortcut_controller(self):
        controllers = ShortcutController.get_all_instances()
        self.assertThat(len(controllers), Equals(1))
        return controllers[0]

    def get_launcher(self):
        # We could parameterise this so all tests run on both monitors (if MM is
        # set up), but I think it's fine to just always use monitor primary monitor:
        monitor = self.screen_geo.get_primary_monitor()
        return self.launcher.get_launcher_for_monitor(monitor)


class ShortcutHintTests(BaseShortcutHintTests):
    """Tests for the shortcut hint functionality in isolation."""

    def test_shortcut_hint_reveal(self):
        """Test that the shortcut hint is shown."""
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.ensure_hidden)
        self.assertThat(self.shortcut_hint.visible, Eventually(Equals(True)))

    def test_shortcut_hint_reveal_timeout(self):
        """Shortcut hint must be shown after a sufficient timeout."""
        timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.ensure_hidden)

        sleep(timeout/2.0)
        self.assertThat(self.shortcut_hint.visible, Equals(False))
        # This should happen after 3/4 of 'timeout':
        self.assertThat(self.shortcut_hint.visible, Eventually(Equals(True)))

    def test_shortcut_hint_unreveal(self):
        """Shortcut hint must hide when keys are released."""
        self.shortcut_hint.ensure_visible()
        self.shortcut_hint.hide()
        self.assertThat(self.shortcut_hint.visible, Eventually(Equals(False)))

    def test_shortcut_hint_cancel(self):
        """Shortcut hint must hide when cancelled."""
        self.shortcut_hint.ensure_visible()
        self.shortcut_hint.cancel()
        self.assertThat(self.shortcut_hint.visible, Eventually(Equals(False)))


class ShortcutHintInteractionsTests(BaseShortcutHintTests):
    """Test the shortcuthint interactions with other Unity parts."""

    def test_shortcut_hint_hide_using_unity_shortcuts(self):
        """Unity shortcuts (like expo) must hide the shortcut hint."""
        self.shortcut_hint.ensure_visible()
        self.addCleanup(self.shortcut_hint.ensure_hidden)

        self.keybinding_tap("expo/start")
        self.addCleanup(self.keybinding, "expo/cancel")

        self.assertThat(self.shortcut_hint.visible, Eventually(Equals(False)))

    def test_launcher_switcher_next_doesnt_show_shortcut_hint(self):
        """Super+Tab switcher cycling forward must not show shortcut hint."""
        switcher_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.ensure_hidden)

        self.keybinding("launcher/switcher/next")
        self.keybinding("launcher/switcher/next")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(switcher_timeout * 2)

        self.assertThat(self.shortcut_hint.visible, Equals(False))

    def test_launcher_switcher_prev_doesnt_show_shortcut_hint(self):
        """Super+Tab switcher cycling backwards must not show shortcut hint."""
        switcher_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.ensure_hidden)

        self.keybinding("launcher/switcher/next")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.keybinding("launcher/switcher/next")
        self.keybinding("launcher/switcher/prev")
        sleep(switcher_timeout * 2)

        self.assertThat(self.shortcut_hint.visible, Equals(False))

    def test_launcher_switcher_next_keeps_shortcut_hint(self):
        """Super+Tab switcher cycling forwards must not dispel an already-showing
        shortcut hint.

        """
        show_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.ensure_visible()
        self.addCleanup(self.shortcut_hint.ensure_hidden)

        self.keybinding("launcher/switcher/next")
        self.addCleanup(self.keyboard.press_and_release, "Escape")

        self.assertThat(self.launcher.key_nav_is_active, Equals(True))

        self.keybinding("launcher/switcher/next")
        sleep(show_timeout * 2)
        self.assertThat(self.shortcut_hint.visible, Equals(True))

    def test_launcher_switcher_prev_keeps_shortcut_hint(self):
        """Super+Tab switcher cycling backwards must not dispel an already-showing
        shortcut hint.

        """
        show_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.ensure_visible()
        self.addCleanup(self.shortcut_hint.ensure_visible)

        self.keybinding("launcher/switcher/next")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))

        self.keybinding("launcher/switcher/prev")
        self.keybinding("launcher/switcher/prev")
        sleep(show_timeout * 2)
        self.assertThat(self.shortcut_hint.visible, Equals(True))

    def test_launcher_switcher_cancel_doesnt_hide_shortcut_hint(self):
        """Cancelling the launcher switcher (by Escape) must not hide the
        shortcut hint view.

        """
        self.shortcut_hint.ensure_visible()
        self.addCleanup(self.shortcut_hint.ensure_hidden)

        self.keybinding("launcher/switcher/next")
        # NOTE: This will generate an extra Escape keypress if the test passes,
        # but that's better than the alternative...
        self.addCleanup(self.keybinding, "launcher/switcher/exit")

        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(True)))
        self.assertThat(self.shortcut_hint.visible, Equals(True))

        self.keyboard.press_and_release("Escape")

        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(False)))
        self.assertThat(self.shortcut_hint.visible, Equals(True))

    def test_launcher_switcher_and_shortcut_hint_escaping(self):
        """Cancelling the launcher switcher (by Escape) should not hide the
        shortcut hint view, an extra keypress is needed.

        """
        self.shortcut_hint.ensure_visible()
        self.addCleanup(self.shortcut_hint.ensure_hidden)

        self.keybinding("launcher/switcher/next")

        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(True)))
        self.assertThat(self.shortcut_hint.visible, Equals(True))

        self.keyboard.press_and_release("Escape")

        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(False)))
        self.assertThat(self.shortcut_hint.visible, Equals(True))

        self.keyboard.press_and_release("Escape")
        self.assertThat(self.shortcut_hint.visible, Eventually(Equals(False)))

    def test_launcher_icons_hints_show_with_shortcut_hint(self):
        """When the shortcut hint is shown also the launcer's icons hints should
        be shown.

        """
        launcher = self.get_launcher()
        self.shortcut_hint.ensure_visible()
        self.addCleanup(self.shortcut_hint.ensure_hidden)

        self.assertThat(self.shortcut_hint.visible, Equals(True))
        self.assertThat(launcher.shortcuts_shown, Equals(True))

    def test_shortcut_hint_shows_with_launcher_icons_hints(self):
        """When the launcher icons hints are shown also the shortcut hint should
        be shown.

        """
        launcher = self.get_launcher()
        launcher.keyboard_reveal_launcher()
        self.addCleanup(launcher.keyboard_unreveal_launcher)

        self.assertThat(launcher.shortcuts_shown, Eventually(Equals(True)))
        self.assertThat(self.shortcut_hint.visible, Eventually(Equals(True)))
