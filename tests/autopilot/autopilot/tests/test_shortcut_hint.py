# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools.matchers import Equals
from time import sleep

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.unity.shortcut_hint import ShortcutController
from autopilot.emulators.X11 import ScreenGeometry


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
        screen = ScreenGeometry();
        monitor = screen.get_primary_monitor()
        monitor_geo = screen.get_monitor_geometry(monitor);
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
        screen = ScreenGeometry();
        monitor = screen.get_primary_monitor()
        return self.launcher.get_launcher_for_monitor(monitor)


class ShortcutHintTests(BaseShortcutHintTests):
    """Test the shortcuthint."""

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


class ShortcutHintInteractionsTests(BaseShortcutHintTests):
    """Test the shortcuthint interactions with other Unity parts."""

    def test_shortcut_hint_hide_using_unity_shortcuts(self):
        """Test that the shortcut hints is hidden pressing unity shortcuts."""
        sleep(.5)
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)
        sleep(self.shortcut_hint.get_show_timeout())

        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))
        self.keybinding_tap("expo/start")
        self.addCleanup(self.keybinding, "expo/cancel")
        sleep(.25)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))
        sleep(.25)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

    def test_launcher_switcher_next_doesnt_show_shortcut_hint(self):
        """Moving forward in launcher switcher must not show the shortcut hint."""
        sleep(.5)
        switcher_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)

        sleep(switcher_timeout * 0.2)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

        self.keybinding("launcher/switcher/next")
        sleep(.25)

        self.keybinding("launcher/switcher/next")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(switcher_timeout * 2)

        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

    def test_launcher_switcher_prev_doesnt_show_shortcut_hint(self):
        """Moving backward in launcher switcher must not show the shortcut hint."""
        sleep(.5)
        switcher_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)

        sleep(switcher_timeout * 0.2)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

        self.keybinding("launcher/switcher/next")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(.25)
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))

        self.keybinding("launcher/switcher/next")
        sleep(.25)

        self.keybinding("launcher/switcher/prev")
        sleep(switcher_timeout)

        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

    def test_launcher_switcher_next_keeps_shortcut_hint(self):
        """Moving forward in launcher switcher after the shortcut hint has been
        shown must keep the shortcuts there.

        """
        sleep(.5)
        show_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)

        sleep(show_timeout)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

        launcher = self.g()
        launcher.start_switcher()
        self.addCleanup(launcher.end_switcher, True)
        sleep(.25)
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))

        launcher.switcher_next()
        sleep(.25)
        launcher.switcher_next()
        sleep(show_timeout)

        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

    def test_launcher_switcher_prev_keeps_shortcut_hint(self):
        """Moving backward in launcher switcher after the shortcut hint has been
        shown must keep the shortcuts there.

        """
        sleep(.5)
        show_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)

        sleep(show_timeout)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

        launcher = self.get_launcher()
        launcher.start_switcher()
        self.addCleanup(launcher.end_switcher, True)
        sleep(.25)
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))

        launcher.switcher_prev()
        sleep(.25)
        launcher.switcher_prev()
        sleep(show_timeout)

        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

    def test_launcher_switcher_cancel_doesnt_hide_shortcut_hint(self):
        """Cancelling the launcher switcher (by Escape) should not hide the
        shortcut hint view.

        """
        sleep(.5)
        show_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)

        sleep(show_timeout)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

        launcher = self.get_launcher()
        launcher.start_switcher()
        self.addCleanup(launcher.end_switcher, True)
        sleep(.25)
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

        launcher.switcher_next()
        sleep(.25)
        self.keyboard.press_and_release("Escape")
        sleep(.25)

        self.assertThat(self.launcher.key_nav_is_active, Equals(False))
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))
        sleep(.5)

        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

    def test_launcher_switcher_and_shortcut_hint_escaping(self):
        """Cancelling the launcher switcher (by Escape) should not hide the
        shortcut hint view, an extra keypress is needed.

        """
        sleep(.5)
        show_timeout = self.shortcut_hint.get_show_timeout()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)

        sleep(show_timeout)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

        launcher = self.get_launcher()
        launcher.start_switcher()
        self.addCleanup(launcher.end_switcher, True)
        sleep(.25)
        self.assertThat(self.launcher.key_nav_is_active(), Equals(True))
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))

        launcher.switcher_next()
        sleep(.25)
        self.keyboard.press_and_release("Escape")
        sleep(.25)

        self.assertThat(self.launcher.key_nav_is_active(), Equals(False))
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))
        sleep(.25)

        self.shortcut_hint.cancel()
        sleep(.25)
        self.assertThat(self.shortcut_hint.is_visible(), Equals(False))

    def test_launcher_icons_hints_show_with_shortcut_hint(self):
        """When the shortcut hint is shown also the launcer's icons hints should
        be shown.

        """
        launcher = self.get_launcher()
        self.shortcut_hint.show()
        self.addCleanup(self.shortcut_hint.hide)
        sleep(self.shortcut_hint.get_show_timeout())


        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))
        self.assertThat(launcher.are_shortcuts_showing(), Equals(True))

    def test_shortcut_hint_shows_with_launcher_icons_hints(self):
        """When the launcher icons hints are shown also the shortcut hint should
        be shown.

        """
        launcher = self.get_launcher()
        launcher.keyboard_reveal_launcher()
        self.addCleanup(launcher.keyboard_unreveal_launcher)
        sleep(1)

        self.assertThat(launcher.are_shortcuts_showing(), Equals(True))
        self.assertThat(self.shortcut_hint.is_visible(), Equals(True))
