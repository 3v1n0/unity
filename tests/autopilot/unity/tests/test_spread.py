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
from unity.emulators.icons import BFBLauncherIcon

from unity.tests import UnityTestCase


class SpreadTests(UnityTestCase):
    """Spread tests"""

    def setUp(self):
        super(SpreadTests, self).setUp()
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

    def assertWindowIsNotScaled(self, window):
        """Assert that a window is not scaled"""
        refresh_fn = lambda: window.id in [w.id for w in self.unity.screen.scaled_windows]
        self.assertThat(refresh_fn, Eventually(Equals(False)))

    def assertWindowIsClosed(self, xid):
        """Assert that a window is not in the list of the open windows"""
        refresh_fn = lambda: xid in [w.x_id for w in self.process_manager.get_open_windows()]
        self.assertThat(refresh_fn, Eventually(Equals(False)))

    def assertLauncherIconsSaturated(self):
        for icon in self.unity.launcher.model.get_launcher_icons():
            self.assertThat(icon.desaturated, Eventually(Equals(False)))

    def assertLauncherIconsDesaturated(self, also_active=True):
        for icon in self.unity.launcher.model.get_launcher_icons():
            if isinstance(icon, BFBLauncherIcon) or (not also_active and icon.active):
                self.assertThat(icon.desaturated, Eventually(Equals(False)))
            else:
                self.assertThat(icon.desaturated, Eventually(Equals(True)))

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

        (x, y, w, h) = target_win.geometry
        self.mouse.move(x + w / 2, y + h / 2)
        sleep(.5)
        self.mouse.click()

        self.assertThat(lambda: not_focused.is_focused, Eventually(Equals(True)))

    def test_scaled_window_closes_on_middle_click(self):
        """Test that a window is closed when middle-clicked in spread"""
        win = self.start_test_application_windows("Calculator", 2)[0]
        self.initiate_spread_for_application(win.application.desktop_file)

        target_xid = win.x_id
        [target_win] = [w for w in self.unity.screen.scaled_windows if w.xid == target_xid]

        (x, y, w, h) = target_win.geometry
        self.mouse.move(x + w / 2, y + h / 2)
        sleep(.5)
        self.mouse.click(button=2)

        self.assertWindowIsNotScaled(target_win)
        self.assertWindowIsClosed(target_xid)

    def test_scaled_window_closes_on_close_button_click(self):
        """Test that a window is closed when its close button is clicked in spread"""
        win = self.start_test_application_windows("Calculator", 1)[0]
        self.initiate_spread_for_screen()

        target_xid = win.x_id
        [target_win] = [w for w in self.unity.screen.scaled_windows if w.xid == target_xid]

        (x, y, w, h) = target_win.scale_close_geometry
        self.mouse.move(x + w / 2, y + h / 2)
        sleep(.5)
        self.mouse.click()

        self.assertWindowIsNotScaled(target_win)
        self.assertWindowIsClosed(target_xid)

    def test_spread_desaturate_launcher_icons(self):
        """Test that the screen spread desaturates the launcher icons"""
        self.start_test_application_windows("Calculator", 1)
        self.initiate_spread_for_screen()
        self.launcher.move_mouse_to_right_of_launcher()
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
        self.mouse.move(icon.center.x, icon.center.y)

        self.assertThat(lambda: icon.get_tooltip(), Eventually(NotEquals(None)))
        self.assertThat(icon.get_tooltip().active, Eventually(Equals(True)))

        self.initiate_spread_for_screen()
        self.assertThat(icon.get_tooltip().active, Eventually(Equals(False)))