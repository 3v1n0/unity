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
from autopilot.testcase import multiply_scenarios
import logging
from testtools.matchers import Equals, NotEquals
from time import sleep

from unity.emulators.icons import BamfLauncherIcon, ExpoLauncherIcon
from unity.emulators.launcher import IconDragType
from unity.tests.launcher import LauncherTestCase, _make_scenarios

logger = logging.getLogger(__name__)


class LauncherIconsTests(LauncherTestCase):
    """Test the launcher icons interactions"""

    def setUp(self):
        super(LauncherIconsTests, self).setUp()
        self.set_unity_option('launcher_hide_mode', 0)

    def ensure_expo_launcher_icon(self):
        EXPO_URI = "'unity://expo-icon'"
        old_fav = self.call_gsettings_cmd('get', 'com.canonical.Unity.Launcher', 'favorites')

        if not EXPO_URI in old_fav:
            if old_fav[:-2] == "[]":
                new_fav = "["+EXPO_URI+"]"
            else:
                new_fav = old_fav[:-1]+", "+EXPO_URI+"]"

            self.addCleanup(self.call_gsettings_cmd, 'set', 'com.canonical.Unity.Launcher', 'favorites', old_fav)
            self.call_gsettings_cmd('set', 'com.canonical.Unity.Launcher', 'favorites', new_fav)

        icon = self.launcher.model.get_children_by_type(ExpoLauncherIcon)[0]
        self.assertThat(icon, NotEquals(None))
        self.assertThat(icon.visible, Eventually(Equals(True)))

        return icon

    def test_bfb_tooltip_disappear_when_dash_is_opened(self):
         """Tests that the bfb tooltip disappear when the dash is opened."""
         bfb = self.launcher.model.get_bfb_icon()
         self.mouse.move(bfb.center_x, bfb.center_y)

         self.dash.ensure_visible()
         self.addCleanup(self.dash.ensure_hidden)

         self.assertThat(bfb.get_tooltip().active, Eventually(Equals(False)))

    def test_bfb_tooltip_is_disabled_when_dash_is_open(self):
         """Tests the that bfb tooltip is disabled when the dash is open."""
         self.dash.ensure_visible()
         self.addCleanup(self.dash.ensure_hidden)

         bfb = self.launcher.model.get_bfb_icon()
         self.mouse.move(bfb.center_x, bfb.center_y)

         self.assertThat(bfb.get_tooltip().active, Eventually(Equals(False)))

    def test_shift_click_opens_new_application_instance(self):
        """Shift+Clicking MUST open a new instance of an already-running application."""
        app = self.start_app("Calculator")
        icon = self.launcher.model.get_icon(desktop_id=app.desktop_file)
        launcher_instance = self.launcher.get_launcher_for_monitor(0)

        self.keyboard.press("Shift")
        self.addCleanup(self.keyboard.release, "Shift")
        launcher_instance.click_launcher_icon(icon)

        self.assertNumberWinsIsEventually(app, 2)

    def test_launcher_activate_last_focused_window(self):
        """Activating a launcher icon must raise only the last focused instance
        of that application.

        """
        char_win1 = self.start_app_window("Character Map")
        calc_win = self.start_app_window("Calculator")
        char_win2 = self.start_app_window("Character Map")

        self.assertVisibleWindowStack([char_win2, calc_win, char_win1])

        char_icon = self.launcher.model.get_icon(
            desktop_id=char_win2.application.desktop_file)
        calc_icon = self.launcher.model.get_icon(
            desktop_id=calc_win.application.desktop_file)

        self.launcher_instance.click_launcher_icon(calc_icon)
        self.assertProperty(calc_win, is_focused=True)
        self.assertVisibleWindowStack([calc_win, char_win2, char_win1])

        self.launcher_instance.click_launcher_icon(char_icon)
        self.assertProperty(char_win2, is_focused=True)
        self.assertVisibleWindowStack([char_win2, calc_win, char_win1])

        self.keybinding("window/minimize")

        self.assertThat(lambda: char_win2.is_hidden, Eventually(Equals(True)))
        self.assertProperty(calc_win, is_focused=True)
        self.assertVisibleWindowStack([calc_win, char_win1])

        self.launcher_instance.click_launcher_icon(char_icon)
        self.assertProperty(char_win1, is_focused=True)
        self.assertThat(lambda: char_win2.is_hidden, Eventually(Equals(True)))
        self.assertVisibleWindowStack([char_win1, calc_win])

    def test_clicking_icon_twice_initiates_spread(self):
        """This tests shows that when you click on a launcher icon twice,
        when an application window is focused, the spread is initiated.
        """
        char_win1 = self.start_app_window("Character Map")
        char_win2 = self.start_app_window("Character Map")
        char_app = char_win1.application

        self.assertVisibleWindowStack([char_win2, char_win1])
        self.assertProperty(char_win2, is_focused=True)

        char_icon = self.launcher.model.get_icon(desktop_id=char_app.desktop_file)
        self.addCleanup(self.keybinding, "spread/cancel")
        self.launcher_instance.click_launcher_icon(char_icon)

        self.assertThat(self.window_manager.scale_active, Eventually(Equals(True)))
        self.assertThat(self.window_manager.scale_active_for_group, Eventually(Equals(True)))

    def test_while_in_scale_mode_the_dash_will_still_open(self):
        """If scale is initiated through the laucher pressing super must close
        scale and open the dash.
        """
        char_win1 = self.start_app_window("Character Map")
        char_win2 = self.start_app_window("Character Map")
        char_app = char_win1.application

        self.assertVisibleWindowStack([char_win2, char_win1])
        self.assertProperty(char_win2, is_focused=True)

        char_icon = self.launcher.model.get_icon(desktop_id=char_app.desktop_file)
        self.launcher_instance.click_launcher_icon(char_icon)
        self.assertThat(self.window_manager.scale_active, Eventually(Equals(True)))

        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)

        self.assertThat(self.dash.visible, Eventually(Equals(True)))
        self.assertThat(self.window_manager.scale_active, Eventually(Equals(False)))

    def test_icon_shows_on_quick_application_reopen(self):
        """Icons must stay on launcher when an application is quickly closed/reopened."""
        calc = self.start_app("Calculator")
        desktop_file = calc.desktop_file
        calc_icon = self.launcher.model.get_icon(desktop_id=desktop_file)
        self.assertThat(calc_icon.visible, Eventually(Equals(True)))

        self.close_all_app("Calculator")
        calc = self.start_app("Calculator")
        sleep(2)

        calc_icon = self.launcher.model.get_icon(desktop_id=desktop_file)
        self.assertThat(calc_icon, NotEquals(None))
        self.assertThat(calc_icon.visible, Eventually(Equals(True)))

    def test_right_click_on_icon_ends_expo(self):
        """Right click on a launcher icon in expo mode must end the expo
        and show the quicklist.

        """
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled workspaces.")

        self.keybinding("expo/start")
        self.assertThat(self.window_manager.expo_active, Eventually(Equals(True)))
        self.addCleanup(self.keybinding, "expo/cancel")

        bfb = self.launcher.model.get_bfb_icon()
        self.mouse.move(bfb.center_x, bfb.center_y)
        self.mouse.click(button=3)

        self.assertThat(self.launcher_instance.quicklist_open, Eventually(Equals(True)))
        self.assertThat(self.window_manager.expo_active, Eventually(Equals(False)))

    def test_expo_launcher_icon_initiates_expo(self):
        """Clicking on the expo launcher icon must start the expo."""
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled workspaces.")

        expo = self.ensure_expo_launcher_icon()
        self.addCleanup(self.keybinding, "expo/cancel")
        self.launcher_instance.click_launcher_icon(expo)

        self.assertThat(self.window_manager.expo_active, Eventually(Equals(True)))

    def test_expo_launcher_icon_terminates_expo(self):
        """Clicking on the expo launcher icon when expo is active must close it."""
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled workspaces.")

        self.keybinding("expo/start")
        self.assertThat(self.window_manager.expo_active, Eventually(Equals(True)))
        self.addCleanup(self.keybinding, "expo/cancel")

        expo = self.ensure_expo_launcher_icon()
        self.launcher_instance.click_launcher_icon(expo)

        self.assertThat(self.window_manager.expo_active, Eventually(Equals(False)))


