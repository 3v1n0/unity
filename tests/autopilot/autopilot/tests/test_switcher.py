# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools.matchers import Equals, NotEquals, Contains, Not
from time import sleep

from autopilot.tests import AutopilotTestCase


class SwitcherTests(AutopilotTestCase):
    """Test the switcher."""

    def set_timeout_setting(self, value):
        self.set_unity_option("alt_tab_timeout", value)

    def setUp(self):
        super(SwitcherTests, self).setUp()

        self.char_map = self.start_app('Character Map')
        self.calc = self.start_app('Calculator')
        self.mahjongg = self.start_app('Mahjongg')

    def tearDown(self):
        super(SwitcherTests, self).tearDown()
        sleep(1)

    def test_switcher_move_next(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        sleep(.2)

        start = self.switcher.get_selection_index()
        self.switcher.next_icon()
        sleep(.2)

        end = self.switcher.get_selection_index()
        self.switcher.terminate()

        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start + 1))

    def test_switcher_move_prev(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        sleep(.2)

        start = self.switcher.get_selection_index()
        self.switcher.previous_icon()
        sleep(.2)

        end = self.switcher.get_selection_index()
        self.switcher.terminate()

        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start - 1))

    def test_switcher_scroll_next(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        sleep(.2)

        start = self.switcher.get_selection_index()
        self.switcher.next_icon_mouse()
        sleep(.2)

        end = self.switcher.get_selection_index()
        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start + 1))

        self.switcher.terminate()

    def test_switcher_scroll_prev(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        sleep(.2)

        start = self.switcher.get_selection_index()
        self.switcher.previous_icon_mouse()
        sleep(.2)

        end = self.switcher.get_selection_index()
        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start - 1))

        self.switcher.terminate()

    def test_switcher_scroll_next_ignores_fast_events(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        sleep(.2)

        # Quickly repeatead events should be ignored (except the first)
        start = self.switcher.get_selection_index()
        self.switcher.next_icon_mouse()
        self.switcher.next_icon_mouse()
        self.switcher.next_icon_mouse()
        sleep(.2)

        end = self.switcher.get_selection_index()
        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start + 1))

        self.switcher.terminate()

    def test_switcher_scroll_prev_ignores_fast_events(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        sleep(.2)

        # Quickly repeatead events should be ignored (except the first)
        start = self.switcher.get_selection_index()
        self.switcher.previous_icon_mouse()
        self.switcher.previous_icon_mouse()
        self.switcher.previous_icon_mouse()
        sleep(.2)

        end = self.switcher.get_selection_index()
        self.assertThat(end, Equals(start - 1))

        self.switcher.terminate()

    def test_switcher_arrow_key_does_not_init(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate_right_arrow()
        sleep(.2)

        self.assertThat(self.switcher.get_is_visible(), Equals(False))
        self.switcher.terminate()
        self.set_timeout_setting(True)

    def test_lazy_switcher_initiate(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.keybinding_hold("switcher/reveal_normal")
        self.addCleanup(self.keybinding_release, "switcher/reveal_normal")
        sleep(1)
        self.assertThat(self.switcher.get_is_visible(), Equals(False))

        sleep(1)
        self.keybinding_tap("switcher/reveal_normal")
        self.addCleanup(self.keybinding, "switcher/cancel")
        sleep(.5)
        self.assertThat(self.switcher.get_is_visible(), Equals(True))

    def test_switcher_cancel(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        sleep(.2)

        self.assertThat(self.switcher.get_is_visible(), Equals(True))

        self.switcher.cancel()
        sleep(.2)

        self.assertThat(self.switcher.get_is_visible(), Equals(False))

    def test_lazy_switcher_cancel(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.keybinding_hold("switcher/reveal_normal")
        self.addCleanup(self.keybinding_release, "switcher/reveal_normal")
        sleep(1)
        self.assertThat(self.switcher.get_is_visible(), Equals(False))

        sleep(1)
        self.keybinding_tap("switcher/reveal_normal")
        self.addCleanup(self.keybinding, "switcher/cancel")
        sleep(.5)
        self.assertThat(self.switcher.get_is_visible(), Equals(True))

        self.switcher.cancel()
        sleep(.2)

        self.assertThat(self.switcher.get_is_visible(), Equals(False))

    def test_switcher_appears_on_monitor_with_focused_window(self):
        num_monitors = self.screen_geo.get_num_monitors()
        if num_monitors == 1:
            self.skip("No point testing this on one monitor")

        [calc_win] = self.calc.get_windows()
        for monitor in range(num_monitors):
            self.screen_geo.drag_window_to_monitor(calc_win, monitor)
            self.switcher.initiate()
            sleep(1)
            self.assertThat(self.switcher.get_monitor(), Equals(monitor))
            self.switcher.terminate()


class SwitcherWindowsManagementTests(AutopilotTestCase):
    """Test the switcher window management."""

    def test_switcher_raises_only_last_focused_window(self):
        """Tests that when we do an alt+tab only the previously focused window
        is raised.
        This is tests by opening 2 Calculators and a Mahjongg.
        Then we do a quick alt+tab twice.
        Then we close the currently focused window.
        """
        self.close_all_app("Mahjongg")
        self.close_all_app("Calculator")

        mahj = self.start_app("Mahjongg")
        [mah_win1] = mahj.get_windows()
        self.assertTrue(mah_win1.is_focused)

        calc = self.start_app("Calculator")
        [calc_win] = calc.get_windows()
        self.assertTrue(calc_win.is_focused)

        self.start_app("Mahjongg")
        # Sleeping due to the start_app only waiting for the bamf model to be
        # updated with the application.  Since the app has already started,
        # and we are just waiting on a second window, however a defined sleep
        # here is likely to be problematic.
        # TODO: fix bamf emulator to enable waiting for new windows.
        sleep(1)
        [mah_win2] = [w for w in mahj.get_windows() if w.x_id != mah_win1.x_id]
        self.assertTrue(mah_win2.is_focused)

        self.assertVisibleWindowStack([mah_win2, calc_win, mah_win1])

        self.keybinding("switcher/reveal_normal")
        sleep(1)
        self.assertTrue(calc_win.is_focused)
        self.assertVisibleWindowStack([calc_win, mah_win2, mah_win1])

        self.keybinding("switcher/reveal_normal")
        sleep(1)
        self.assertTrue(mah_win2.is_focused)
        self.assertVisibleWindowStack([mah_win2, calc_win, mah_win1])

        self.keybinding("window/close")
        sleep(1)

        self.assertTrue(calc_win.is_focused)
        self.assertVisibleWindowStack([calc_win, mah_win1])


class SwitcherDetailsTests(AutopilotTestCase):
    """Test the details mode for the switcher."""

    def test_switcher_starts_in_normal_mode(self):
        """Switcher must start in normal (i.e.- not details) mode."""
        self.start_app("Character Map")
        sleep(1)

        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        self.assertThat(self.switcher.get_is_in_details_mode(), Equals(False))

    def test_details_mode_on_delay(self):
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

        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        # Wait longer than details mode.
        sleep(3)
        self.assertTrue(self.switcher.get_is_in_details_mode())

    def test_no_details_for_apps_on_different_workspace(self):
        # Re bug: 933406
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

        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        # Wait longer than details mode.
        sleep(3)
        self.assertFalse(self.switcher.get_is_in_details_mode())


class SwitcherDetailsModeTests(AutopilotTestCase):
    """Tests for the details mode of the switcher.

    Tests for initiation with both grave (`) and Down arrow.

    """

    scenarios = [
        ('initiate_with_grave', {'initiate_keycode': '`'}),
        ('initiate_with_down', {'initiate_keycode': 'Down'}),
    ]

    def test_can_start_details_mode(self):
        """Must be able to initiate details mode using selected scenario keycode."""
        self.start_app("Character Map")
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        self.keyboard.press_and_release(self.initiate_keycode)
        self.assertThat(self.switcher.get_is_in_details_mode(), Equals(True))

    def test_tab_from_last_detail_works(self):
        """Pressing tab while showing last switcher item in details mode
        must select first item in the model in non-details mode.

        """
        self.start_app("Character Map")
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        while self.switcher.get_selection_index() < self.switcher.get_model_size() -1:
            self.switcher.next_icon()
        self.keyboard.press_and_release(self.initiate_keycode)
        sleep(0.5)
        self.switcher.next_icon()
        self.assertThat(self.switcher.get_selection_index(), Equals(0))


class SwitcherWorkspaceTests(AutopilotTestCase):
    """Test Switcher behavior with respect to multiple workspaces."""

    def test_switcher_shows_current_workspace_only(self):
        """Switcher must show apps from the current workspace only."""
        self.close_all_app('Calculator')
        self.close_all_app('Character Map')

        self.workspace.switch_to(1)
        calc = self.start_app("Calculator")
        sleep(1)
        self.workspace.switch_to(2)
        char_map = self.start_app("Character Map")
        sleep(1)

        self.switcher.initiate()
        sleep(1)
        icon_names = [i.tooltip_text for i in self.switcher.get_switcher_icons()]
        self.switcher.terminate()
        self.assertThat(icon_names, Contains(char_map.name))
        self.assertThat(icon_names, Not(Contains(calc.name)))

    def test_switcher_all_mode_shows_all_apps(self):
        """Test switcher 'show_all' mode shows apps from all workspaces."""
        self.close_all_app('Calculator')
        self.close_all_app('Character Map')

        self.workspace.switch_to(1)
        calc = self.start_app("Calculator")
        sleep(1)
        self.workspace.switch_to(2)
        char_map = self.start_app("Character Map")
        sleep(1)

        self.switcher.initiate_all_mode()
        sleep(1)
        icon_names = [i.tooltip_text for i in self.switcher.get_switcher_icons()]
        self.switcher.terminate()
        self.assertThat(icon_names, Contains(calc.name))
        self.assertThat(icon_names, Contains(char_map.name))

    def test_switcher_can_switch_to_minimised_window(self):
        """Switcher must be able to switch to a minimised window when there's
        another instance of the same application on a different workspace."""
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

        self.switcher.initiate()
        sleep(1)
        while self.switcher.current_icon.tooltip_text != mahjongg.name:
            self.switcher.next_icon()
            sleep(1)
        self.switcher.stop()
        sleep(1)

        #get mahjongg windows - there should be two:
        wins = mahjongg.get_windows()
        self.assertThat(len(wins), Equals(2))
        # Ideally we should be able to find the instance that is on the
        # current workspace and ask that one if it is hidden.
        self.assertFalse(wins[0].is_hidden)
        self.assertFalse(wins[1].is_hidden)
