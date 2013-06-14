# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.display import move_mouse_to_screen
from autopilot.matchers import Eventually
from autopilot.testcase import multiply_scenarios
import logging
from testtools.matchers import Equals, Contains, Not
from time import sleep

from unity.emulators.switcher import SwitcherDirection, SwitcherMode
from unity.tests import UnityTestCase

logger = logging.getLogger(__name__)

class SwitcherTestCase(UnityTestCase):

    scenarios = [
        ('show_desktop_icon_true', {'show_desktop_option': True}),
        ('show_desktop_icon_false', {'show_desktop_option': False}),
    ]

    def setUp(self):
        super(SwitcherTestCase, self).setUp()
        self.set_show_desktop(self.show_desktop_option)

    def set_show_desktop(self, state):
        if type(state) is not bool:
            raise TypeError("'state' must be boolean, not %r" % type(state))
        self.set_unity_option("disable_show_desktop", state)
        self.assertThat(self.unity.switcher.show_desktop_disabled, Eventually(Equals(state)))

    def set_timeout_setting(self, state):
        if type(state) is not bool:
            raise TypeError("'state' must be boolean, not %r" % type(state))
        self.set_unity_option("alt_tab_timeout", state)
        sleep(1)

    def start_applications(self, *args):
        """Start some applications, returning their windows.

        If no applications are specified, the following will be started:
         * Calculator
         * Character Map
         * Character Map

         Windows are always started in the order that they are specified (which
        means the last specified application will *probably* be at the top of the
        window stack after calling this method). Windows are returned in the same
        order they are specified in.

        """
        if len(args) == 0:
            args = ('Calculator', 'Character Map', 'Character Map')
        windows = []
        for app in args:
            windows.append(self.process_manager.start_app_window(app))

        return windows


