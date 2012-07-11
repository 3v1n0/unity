# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
import logging
from testtools.matchers import Equals, Contains, Not
from time import sleep

from unity.emulators.switcher import SwitcherMode
from unity.tests import UnityTestCase

logger = logging.getLogger(__name__)

class SwitcherTestCase(UnityTestCase):
    def set_timeout_setting(self, state):
        if type(state) is not bool:
            raise TypeError("'state' must be boolean, not %r" % type(state))
        self.set_unity_option("alt_tab_timeout", state)
        sleep(1)


class SwitcherTests(SwitcherTestCase):
    """Test the switcher."""

    def setUp(self):
        super(SwitcherTests, self).setUp()
        self.set_timeout_setting(False)
        self.char_map = self.start_app('Character Map')
        self.calc = self.start_app('Calculator')
        self.mahjongg = self.start_app('Mahjongg')

    def tearDown(self):
        super(SwitcherTests, self).tearDown()

    def test_witcher_starts_in_normal_mode(self):
        """Switcher must start in normal (i.e.- not details) mode."""
        self.start_app("Character Map")
        sleep(1)

        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        self.assertThat(self.switcher.mode, Equals(SwitcherMode.NORMAL))

    def test_first_detail_mode_has_correct_label(self):
        """Starting switcher in details mode must show the focused window title."""
        app = self.start_app("Text Editor")
        sleep(1)

        [title] = [w.title for w in app.get_windows() if w.is_focused]
        self.switcher.initiate(SwitcherMode.DETAIL)
        self.addCleanup(self.switcher.terminate)

        self.assertThat(self.switcher.controller.view.label, Eventually(Equals(title)))

    def test_switcher_move_next(self):
        """Test that pressing the next icon binding moves to the next icon"""
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)

        start = self.switcher.selection_index
        self.switcher.next_icon()
        self.assertThat(self.switcher.selection_index, Equals(start + 1))

    def test_switcher_move_prev(self):
        """Test that pressing the previous icon binding moves to the previous icon"""
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)

        start = self.switcher.selection_index
        self.switcher.previous_icon()
        self.assertThat(self.switcher.selection_index, Equals(start - 1))

    def test_switcher_scroll_next(self):
        """Test that scrolling the mouse wheel down moves to the next icon"""
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)

        start = self.switcher.selection_index
        self.switcher.next_via_mouse()

        self.assertThat(self.switcher.selection_index, Equals(start + 1))

    def test_switcher_scroll_prev(self):
        """Test that scrolling the mouse wheel up moves to the previous icon"""
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)

        start = self.switcher.selection_index
        self.switcher.previous_via_mouse()

        end = self.switcher.selection_index
        self.assertThat(end, Equals(start - 1))

        self.switcher.terminate()

    def test_switcher_arrow_key_does_not_init(self):
        """Ensure that Alt+Right does not initiate switcher.

        Regression test for LP:??????

        """
        self.keyboard.press('Alt')
        self.addCleanup(self.keyboard.release, 'Alt')
        self.keyboard.press_and_release('Right')
        self.assertThat(self.switcher.visible, Equals(False))

    def test_lazy_switcher_initiate(self):
        self.keybinding_hold("switcher/reveal_normal")
        self.addCleanup(self.keybinding_release, "switcher/reveal_normal")
        self.assertThat(self.switcher.visible, Eventually(Equals(False)))

        self.keybinding_tap("switcher/reveal_normal")
        self.addCleanup(self.keybinding, "switcher/cancel")
        self.assertThat(self.switcher.visible, Eventually(Equals(True)))

    def test_switcher_cancel(self):
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)

        self.assertThat(self.switcher.visible, Eventually(Equals(True)))
        self.switcher.cancel()
        self.assertThat(self.switcher.visible, Eventually(Equals(False)))

    def test_lazy_switcher_cancel(self):
        self.keybinding_hold("switcher/reveal_normal")
        self.addCleanup(self.keybinding_release, "switcher/reveal_normal")
        self.assertThat(self.switcher.visible, Eventually(Equals(False)))
        self.keybinding_tap("switcher/reveal_normal")
        self.assertThat(self.switcher.visible, Eventually(Equals(True)))
        self.switcher.cancel()
        self.assertThat(self.switcher.visible, Eventually(Equals(False)))

    def test_switcher_appears_on_monitor_with_focused_window(self):
        """Tests that the switches appears on the correct monitor.

        This is defined as the monitor with a focused window.

        """
        num_monitors = self.screen_geo.get_num_monitors()
        if num_monitors == 1:
            self.skip("No point testing this on one monitor")

        [calc_win] = self.calc.get_windows()
        for monitor in range(num_monitors):
            self.screen_geo.drag_window_to_monitor(calc_win, monitor)
            self.switcher.initiate()
            self.addCleanup(self.switcher.terminate)
            self.assertThat(self.switcher.controller.monitor, Eventually(Equals(monitor)))

    def test_switcher_alt_f4_is_disabled(self):
        """Tests that alt+f4 does not work while switcher is active."""

        app = self.start_app("Text Editor")
        sleep(1)

        self.switcher.initiate(SwitcherMode.DETAIL)
        self.addCleanup(self.switcher.terminate)

        self.keyboard.press_and_release("Alt+F4")
        [win] = [w for w in app.get_windows()]

        # Need the sleep to allow the window time to close, for jenkins!
        sleep(10)
        self.assertThat(win.is_valid, Equals(True))