class LauncherDragIconsBehavior(LauncherTestCase):
    """Tests dragging icons around the Launcher."""

    scenarios = multiply_scenarios(_make_scenarios(),
                                   [
                                       ('inside', {'drag_type': IconDragType.INSIDE}),
                                       ('outside', {'drag_type': IconDragType.OUTSIDE}),
                                   ])

    def setUp(self):
        super(LauncherDragIconsBehavior, self).setUp()
        self.set_unity_option('launcher_hide_mode', 0)

    def ensure_calc_icon_not_in_launcher(self):
        """Wait until the launcher model updates and removes the calc icon."""
        # Normally we'd use get_icon(desktop_id="...") but we're expecting it to
        # not exist, and we don't want to wait for 10 seconds, so we do this
        # the old fashioned way.
        refresh_fn = lambda: self.launcher.model.get_children_by_type(
            BamfLauncherIcon, desktop_id="gcalctool.desktop")
        self.assertThat(refresh_fn, Eventually(Equals([])))

    def test_can_drag_icon_below_bfb(self):
        """Application icons must be draggable to below the BFB."""

        self.ensure_calc_icon_not_in_launcher()
        calc = self.start_app("Calculator")
        calc_icon = self.launcher.model.get_icon(desktop_id=calc.desktop_file)
        bfb_icon = self.launcher.model.get_bfb_icon()

        self.launcher_instance.drag_icon_to_position(
            calc_icon,
            IconDragType.AFTER,
            bfb_icon,
            self.drag_type)
        moved_icon = self.launcher.model.\
                     get_launcher_icons_for_monitor(self.launcher_monitor)[1]
        self.assertThat(moved_icon.id, Equals(calc_icon.id))

    def test_can_drag_icon_below_window_switcher(self):
        """Application icons must be dragable to below the workspace switcher icon."""

        self.ensure_calc_icon_not_in_launcher()
        calc = self.start_app("Calculator")
        calc_icon = self.launcher.model.get_icon(desktop_id=calc.desktop_file)
        bfb_icon = self.launcher.model.get_bfb_icon()
        trash_icon = self.launcher.model.get_trash_icon()

        # Move a known icon to the top as it needs to be more than 2 icon
        # spaces away for this test to actually do anything
        self.launcher_instance.drag_icon_to_position(
            calc_icon,
            IconDragType.AFTER,
            bfb_icon,
            self.drag_type)

        sleep(1)
        self.launcher_instance.drag_icon_to_position(
            calc_icon,
            IconDragType.BEFORE,
            trash_icon,
            self.drag_type)

        # Must be the last bamf icon - not necessarily the third-from-end icon.
        refresh_fn = lambda: self.launcher.model.get_launcher_icons()[-2].id
        self.assertThat(refresh_fn,
            Eventually(Equals(calc_icon.id)),
            "Launcher icons are: %r" % self.launcher.model.get_launcher_icons())