class SwitcherTests(SwitcherTestCase):
    """Test the switcher."""

    def setUp(self):
        super(SwitcherTests, self).setUp()
        self.set_timeout_setting(False)

    def tearDown(self):
        super(SwitcherTests, self).tearDown()

    def test_switcher_starts_in_normal_mode(self):
        """Switcher must start in normal (i.e.- not details) mode."""
        self.process_manager.start_app("Character Map")

        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)
        self.assertProperty(self.unity.switcher, mode=SwitcherMode.NORMAL)

    def test_label_matches_application_name(self):
        """The switcher label must match the selected application name in normal mode."""
        windows = self.start_applications()
        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        for win in windows:
            app_name = win.application.name
            self.unity.switcher.select_icon(SwitcherDirection.FORWARDS, tooltip_text=app_name)
            self.assertThat(self.unity.switcher.label_visible, Eventually(Equals(True)))
            self.assertThat(self.unity.switcher.label, Eventually(Equals(app_name)))

    def test_application_window_is_fake_decorated(self):
        """When the switcher is in details mode must not show the focused window title."""
        window = self.process_manager.start_app_window("Text Editor")
        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        self.unity.switcher.select_icon(SwitcherDirection.BACKWARDS, tooltip_text=window.application.name)

        self.unity.switcher.show_details()
        self.assertThat(self.unity.switcher.label_visible, Eventually(Equals(False)))
        self.assertThat(self.unity.screen.window(window.x_id).fake_decorated, Eventually(Equals(True)))

    def test_application_window_is_fake_decorated_in_detail_mode(self):
        """Starting switcher in details mode must not show the focused window title."""
        window = self.process_manager.start_app_window("Text Editor")
        self.unity.switcher.initiate(SwitcherMode.DETAIL)
        self.addCleanup(self.unity.switcher.terminate)

        self.assertThat(self.unity.switcher.label_visible, Eventually(Equals(False)))
        self.assertThat(self.unity.screen.window(window.x_id).fake_decorated, Eventually(Equals(True)))

    def test_switcher_move_next(self):
        """Test that pressing the next icon binding moves to the next icon"""
        self.start_applications()
        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        start = self.unity.switcher.selection_index
        self.unity.switcher.next_icon()
        # Allow for wrap-around to first icon in switcher
        next_index = (start + 1) % len(self.unity.switcher.icons)

        self.assertThat(self.unity.switcher.selection_index, Eventually(Equals(next_index)))

    def test_switcher_move_prev(self):
        """Test that pressing the previous icon binding moves to the previous icon"""
        self.start_applications()
        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        start = self.unity.switcher.selection_index
        self.unity.switcher.previous_icon()
        # Allow for wrap-around to last icon in switcher
        prev_index = (start - 1) % len(self.unity.switcher.icons)

        self.assertThat(self.unity.switcher.selection_index, Eventually(Equals(prev_index)))

    def test_switcher_scroll_next(self):
        """Test that scrolling the mouse wheel down moves to the next icon"""
        self.start_applications()
        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        start = self.unity.switcher.selection_index
        self.unity.switcher.next_via_mouse()
        # Allow for wrap-around to first icon in switcher
        next_index = (start + 1) % len(self.unity.switcher.icons)

        self.assertThat(self.unity.switcher.selection_index, Eventually(Equals(next_index)))

    def test_switcher_scroll_prev(self):
        """Test that scrolling the mouse wheel up moves to the previous icon"""
        self.start_applications()
        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        start = self.unity.switcher.selection_index
        self.unity.switcher.previous_via_mouse()

        self.assertThat(self.unity.switcher.selection_index, Eventually(Equals(start - 1)))

    def test_switcher_arrow_key_does_not_init(self):
        """Ensure that Alt+Right does not initiate switcher.

        Regression test for LP:??????

        """
        self.keyboard.press_and_release('Alt+Right')
        self.assertThat(self.unity.switcher.visible, Equals(False))

    def test_lazy_switcher_initiate(self):
        """Inserting a long delay between the Alt press and the Tab tab must still
        open the switcher.

        """
        self.process_manager.start_app("Character Map")

        self.keybinding_hold("switcher/reveal_normal")
        self.addCleanup(self.keybinding_release, "switcher/reveal_normal")
        self.assertThat(self.unity.switcher.visible, Eventually(Equals(False)))
        sleep(5)
        self.keybinding_tap("switcher/reveal_normal")
        self.addCleanup(self.keybinding, "switcher/cancel")
        self.assertThat(self.unity.switcher.visible, Eventually(Equals(True)))

    def test_switcher_cancel(self):
        """Pressing the switcher cancel keystroke must cancel the switcher."""
        self.process_manager.start_app("Character Map")

        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        self.assertThat(self.unity.switcher.visible, Eventually(Equals(True)))
        self.unity.switcher.cancel()
        self.assertThat(self.unity.switcher.visible, Eventually(Equals(False)))

    def test_lazy_switcher_cancel(self):
        """Must be able to cancel the switcher after a 'lazy' initiation."""
        self.process_manager.start_app("Character Map")

        self.keybinding_hold("switcher/reveal_normal")
        self.addCleanup(self.keybinding_release, "switcher/reveal_normal")
        self.assertThat(self.unity.switcher.visible, Eventually(Equals(False)))
        sleep(5)
        self.keybinding_tap("switcher/reveal_normal")
        self.assertThat(self.unity.switcher.visible, Eventually(Equals(True)))
        self.unity.switcher.cancel()
        self.assertThat(self.unity.switcher.visible, Eventually(Equals(False)))

    def test_switcher_appears_on_monitor_with_mouse(self):
        """Tests that the switches appears on the correct monitor.

        This is defined as the monitor with the mouse.

        """
        # TODO - this test fails in multi-monitor setups. You can't use addCleanup
        # a better way would be to have a scenario'd class for multi-monitor
        # switcher tests.
        num_monitors = self.display.get_num_screens()
        if num_monitors == 1:
            self.skip("No point testing this on one monitor")

        charmap, calc, mahjongg = self.start_applications()

        for monitor in range(num_monitors):
            move_mouse_to_screen(monitor)
            self.unity.switcher.initiate()
            self.addCleanup(self.unity.switcher.terminate)
            self.assertThat(self.unity.switcher.monitor, Eventually(Equals(monitor)))

    def test_switcher_alt_f4_is_disabled(self):
        """Tests that alt+f4 does not work while switcher is active."""

        win = self.process_manager.start_app_window("Text Editor")

        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)
        self.assertThat(self.unity.switcher.visible, Eventually(Equals(True)))

        self.keyboard.press_and_release("Alt+F4")
        # Need the sleep to allow the window time to close, for jenkins!
        sleep(10)
        self.assertProperty(win, is_valid=True)


