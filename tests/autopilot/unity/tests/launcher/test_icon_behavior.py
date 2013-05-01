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
from testtools.matchers import Equals, NotEquals, GreaterThan
from time import sleep

from unity.emulators.icons import ApplicationLauncherIcon, ExpoLauncherIcon
from unity.emulators.launcher import IconDragType
from unity.tests.launcher import LauncherTestCase, _make_scenarios

from Xlib import Xutil

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

        icon = self.unity.launcher.model.get_children_by_type(ExpoLauncherIcon)[0]
        self.assertThat(icon, NotEquals(None))
        self.assertThat(icon.visible, Eventually(Equals(True)))

        return icon

    def ensure_calculator_in_launcher_and_not_running(self):
        calc = self.process_manager.start_app("Calculator")
        calc_icon = self.unity.launcher.model.get_icon(desktop_id=calc.desktop_file)
        self.addCleanup(self.launcher_instance.unlock_from_launcher, calc_icon)
        self.launcher_instance.lock_to_launcher(calc_icon)
        self.process_manager.close_all_app("Calculator")
        self.assertThat(lambda: self.process_manager.app_is_running("Calculator"), Eventually(Equals(False)))
        return calc_icon

    def test_bfb_tooltip_disappear_when_dash_is_opened(self):
        """Tests that the bfb tooltip disappear when the dash is opened."""
        bfb = self.unity.launcher.model.get_bfb_icon()
        self.mouse.move(bfb.center_x, bfb.center_y)

        self.assertThat(lambda: bfb.get_tooltip(), Eventually(NotEquals(None)))
        self.assertThat(bfb.get_tooltip().active, Eventually(Equals(True)))
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        self.assertThat(bfb.get_tooltip().active, Eventually(Equals(False)))

    def test_bfb_tooltip_is_disabled_when_dash_is_open(self):
        """Tests the that bfb tooltip is disabled when the dash is open."""
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        bfb = self.unity.launcher.model.get_bfb_icon()
        self.mouse.move(bfb.center_x, bfb.center_y)

        # Tooltips are lazy-created  in Unity, so if the BFB tooltip has never
        # been shown before, get_tooltip will return None. If that happens, then
        # this test should pass.
        tooltip = bfb.get_tooltip()
        if tooltip is not None:
            self.assertThat(tooltip.active, Eventually(Equals(False)))

    def test_shift_click_opens_new_application_instance(self):
        """Shift+Clicking MUST open a new instance of an already-running application."""
        app = self.process_manager.start_app("Calculator")
        icon = self.unity.launcher.model.get_icon(desktop_id=app.desktop_file)
        launcher_instance = self.unity.launcher.get_launcher_for_monitor(0)

        self.keyboard.press("Shift")
        self.addCleanup(self.keyboard.release, "Shift")
        launcher_instance.click_launcher_icon(icon)

        self.assertNumberWinsIsEventually(app, 2)

    def test_launcher_activate_last_focused_window(self):
        """Activating a launcher icon must raise only the last focused instance
        of that application.

        """
        char_win1 = self.process_manager.start_app_window("Character Map")
        calc_win = self.process_manager.start_app_window("Calculator")
        char_win2 = self.process_manager.start_app_window("Character Map")

        self.assertVisibleWindowStack([char_win2, calc_win, char_win1])

        char_icon = self.unity.launcher.model.get_icon(
            desktop_id=char_win2.application.desktop_file)
        calc_icon = self.unity.launcher.model.get_icon(
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

    def test_launcher_uses_startup_notification(self):
        """Tests that unity uses startup notification protocol."""
        calc_icon = self.ensure_calculator_in_launcher_and_not_running()
        self.addCleanup(self.process_manager.close_all_app, "Calculator")
        self.launcher_instance.click_launcher_icon(calc_icon)

        calc_app = self.process_manager.get_running_applications_by_desktop_file(calc_icon.desktop_id)[0]
        calc_window = calc_app.get_windows()[0]

        self.assertThat(lambda: self.get_startup_notification_timestamp(calc_window), Eventually(Equals(calc_icon.startup_notification_timestamp)))

    def test_trash_icon_refocus_opened_instance(self):
        """Tests that when the trash is opened, clicking on the icon re-focus the trash again"""
        self.register_nautilus()
        self.addCleanup(self.close_all_windows, "Nautilus")
        self.addCleanup(self.process_manager.close_all_app, "Calculator")
        self.close_all_windows("Nautilus")

        trash_icon = self.unity.launcher.model.get_trash_icon()
        self.launcher_instance.click_launcher_icon(trash_icon)
        self.assertThat(lambda: len(self.get_open_windows_by_application("Nautilus")), Eventually(Equals(1)))
        [trash_window] = self.get_open_windows_by_application("Nautilus")
        self.assertThat(lambda: trash_window.is_focused, Eventually(Equals(True)))

        calc_win = self.process_manager.start_app_window("Calculator")
        self.assertThat(lambda: calc_win.is_focused, Eventually(Equals(True)))
        self.assertThat(lambda: trash_window.is_focused, Eventually(Equals(False)))

        self.launcher_instance.click_launcher_icon(trash_icon)
        self.assertThat(lambda: trash_window.is_focused, Eventually(Equals(True)))

    def test_trash_open_does_not_prevent_nautilus_to_run(self):
        """Tests that when the trash is opened, launching still opens a new window"""
        self.register_nautilus()
        self.addCleanup(self.close_all_windows, "Nautilus")
        self.close_all_windows("Nautilus")

        trash_icon = self.unity.launcher.model.get_trash_icon()
        self.launcher_instance.click_launcher_icon(trash_icon)
        self.assertThat(lambda: len(self.get_open_windows_by_application("Nautilus")), Eventually(Equals(1)))

        nautilus_app = self.get_app_instances("Nautilus")
        nautilus_icon = self.unity.launcher.model.get_icon(desktop_id="nautilus.desktop")
        self.launcher_instance.click_launcher_icon(nautilus_icon)
        self.assertThat(lambda: len(self.get_open_windows_by_application("Nautilus")), Eventually(Equals(2)))

    def test_super_number_shortcut_focuses_new_windows(self):
        """Windows launched using super+number must have
        keyboard focus.

        """
        bfb_icon = self.unity.launcher.model.get_bfb_icon()
        calc_icon = self.ensure_calculator_in_launcher_and_not_running()
        self.addCleanup(self.process_manager.close_all_app, "Calculator")

        self.launcher_instance.drag_icon_to_position(
            calc_icon,
            IconDragType.AFTER,
            bfb_icon)

        self.launcher_instance.keyboard_reveal_launcher()
        self.addCleanup(self.launcher_instance.keyboard_unreveal_launcher)
        self.keyboard.press_and_release("1");

        calc_app = self.process_manager.get_running_applications_by_desktop_file(calc_icon.desktop_id)[0]
        calc_window = calc_app.get_windows()[0]

        self.assertThat(lambda: calc_window.is_focused, Eventually(Equals(True)))

    def test_clicking_icon_twice_initiates_spread(self):
        """This tests shows that when you click on a launcher icon twice,
        when an application window is focused, the spread is initiated.
        """
        char_win1 = self.process_manager.start_app_window("Character Map")
        char_win2 = self.process_manager.start_app_window("Character Map")
        char_app = char_win1.application

        self.assertVisibleWindowStack([char_win2, char_win1])
        self.assertProperty(char_win2, is_focused=True)

        char_icon = self.unity.launcher.model.get_icon(desktop_id=char_app.desktop_file)
        self.addCleanup(self.keybinding, "spread/cancel")
        self.launcher_instance.click_launcher_icon(char_icon)

        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))
        self.assertThat(self.unity.window_manager.scale_active_for_group, Eventually(Equals(True)))

    def test_while_in_scale_mode_the_dash_will_still_open(self):
        """If scale is initiated through the laucher pressing super must close
        scale and open the dash.
        """
        char_win1 = self.process_manager.start_app_window("Character Map")
        char_win2 = self.process_manager.start_app_window("Character Map")
        char_app = char_win1.application

        self.assertVisibleWindowStack([char_win2, char_win1])
        self.assertProperty(char_win2, is_focused=True)

        char_icon = self.unity.launcher.model.get_icon(desktop_id=char_app.desktop_file)
        self.launcher_instance.click_launcher_icon(char_icon)
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))

        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        self.assertThat(self.unity.dash.visible, Eventually(Equals(True)))
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(False)))

    def test_icon_shows_on_quick_application_reopen(self):
        """Icons must stay on launcher when an application is quickly closed/reopened."""
        calc = self.process_manager.start_app("Calculator")
        desktop_file = calc.desktop_file
        calc_icon = self.unity.launcher.model.get_icon(desktop_id=desktop_file)
        self.assertThat(calc_icon.visible, Eventually(Equals(True)))

        self.process_manager.close_all_app("Calculator")
        calc = self.process_manager.start_app("Calculator")
        sleep(2)

        calc_icon = self.unity.launcher.model.get_icon(desktop_id=desktop_file)
        self.assertThat(calc_icon, NotEquals(None))
        self.assertThat(calc_icon.visible, Eventually(Equals(True)))

    def test_right_click_on_icon_ends_expo(self):
        """Right click on a launcher icon in expo mode must end the expo
        and show the quicklist.

        """
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled workspaces.")

        self.keybinding("expo/start")
        self.assertThat(self.unity.window_manager.expo_active, Eventually(Equals(True)))
        self.addCleanup(self.keybinding, "expo/cancel")

        bfb = self.unity.launcher.model.get_bfb_icon()
        self.mouse.move(bfb.center_x, bfb.center_y)
        self.mouse.click(button=3)

        self.assertThat(self.launcher_instance.quicklist_open, Eventually(Equals(True)))
        self.assertThat(self.unity.window_manager.expo_active, Eventually(Equals(False)))

    def test_expo_launcher_icon_initiates_expo(self):
        """Clicking on the expo launcher icon must start the expo."""
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled workspaces.")

        expo = self.ensure_expo_launcher_icon()
        self.addCleanup(self.keybinding, "expo/cancel")
        self.launcher_instance.click_launcher_icon(expo)

        self.assertThat(self.unity.window_manager.expo_active, Eventually(Equals(True)))

    def test_expo_launcher_icon_terminates_expo(self):
        """Clicking on the expo launcher icon when expo is active must close it."""
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled workspaces.")

        self.keybinding("expo/start")
        self.assertThat(self.unity.window_manager.expo_active, Eventually(Equals(True)))
        self.addCleanup(self.keybinding, "expo/cancel")

        expo = self.ensure_expo_launcher_icon()
        self.launcher_instance.click_launcher_icon(expo)

        self.assertThat(self.unity.window_manager.expo_active, Eventually(Equals(False)))

    def test_unminimize_initially_minimized_windows(self):
        """Make sure initially minimized windows can be unminimized."""
        window_spec = {
            "Title": "Hello",
            "Minimized": True
        }

        window = self.launch_test_window(window_spec)
        icon = self.unity.launcher.model.get_icon(desktop_id=window.application.desktop_file)

        self.launcher_instance.click_launcher_icon(icon)
        self.assertThat(lambda: window.x_win.get_wm_state()['state'], Eventually(Equals(Xutil.NormalState)))

    def test_unminimize_minimized_immediately_after_show_windows(self):
        """Make sure minimized-immediately-after-show windows can be unminimized."""
        window_spec = {
            "Title": "Hello",
            "MinimizeImmediatelyAfterShow": True
        }

        window = self.launch_test_window(window_spec)
        icon = self.unity.launcher.model.get_icon(desktop_id=window.application.desktop_file)

        self.launcher_instance.click_launcher_icon(icon)
        self.assertThat(lambda: window.x_win.get_wm_state()['state'], Eventually(Equals(Xutil.NormalState)))

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
        get_icon_fn = lambda: self.unity.launcher.model.get_children_by_type(
            ApplicationLauncherIcon, desktop_id="gcalctool.desktop")
        calc_icon = get_icon_fn()
        if calc_icon:
            self.launcher_instance.unlock_from_launcher(calc_icon[0])

        self.assertThat(get_icon_fn, Eventually(Equals([])))

    def test_can_drag_icon_below_bfb(self):
        """Application icons must be draggable to below the BFB."""

        self.ensure_calc_icon_not_in_launcher()
        calc = self.process_manager.start_app("Calculator")
        calc_icon = self.unity.launcher.model.get_icon(desktop_id=calc.desktop_file)
        bfb_icon = self.unity.launcher.model.get_bfb_icon()

        self.launcher_instance.drag_icon_to_position(
            calc_icon,
            IconDragType.AFTER,
            bfb_icon,
            self.drag_type)
        moved_icon = self.unity.launcher.model.\
                     get_launcher_icons_for_monitor(self.launcher_monitor)[1]
        self.assertThat(moved_icon.id, Equals(calc_icon.id))

    def test_can_drag_icon_below_window_switcher(self):
        """Application icons must be dragable to below the workspace switcher icon."""

        self.ensure_calc_icon_not_in_launcher()
        calc = self.process_manager.start_app("Calculator")
        calc_icon = self.unity.launcher.model.get_icon(desktop_id=calc.desktop_file)
        bfb_icon = self.unity.launcher.model.get_bfb_icon()
        trash_icon = self.unity.launcher.model.get_trash_icon()

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
        refresh_fn = lambda: self.unity.launcher.model.get_launcher_icons()[-2].id
        self.assertThat(refresh_fn,
            Eventually(Equals(calc_icon.id)),
            "Launcher icons are: %r" % self.unity.launcher.model.get_launcher_icons())
