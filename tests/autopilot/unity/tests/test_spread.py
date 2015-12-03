# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.display import Display
from autopilot.matchers import Eventually
from testtools.matchers import Equals, NotEquals
from time import sleep

from unity.tests import UnityTestCase


class SpreadTests(UnityTestCase):
    """Spread tests"""

    def setUp(self):
        super(SpreadTests, self).setUp()
        self.monitor = self.display.get_primary_screen()
        self.launcher = self.unity.launcher.get_launcher_for_monitor(self.display.get_primary_screen())

    def start_test_application_windows(self, app_name, num_windows=2):
        """Start a given number of windows of the requested application"""
        self.process_manager.close_all_app(app_name)
        windows = []

        for i in range(num_windows):
            win = self.process_manager.start_app_window(app_name)
            if windows:
                self.assertThat(win.application, Equals(windows[-1].application))

            windows.append(win)

        self.assertThat(len(windows), Equals(num_windows))

        return windows

    def initiate_spread_for_screen(self):
        """Initiate the Spread for all windows"""
        self.addCleanup(self.unity.window_manager.terminate_spread)
        self.unity.window_manager.initiate_spread()
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))

    def initiate_spread_for_application(self, desktop_id):
        """Initiate the Spread for windows of the given app"""
        icon = self.unity.launcher.model.get_icon(desktop_id=desktop_id)
        self.assertThat(icon, NotEquals(None))

        self.addCleanup(self.unity.window_manager.terminate_spread)
        self.launcher.click_launcher_icon(icon, move_mouse_after=False)
        self.assertThat(self.unity.window_manager.scale_active_for_group, Eventually(Equals(True)))

    def get_spread_filter(self):
        self.assertThat(lambda: self.unity.screen.spread_filter, Eventually(NotEquals(None)))
        return self.unity.screen.spread_filter

    def assertWindowIsScaledEquals(self, xid, is_scaled):
        """Assert weather a window is scaled"""
        def scaled_windows_for_screen_contains_xid():
            """Predicates the window is in the list of scaled windows for the screen.
               The DBus introspection object actually raises an exception if you try to look
               at a window in the scaled_windows list and it's not a scaled window.  Buggzorz.
            """
            scaled_windows = self.unity.screen.scaled_windows
            for w in scaled_windows:
                try:
                    if xid == w.xid:
                        return True
                except:
                    pass
            return False
        self.assertThat(scaled_windows_for_screen_contains_xid, Eventually(Equals(is_scaled)))

    def assertWindowIsClosed(self, xid):
        """Assert that a window is not in the list of the open windows"""
        refresh_fn = lambda: xid in [w.x_id for w in self.process_manager.get_open_windows()]
        self.assertThat(refresh_fn, Eventually(Equals(False)))

    def assertLauncherIconsSaturated(self):
        for icon in self.unity.launcher.model.get_launcher_icons():
            self.assertFalse(icon.monitors_desaturated[self.monitor])

    def assertLauncherIconsDesaturated(self, also_active=True):
        for icon in self.unity.launcher.model.get_launcher_icons():
            if not also_active and icon.active:
                self.assertFalse(icon.monitors_desaturated[self.monitor])
            else:
                self.assertTrue(icon.monitors_desaturated[self.monitor])

    def test_scale_application_windows(self):
        """All the windows of an application must be scaled when application
        spread is initiated

        """
        [win1, win2] = self.start_test_application_windows("Calculator")
        self.initiate_spread_for_application(win1.application.desktop_file)

        self.assertThat(lambda: len(self.unity.screen.scaled_windows), Eventually(Equals(2)))
        self.assertThat(lambda: (win1.x_id and win2.x_id) in [w.xid for w in self.unity.screen.scaled_windows],
                        Eventually(Equals(True)))

    def test_scaled_window_is_focused_on_click(self):
        """Test that a window is focused when clicked in spread"""
        windows = self.start_test_application_windows("Calculator", 3)
        self.initiate_spread_for_application(windows[0].application.desktop_file)

        not_focused = [w for w in windows if not w.is_focused][0]

        target_xid = not_focused.x_id
        [target_win] = [w for w in self.unity.screen.scaled_windows if w.xid == target_xid]

        self.mouse.click_object(target_win, button=1)
        self.assertThat(lambda: not_focused.is_focused, Eventually(Equals(True)))

    def test_scaled_window_closes_on_middle_click(self):
        """Test that a window is closed when middle-clicked in spread"""
        win = self.start_test_application_windows("Calculator", 2)[0]
        self.initiate_spread_for_application(win.application.desktop_file)

        target_xid = win.x_id
        [target_win] = [w for w in self.unity.screen.scaled_windows if w.xid == target_xid]

        sleep(1)
        self.mouse.click_object(target_win, button=2)

        self.assertWindowIsScaledEquals(target_xid, False)
        self.assertWindowIsClosed(target_xid)

    def test_scaled_window_closes_on_close_button_click(self):
        """Test that a window is closed when its close button is clicked in spread"""
        win = self.start_test_application_windows("Calculator", 1)[0]
        self.initiate_spread_for_screen()

        target_xid = win.x_id
        [target_win] = [w for w in self.unity.screen.scaled_windows if w.xid == target_xid]

        # Make sure mouse is over the test window
        self.mouse.move_to_object(target_win)
        self.mouse.click_object(target_win.scale_close_geometry)

        self.assertWindowIsScaledEquals(target_xid, False)
        self.assertWindowIsClosed(target_xid)

    def test_spread_desaturate_launcher_icons(self):
        """Test that the screen spread desaturates the launcher icons"""
        self.start_test_application_windows("Calculator", 1)
        self.initiate_spread_for_screen()
        self.launcher.move_mouse_beside_launcher()
        self.assertLauncherIconsDesaturated()

    def test_spread_saturate_launcher_icons_on_mouse_over(self):
        """Test that the screen spread re-saturates the launcher icons on mouse over"""
        win = self.start_test_application_windows("Calculator", 2)[0]
        self.initiate_spread_for_application(win.application.desktop_file)
        self.launcher.move_mouse_over_launcher()
        self.assertLauncherIconsSaturated()

    def test_app_spread_desaturate_inactive_launcher_icons(self):
        """Test that the app spread desaturates the inactive launcher icons"""
        win = self.start_test_application_windows("Calculator", 2)[0]
        self.initiate_spread_for_application(win.application.desktop_file)
        self.assertLauncherIconsDesaturated(also_active=False)

    def test_app_spread_saturate_launcher_icons_on_mouse_move(self):
        """Test that the app spread re-saturates the launcher icons on mouse move"""
        win = self.start_test_application_windows("Calculator", 2)[0]
        self.initiate_spread_for_application(win.application.desktop_file)
        self.launcher.move_mouse_to_icon(self.unity.launcher.model.get_bfb_icon())
        self.assertLauncherIconsSaturated()

    def test_app_spread_saturate_launcher_icons_on_mouse_over(self):
        """Test that the app spread re-saturates the launcher icons on mouse over"""
        win = self.start_test_application_windows("Calculator", 2)[0]
        self.initiate_spread_for_application(win.application.desktop_file)
        self.launcher.move_mouse_over_launcher()
        self.assertLauncherIconsSaturated()

    def test_app_spread_desaturate_launcher_icons_switching_application(self):
        """Test that the app spread desaturates the launcher icons on mouse over"""
        cal_win = self.start_test_application_windows("Calculator", 2)[0]
        char_win = self.start_test_application_windows("Character Map", 2)[0]
        self.initiate_spread_for_application(char_win.application.desktop_file)
        self.initiate_spread_for_application(cal_win.application.desktop_file)
        self.assertLauncherIconsDesaturated(also_active=False)

    def test_spread_hides_icon_tooltip(self):
        """Tests that the screen spread hides the active tooltip."""
        [win] = self.start_test_application_windows("Calculator", 1)
        icon = self.unity.launcher.model.get_icon(desktop_id=win.application.desktop_file)
        self.launcher.move_mouse_to_icon(icon)

        self.assertThat(lambda: icon.get_tooltip(), Eventually(NotEquals(None)))
        self.assertThat(icon.get_tooltip().active, Eventually(Equals(True)))

        self.initiate_spread_for_screen()
        self.assertThat(icon.get_tooltip(), Equals(None))

    def test_spread_puts_panel_in_overlay_mode(self):
        """Test that the panel is in overlay mode when in spread"""
        self.start_test_application_windows("Calculator", 1)
        self.initiate_spread_for_screen()
        self.assertThat(self.unity.panels.get_active_panel().in_overlay_mode, Eventually(Equals(True)))
        self.unity.window_manager.terminate_spread()
        self.assertThat(self.unity.panels.get_active_panel().in_overlay_mode, Eventually(Equals(False)))

    def test_panel_close_window_button_terminates_spread(self):
        """Test that the panel close window button terminates the spread"""
        self.start_test_application_windows("Calculator", 1)
        self.initiate_spread_for_screen()
        self.unity.panels.get_active_panel().window_buttons.close.mouse_click();
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(False)))

    def test_spread_filter(self):
        """Test spread filter"""
        cal_wins = self.start_test_application_windows("Calculator", 2)
        char_wins = self.start_test_application_windows("Character Map", 2)
        self.initiate_spread_for_screen()
        spread_filter = self.get_spread_filter()
        self.assertThat(spread_filter.visible, Eventually(Equals(False)))

        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.keyboard.type(cal_wins[0].title)
        self.assertThat(spread_filter.visible, Eventually(Equals(True)))
        self.assertThat(spread_filter.search_bar.search_string, Eventually(Equals(cal_wins[0].title)))

        for w in cal_wins + char_wins:
            self.assertWindowIsScaledEquals(w.x_id, (w in cal_wins))

        self.keyboard.press_and_release("Escape")
        self.assertThat(spread_filter.visible, Eventually(Equals(False)))
        self.assertThat(spread_filter.search_bar.search_string, Eventually(Equals("")))

        for w in cal_wins + char_wins:
            self.assertWindowIsScaledEquals(w.x_id, True)