class SwitcherWindowsManagementTests(SwitcherTestCase):
    """Test the switcher window management."""

    def test_switcher_raises_only_last_focused_window(self):
        """Tests that when we do an alt+tab only the previously focused window is raised.

        This is tests by opening 2 Calculators and a Mahjongg.
        Then we do a quick alt+tab twice.
        Then we close the currently focused window.

        """
        char_win1, calc_win, char_win2 = self.start_applications("Character Map", "Calculator", "Character Map")
        self.assertVisibleWindowStack([char_win2, calc_win, char_win1])

        self.keybinding("switcher/reveal_normal")
        self.assertProperty(calc_win, is_focused=True)
        self.assertVisibleWindowStack([calc_win, char_win2, char_win1])

        self.keybinding("switcher/reveal_normal")
        self.assertProperty(char_win2, is_focused=True)
        self.assertVisibleWindowStack([char_win2, calc_win, char_win1])

        self.keybinding("window/close")
        self.assertProperty(calc_win, is_focused=True)
        self.assertVisibleWindowStack([calc_win, char_win1])

    def test_switcher_rises_next_window_of_same_application(self):
        """Tests if alt+tab invoked normally switches to the next application
        window of the same type.

        """
        char_win1, char_win2 = self.start_applications("Character Map", "Character Map")
        self.assertVisibleWindowStack([char_win2, char_win1])

        self.keybinding("switcher/reveal_normal")
        self.assertProperty(char_win1, is_focused=True)

    def test_switcher_rises_other_application(self):
        """Tests if alt+tab invoked normally switches correctly to the other
        application window when the last focused application had 2 windows

        """
        char_win1, char_win2, calc_win = self.start_applications("Character Map", "Character Map", "Calculator")
        self.assertVisibleWindowStack([calc_win, char_win2, char_win1])

        self.keybinding("switcher/reveal_normal")
        self.assertProperty(char_win2, is_focused=True)

        self.keybinding("switcher/reveal_normal")
        self.assertProperty(calc_win, is_focused=True)


class SwitcherDetailsTests(SwitcherTestCase):
    """Test the details mode for the switcher."""

    def setUp(self):
        super(SwitcherDetailsTests, self).setUp()
        self.set_timeout_setting(True)

    def test_details_mode_on_delay(self):
        """Test that details mode activates on a timeout."""
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled more than one workspace.")
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        self.workspace.switch_to((initial_workspace + 1) % self.workspace.num_workspaces)
        self.start_applications("Character Map", "Character Map", "Mahjongg")

        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)
        # Wait longer than details mode.
        sleep(3)
        self.assertProperty(self.unity.switcher, mode=SwitcherMode.DETAIL)

    def test_no_details_for_apps_on_different_workspace(self):
        """Tests that details mode does not initiates when there are multiple windows

        of an application spread across different workspaces.
        Regression test for LP:933406.

        """
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled more than one workspace.")
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        self.process_manager.start_app_window("Character Map")
        self.workspace.switch_to((initial_workspace + 1) % self.workspace.num_workspaces)
        self.start_applications("Character Map", "Mahjongg")

        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)
        # Wait longer than details mode.
        sleep(3)
        self.assertProperty(self.unity.switcher, mode=SwitcherMode.NORMAL)


class SwitcherDetailsModeTests(SwitcherTestCase):
    """Tests for the details mode of the switcher.

    Tests for initiation with both grave (`) and Down arrow.

    """

    scenarios = multiply_scenarios(SwitcherTestCase.scenarios,
      [
          ('initiate_with_grave', {'initiate_keycode': '`'}),
          ('initiate_with_down', {'initiate_keycode': 'Down'}),
      ]
      )

    def setUp(self):
        super(SwitcherDetailsModeTests, self).setUp()
        self.set_timeout_setting(False)

    def test_can_start_details_mode(self):
        """Must be able to switch to details mode using selected scenario keycode.

        """
        self.process_manager.start_app_window("Character Map")
        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        self.keyboard.press_and_release(self.initiate_keycode)

        self.assertProperty(self.unity.switcher, mode=SwitcherMode.DETAIL)

    def test_next_icon_from_last_detail_works(self):
        """Pressing next while showing last switcher item in details mode
        must select first item in the model in non-details mode.

        """
        self.process_manager.start_app("Character Map")
        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)
        while self.unity.switcher.selection_index < len(self.unity.switcher.icons) - 1:
            self.unity.switcher.next_icon()
        self.keyboard.press_and_release(self.initiate_keycode)
        sleep(0.5)
        # Make sure we're at the end of the details list for this icon
        possible_details = self.unity.switcher.detail_current_count - 1
        while self.unity.switcher.detail_selection_index < possible_details:
            self.unity.switcher.next_detail()

        self.unity.switcher.next_icon()
        self.assertThat(self.unity.switcher.selection_index, Eventually(Equals(0)))

    def test_detail_mode_selects_last_active_window(self):
        """The active selection in detail mode must be the last focused window.
        If it was the currently active application type.
        """
        char_win1, char_win2 = self.start_applications("Character Map", "Character Map")
        self.assertVisibleWindowStack([char_win2, char_win1])

        self.unity.switcher.initiate()
        while self.unity.switcher.current_icon.tooltip_text != char_win2.application.name:
            self.unity.switcher.next_icon()
        self.keyboard.press_and_release(self.initiate_keycode)
        sleep(0.5)
        self.unity.switcher.select()

        self.assertProperty(char_win1, is_focused=True)

    def test_detail_mode_selects_third_window(self):
        """Pressing Alt+` twice must select the third last used window.
        LP:1061229
        """
        char_win1, char_win2, char_win3 = self.start_applications("Character Map", "Character Map", "Character Map")
        self.assertVisibleWindowStack([char_win3, char_win2, char_win1])

        self.unity.switcher.initiate(SwitcherMode.DETAIL)
        self.unity.switcher.next_detail()

        self.unity.switcher.select()
        self.assertVisibleWindowStack([char_win1, char_win3, char_win2])