class SwitcherWindowsManagementTests(SwitcherTestCase):
    """Test the switcher window management."""

    def test_switcher_raises_only_last_focused_window(self):
        """Tests that when we do an alt+tab only the previously focused window is raised.

        This is tests by opening 2 Calculators and a Mahjongg.
        Then we do a quick alt+tab twice.
        Then we close the currently focused window.

        """
        mah_win1 = self.start_app_window("Mahjongg")
        calc_win = self.start_app_window("Calculator")
        mah_win2 = self.start_app_window("Mahjongg")

        self.assertVisibleWindowStack([mah_win2, calc_win, mah_win1])

        self.keybinding("switcher/reveal_normal")
        self.assertThat(lambda: calc_win.is_focused, Eventually(Equals(True)))
        self.assertVisibleWindowStack([calc_win, mah_win2, mah_win1])

        self.keybinding("switcher/reveal_normal")
        self.assertThat(lambda: mah_win2.is_focused, Eventually(Equals(True)))
        self.assertVisibleWindowStack([mah_win2, calc_win, mah_win1])

        self.keybinding("window/close")
        self.assertThat(lambda: calc_win.is_focused, Eventually(Equals(True)))
        self.assertVisibleWindowStack([calc_win, mah_win1])


class SwitcherDetailsTests(SwitcherTestCase):
    """Test the details mode for the switcher."""
    def setUp(self):
        super(SwitcherDetailsTests, self).setUp()
        self.set_timeout_setting(True)

    def test_details_mode_on_delay(self):
        """Test that details mode activates on a timeout."""
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        #FIXME: Setup
        self.close_all_app('Character Map')
        self.workspace.switch_to(1)
        self.start_app("Character Map")
        sleep(1)
        self.start_app("Character Map")
        sleep(1)

        # Need to start a different app, so it has focus, so alt-tab goes to
        # the character map.
        self.start_app('Mahjongg')
        sleep(1)
        # end setup

        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        # Wait longer than details mode.
        sleep(3)
        self.assertThat(self.switcher.mode, Equals(SwitcherMode.DETAIL))

    def test_no_details_for_apps_on_different_workspace(self):
        """Tests that details mode does not initiates when there are multiple windows

        of an application spread across different workspaces.
        Regression test for LP:933406.

        """
        #Fixme: setup
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        self.close_all_app('Character Map')
        self.workspace.switch_to(1)
        self.start_app("Character Map")
        sleep(1)
        self.workspace.switch_to(2)
        self.start_app("Character Map")
        sleep(1)
        # Need to start a different app, so it has focus, so alt-tab goes to
        # the character map.
        self.start_app('Mahjongg')
        sleep(1)
        # end setup

        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        # Wait longer than details mode.
        sleep(3)
        self.assertThat(self.switcher.mode, Equals(SwitcherMode.NORMAL))


