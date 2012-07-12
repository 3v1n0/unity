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

from unity.emulators.icons import BamfLauncherIcon
from unity.emulators.launcher import IconDragType
from unity.tests.launcher import LauncherTestCase, _make_scenarios

logger = logging.getLogger(__name__)


class LauncherIconsTests(LauncherTestCase):
    """Test the launcher icons interactions"""

    def setUp(self):
        super(LauncherIconsTests, self).setUp()
        self.set_unity_option('launcher_hide_mode', 0)

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
        mah_win1 = self.start_app_window("Mahjongg")
        calc_win = self.start_app_window("Calculator")
        mah_win2 = self.start_app_window("Mahjongg")

        self.assertVisibleWindowStack([mah_win2, calc_win, mah_win1])

        mahj_icon = self.launcher.model.get_icon(
            desktop_id=mah_win2.application.desktop_file)
        calc_icon = self.launcher.model.get_icon(
            desktop_id=calc_win.application.desktop_file)

        self.launcher_instance.click_launcher_icon(calc_icon)
        self.assertProperty(calc_win, is_focused=True)
        self.assertVisibleWindowStack([calc_win, mah_win2, mah_win1])

        self.launcher_instance.click_launcher_icon(mahj_icon)
        self.assertProperty(mah_win2, is_focused=True)
        self.assertVisibleWindowStack([mah_win2, calc_win, mah_win1])

        self.keybinding("window/minimize")

        self.assertThat(lambda: mah_win2.is_hidden, Eventually(Equals(True)))
        self.assertProperty(calc_win, is_focused=True)
        self.assertVisibleWindowStack([calc_win, mah_win1])

        self.launcher_instance.click_launcher_icon(mahj_icon)
        self.assertProperty(mah_win1, is_focused=True)
        self.assertThat(lambda: mah_win2.is_hidden, Eventually(Equals(True)))
        self.assertVisibleWindowStack([mah_win1, calc_win])

    def test_clicking_icon_twice_initiates_spread(self):
        """This tests shows that when you click on a launcher icon twice,
        when an application window is focused, the spread is initiated.
        """
        calc_win1 = self.start_app_window("Calculator")
        calc_win2 = self.start_app_window("Calculator")
        calc_app = calc_win1.application

        self.assertVisibleWindowStack([calc_win2, calc_win1])
        self.assertProperty(calc_win2, is_focused=True)

        calc_icon = self.launcher.model.get_icon(desktop_id=calc_app.desktop_file)
        self.addCleanup(self.keybinding, "spread/cancel")
        self.launcher_instance.click_launcher_icon(calc_icon)

        self.assertThat(self.window_manager.scale_active, Eventually(Equals(True)))
        self.assertThat(self.window_manager.scale_active_for_group, Eventually(Equals(True)))

    def test_while_in_scale_mode_the_dash_will_still_open(self):
        """If scale is initiated through the laucher pressing super must close
        scale and open the dash.
        """
        calc_win1 = self.start_app_window("Calculator")
        calc_win2 = self.start_app_window("Calculator")
        calc_app = calc_win1.application

        self.assertVisibleWindowStack([calc_win2, calc_win1])
        self.assertProperty(calc_win2, is_focused=True)

        calc_icon = self.launcher.model.get_icon(desktop_id=calc_app.desktop_file)
        self.launcher_instance.click_launcher_icon(calc_icon)

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


class LauncherDragIconsBehavior(LauncherTestCase):
    """Tests dragging icons around the Launcher."""

    scenarios = multiply_scenarios(_make_scenarios(),
                                   [
                                       ('inside', {'drag_type': IconDragType.INSIDE}),
                                       ('outside', {'drag_type': IconDragType.OUTSIDE}),
                                   ])

    def ensure_calc_icon_not_in_launcher(self):
        """Wait until the launcher model updates and removes the calc icon."""
        # Normally we'd use get_icon(desktop_id="...") but we're expecting it to
        # not exist, and we don't want to wait for 10 seconds, so we do this
        # the old fashioned way.
        refresh_fn = lambda: self.launcher.model.get_children_by_type(
            BamfLauncherIcon, desktop_id="gcalctool.desktop")
        self.assertThat(refresh_fn, Eventually(Equals(None)))

    def test_can_drag_icon_below_bfb(self):
        """Application icons must be draggable to below the BFB."""

        self.ensure_calc_icon_not_in_launcher()
        calc = self.start_app("Calculator")
        calc_icon = self.launcher.model.get_icon(desktop_id=calc.desktop_file)

        bfb_icon_position = 0
        self.launcher_instance.drag_icon_to_position(calc_icon,
                                                     bfb_icon_position,
                                                     self.drag_type)
        moved_icon = self.launcher.model.\
                     get_launcher_icons_for_monitor(self.launcher_monitor)[1]
        self.assertThat(moved_icon.id, Equals(calc_icon.id))

    def test_can_drag_icon_above_window_switcher(self):
        """Application icons must be dragable to above the workspace switcher icon."""

        self.ensure_calc_icon_not_in_launcher()
        calc = self.start_app("Calculator")
        calc_icon = self.launcher.model.get_icon(desktop_id=calc.desktop_file)

        # Move a known icon to the top as it needs to be more than 2 icon
        # spaces away for this test to actually do anything
        bfb_icon_position = 0
        self.launcher_instance.drag_icon_to_position(calc_icon,
                                                     bfb_icon_position,
                                                     self.drag_type)
        sleep(1)
        switcher_pos = -2
        self.launcher_instance.drag_icon_to_position(calc_icon,
                                                     switcher_pos,
                                                     self.drag_type)

        moved_icon = self.launcher.model.\
                     get_launcher_icons_for_monitor(self.launcher_monitor)[-3]
        self.assertThat(moved_icon.id, Equals(calc_icon.id))