class SwitcherWorkspaceTests(SwitcherTestCase):
    """Test Switcher behavior with respect to multiple workspaces."""

    def setUp(self):
        super(SwitcherWorkspaceTests, self).setUp()
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled more than one workspace.")

    def test_switcher_shows_current_workspace_only(self):
        """Switcher must show apps from the current workspace only."""
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)

        calc = self.process_manager.start_app("Calculator")
        self.workspace.switch_to((initial_workspace + 1) % self.workspace.num_workspaces)
        char_map = self.process_manager.start_app("Character Map")

        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)

        get_icon_names = lambda: [i.tooltip_text for i in self.unity.switcher.icons]
        self.assertThat(get_icon_names, Eventually(Contains(char_map.name)))
        self.assertThat(get_icon_names, Eventually(Not(Contains(calc.name))))

    def test_switcher_all_mode_shows_all_apps(self):
        """Test switcher 'show_all' mode shows apps from all workspaces."""
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)

        calc = self.process_manager.start_app("Calculator")
        self.workspace.switch_to((initial_workspace + 1) % self.workspace.num_workspaces)
        char_map = self.process_manager.start_app("Character Map")

        self.unity.switcher.initiate(SwitcherMode.ALL)
        self.addCleanup(self.unity.switcher.terminate)

        get_icon_names = lambda: [i.tooltip_text for i in self.unity.switcher.icons]
        self.assertThat(get_icon_names, Eventually(Contains(calc.name)))
        self.assertThat(get_icon_names, Eventually(Contains(char_map.name)))

    def test_switcher_can_switch_to_minimised_window(self):
        """Switcher must be able to switch to a minimised window when there's

        another instance of the same application on a different workspace.

        """
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)

        # disable automatic gridding of the switcher after a timeout, since it makes
        # it harder to write the tests.
        self.set_unity_option("alt_tab_timeout", False)

        self.process_manager.start_app("Character Map")

        self.workspace.switch_to((initial_workspace + 1) % self.workspace.num_workspaces)
        char_win2 = self.process_manager.start_app_window("Character Map")
        self.keybinding("window/minimize")
        self.assertProperty(char_win2, is_hidden=True)

        self.process_manager.start_app("Calculator")

        self.unity.switcher.initiate()
        while self.unity.switcher.current_icon.tooltip_text != char_win2.application.name:
            self.unity.switcher.next_icon()
        self.unity.switcher.select()

        self.assertProperty(char_win2, is_hidden=False)

    def test_switcher_is_disabled_when_wall_plugin_active(self):
        """The switcher must not open when the wall plugin is active using ctrl+alt+<direction>."""
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)

        self.workspace.switch_to(0)
        sleep(1)
        self.keyboard.press("Ctrl+Alt+Right")
        self.addCleanup(self.keyboard.release, "Ctrl+Alt+Right")
        sleep(1)
        self.keybinding_hold_part_then_tap("switcher/reveal_normal")
        self.addCleanup(self.unity.switcher.terminate)

        self.assertThat(self.unity.switcher.visible, Eventually(Equals(False)))