class SwitcherDetailsModeTests(SwitcherTestCase):
    """Tests for the details mode of the switcher.

    Tests for initiation with both grave (`) and Down arrow.

    """

    scenarios = [
        ('initiate_with_grave', {'initiate_keycode': '`'}),
        ('initiate_with_down', {'initiate_keycode': 'Down'}),
    ]

    def test_can_start_details_mode(self):
        """Must be able to initiate details mode using selected scenario keycode.

        """
        self.start_app("Character Map")
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        self.keyboard.press_and_release(self.initiate_keycode)
        self.assertThat(self.switcher.mode, Equals(SwitcherMode.DETAIL))

    def test_next_icon_from_last_detail_works(self):
        """Pressing next while showing last switcher item in details mode
        must select first item in the model in non-details mode.

        """
        self.start_app("Character Map")
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        while self.switcher.selection_index < len(self.switcher.icons) - 1:
            self.switcher.next_icon()
        self.keyboard.press_and_release(self.initiate_keycode)
        sleep(0.5)
        # Make sure we're at the end of the details list for this icon
        possible_details = self.switcher.detail_current_count - 1
        while self.switcher.detail_selection_index < possible_details:
            self.switcher.next_detail()

        self.switcher.next_icon()
        self.assertThat(self.switcher.selection_index, Equals(0))


class SwitcherWorkspaceTests(SwitcherTestCase):
    """Test Switcher behavior with respect to multiple workspaces."""

    def test_switcher_shows_current_workspace_only(self):
        """Switcher must show apps from the current workspace only."""
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        #FIXME: SETUP
        self.close_all_app('Calculator')
        self.close_all_app('Character Map')

        self.workspace.switch_to(1)
        calc = self.start_app("Calculator")
        sleep(1)
        self.workspace.switch_to(2)
        char_map = self.start_app("Character Map")
        sleep(1)
        # END SETUP

        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)

        icon_names = [i.tooltip_text for i in self.switcher.icons]
        self.assertThat(icon_names, Contains(char_map.name))
        self.assertThat(icon_names, Not(Contains(calc.name)))

    def test_switcher_all_mode_shows_all_apps(self):
        """Test switcher 'show_all' mode shows apps from all workspaces."""
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        self.close_all_app('Calculator')
        self.close_all_app('Character Map')

        #FIXME: this is setup
        self.workspace.switch_to(1)
        calc = self.start_app("Calculator")
        sleep(1)
        self.workspace.switch_to(2)
        char_map = self.start_app("Character Map")
        sleep(1)
        # END SETUP

        self.switcher.initiate(SwitcherMode.ALL)
        self.addCleanup(self.switcher.terminate)

        icon_names = [i.tooltip_text for i in self.switcher.icons]
        self.assertThat(icon_names, Contains(calc.name))
        self.assertThat(icon_names, Contains(char_map.name))

    def test_switcher_can_switch_to_minimised_window(self):
        """Switcher must be able to switch to a minimised window when there's

        another instance of the same application on a different workspace.

        """
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        #FIXME this is setup.
        # disable automatic gridding of the switcher after a timeout, since it makes
        # it harder to write the tests.
        self.set_unity_option("alt_tab_timeout", False)
        self.close_all_app("Calculator")
        self.close_all_app("Mahjongg")

        self.workspace.switch_to(1)
        self.start_app("Mahjongg")

        self.workspace.switch_to(3)
        mahjongg = self.start_app("Mahjongg")
        sleep(1)
        self.keybinding("window/minimize")
        sleep(1)

        self.start_app("Calculator")
        sleep(1)
        # END SETUP

        self.switcher.initiate()
        while self.switcher.current_icon.tooltip_text != mahjongg.name:
            logger.debug("%s -> %s" % (self.switcher.current_icon.tooltip_text, mahjongg.name))
            self.switcher.next_icon()
            sleep(1)
        self.switcher.select()

        #get mahjongg windows - there should be two:
        wins = mahjongg.get_windows()
        self.assertThat(len(wins), Equals(2))
        # Ideally we should be able to find the instance that is on the
        # current workspace and ask that one if it is hidden.
        self.assertFalse(wins[0].is_hidden)
        self.assertFalse(wins[1].is_hidden)
