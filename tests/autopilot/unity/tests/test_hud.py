# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Alex Launi,
#         Marco Trevisan
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from autopilot.display import Display, move_mouse_to_screen, is_rect_on_screen
from autopilot.testcase import multiply_scenarios
from os import remove, environ
from os.path import exists
from tempfile import mktemp
from testtools.matchers import (
    Equals,
    EndsWith,
    GreaterThan,
    LessThan,
    NotEquals,
    )
from testtools.matchers import MismatchError
from time import sleep

from unity.emulators.icons import HudLauncherIcon
from unity.tests import UnityTestCase


def _make_monitor_scenarios():
    num_monitors = Display.create().get_num_screens()
    scenarios = []

    if num_monitors == 1:
        scenarios = [('Single Monitor', {'hud_monitor': 0})]
    else:
        for i in range(num_monitors):
            scenarios += [('Monitor %d' % (i), {'hud_monitor': i})]

    return scenarios


class HudTestsBase(UnityTestCase):

    def setUp(self):
        super(HudTestsBase, self).setUp()

    def tearDown(self):
        self.unity.hud.ensure_hidden()
        super(HudTestsBase, self).tearDown()

    def get_num_active_launcher_icons(self):
        num_active = 0
        for icon in self.unity.launcher.model.get_launcher_icons():
            if icon.active and icon.visible:
                num_active += 1
        return num_active

    # Unable to exit SDM without any active apps, need a placeholder.
    # See bug LP:1079460
    def start_placeholder_app(self):
        window_spec = {
            "Title": "Placeholder application",
        }
        self.launch_test_window(window_spec)

    def start_menu_app(self):
        window_spec = {
            "Title": "Test menu application",
            "Menu": ["Entry 1", "Entry 2", "Entry 3", "Entry 4", "Entry 5", "Quit"],
        }
        self.launch_test_window(window_spec)



class HudBehaviorTests(HudTestsBase):

    def setUp(self):
        super(HudBehaviorTests, self).setUp()

        if not environ.get('UBUNTU_MENUPROXY', ''):
            self.patch_environment('UBUNTU_MENUPROXY', 'libappmenu.so')
        self.hud_monitor = self.display.get_primary_screen()
        move_mouse_to_screen(self.hud_monitor)

    def test_no_initial_values(self):
        self.unity.hud.ensure_visible()
        self.assertThat(self.unity.hud.num_buttons, Equals(0))
        self.assertThat(self.unity.hud.selected_button, Equals(0))

    def test_check_a_values(self):
        self.start_menu_app()
        self.unity.hud.ensure_visible()
        self.keyboard.type('e')
        self.assertThat(self.unity.hud.search_string, Eventually(Equals('e')))
        self.assertThat(self.unity.hud.num_buttons, Eventually(Equals(5)))
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(1)))

    def test_up_down_arrows(self):
        self.start_menu_app()
        self.unity.hud.ensure_visible()
        self.keyboard.type('e')
        self.assertThat(self.unity.hud.search_string, Eventually(Equals('e')))
        self.assertThat(self.unity.hud.num_buttons, Eventually(Equals(5)))

        self.keyboard.press_and_release('Down')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(2)))
        self.keyboard.press_and_release('Down')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(3)))
        self.keyboard.press_and_release('Down')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(4)))
        self.keyboard.press_and_release('Down')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(5)))
        # Down again stays on 5.
        self.keyboard.press_and_release('Down')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(5)))
        self.keyboard.press_and_release('Up')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(4)))
        self.keyboard.press_and_release('Up')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(3)))
        self.keyboard.press_and_release('Up')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(2)))
        self.keyboard.press_and_release('Up')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(1)))
        # Up again stays on 1.
        self.keyboard.press_and_release('Up')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(1)))

    def test_no_reset_selected_button(self):
        """Hud must not change selected button when results update over time."""
        # TODO - this test doesn't test anything. Onmy system the results never update.
        # ideally we'd send artificial results to the hud from the test.
        self.skipTest("This test makes no sense in its current state, needs reworking.")

        self.unity.hud.ensure_visible()
        self.keyboard.type('is')
        self.assertThat(self.unity.hud.search_string, Eventually(Equals('is')))
        self.keyboard.press_and_release('Down')
        self.assertThat(self.unity.hud.selected_button, Eventually(Equals(2)))
        # long sleep to let the service send updated results
        sleep(10)
        self.assertThat(self.unity.hud.selected_button, Equals(2))

    def test_slow_tap_not_reveal_hud(self):
        """A slow tap must not reveal the HUD."""
        self.keybinding("hud/reveal", 0.3)
        # need a long sleep to ensure that we test after the hud controller has
        # seen the keypress.
        sleep(5)
        self.assertThat(self.unity.hud.visible, Equals(False))

    def test_alt_f4_doesnt_show_hud(self):
        self.process_manager.start_app('Calculator')
        sleep(1)
        # Do a very fast Alt+F4
        self.keyboard.press_and_release("Alt+F4", 0.05)
        sleep(1)
        self.assertFalse(self.unity.hud.visible)

    def test_reveal_hud_with_no_apps(self):
        """Hud must show even with no visible applications.

        This used to cause unity to crash (hence the lack of assertion in this test).

        """
        self.start_placeholder_app()
        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        self.unity.hud.ensure_visible()
        self.unity.hud.ensure_hidden()

    def test_restore_focus(self):
        """Ensures that once the hud is dismissed, the same application
        that was focused before hud invocation is refocused.
        """
        calc = self.process_manager.start_app("Calculator")

        # first ensure that the application has started and is focused
        self.assertEqual(calc.is_active, True)

        self.unity.hud.ensure_visible()
        self.unity.hud.ensure_hidden()

        # again ensure that the application we started is focused
        self.assertEqual(calc.is_active, True)

        self.unity.hud.ensure_visible()
        self.unity.hud.ensure_hidden()
        # why do we do this: ???
        self.keyboard.press_and_release('Return')
        sleep(1)

        self.assertEqual(calc.is_active, True)

    def test_gedit_undo(self):
        """Test that the 'undo' action in the Hud works with GEdit."""

        file_path = mktemp()
        self.addCleanup(remove, file_path)
        gedit_win = self.process_manager.start_app_window('Text Editor', files=[file_path], locale='C')
        self.addCleanup(self.process_manager.close_all_app, 'Text Editor')
        self.assertProperty(gedit_win, is_focused=True)

        self.keyboard.type("0")
        self.keyboard.type(" ")
        self.keyboard.type("1")

        self.unity.hud.ensure_visible()

        self.keyboard.type("undo")
        hud_query_check = lambda: self.unity.hud.selected_hud_button.label_no_formatting
        # XXX: with the new HUD, command and description is separated by '\u2002' and
        #  not a regular space ' '. Is that correct? (LP: #1172237)
        self.assertThat(hud_query_check,
                        Eventually(Equals(u'Undo\u2002(Edit)')))
        self.keyboard.press_and_release('Return')
        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

        self.assertProperty(gedit_win, is_focused=True)
        self.keyboard.press_and_release("Ctrl+s")
        self.assertThat(lambda: exists(file_path), Eventually(Equals(True)))

        contents = open(file_path).read().strip('\n')
        self.assertEqual("0 ", contents)

    def test_hud_to_dash_has_key_focus(self):
        """When switching from the hud to the dash you don't lose key focus."""
        self.unity.hud.ensure_visible()
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)
        self.keyboard.type('focus1')
        self.assertThat(self.unity.dash.search_string, Eventually(Equals('focus1')))

    def test_dash_to_hud_has_key_focus(self):
        """When switching from the dash to the hud you don't lose key focus."""
        self.unity.dash.ensure_visible()
        self.unity.hud.ensure_visible()
        self.keyboard.type('focus2')
        self.assertThat(self.unity.hud.search_string, Eventually(Equals('focus2')))

    def test_hud_closes_on_workspace_switch(self):
        """This test shows that when you switch to another workspace the hud closes."""
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled more than one workspace.")
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        self.unity.hud.ensure_visible()
        self.workspace.switch_to((initial_workspace + 1) % self.workspace.num_workspaces)
        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_hud_closes_on_spread(self):
        """This test shows that when the spread is initiated, the hud closes."""
        # Need at least one application open for the spread to work.
        self.process_manager.start_app_window("Calculator")
        self.unity.hud.ensure_visible()
        self.addCleanup(self.keybinding, "spread/cancel")
        self.keybinding("spread/start")
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))
        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_hud_closes_click_outside_geo_shrunk(self):
        """
        Clicking outside the hud when it is shurnk will make it close.
        Shurnk is when the hud has no results and is much smaller then normal.
        """

        self.unity.hud.ensure_visible()
        (x,y,w,h) = self.unity.hud.view.geometry
        self.mouse.move(w/2, h-50)
        self.mouse.click()

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_hud_closes_click_outside_geo(self):
        """Clicking outside of the hud will make it close."""

        self.unity.hud.ensure_visible()
        self.keyboard.type("Test")

        (x,y,w,h) = self.unity.hud.view.geometry
        self.mouse.move(w/2, h+50)
        self.mouse.click()

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_hud_closes_click_after_text_removed(self):
        """Clicking outside of the hud after a search text has been entered and
        then removed from the searchbox will make it close."""

        self.unity.hud.ensure_visible()
        self.keyboard.type("Test")
        self.keyboard.press_and_release("Escape")

        (x,y,w,h) = self.unity.hud.view.geometry
        self.mouse.move(w/2, h+50)
        self.mouse.click()

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_alt_f4_close_hud(self):
        """Hud must close on alt+F4."""
        self.unity.hud.ensure_visible()
        self.keyboard.press_and_release("Alt+F4")
        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_alt_f4_close_hud_with_capslock_on(self):
        """Hud must close on Alt+F4 even when the capslock is turned on."""
        self.keyboard.press_and_release("Caps_Lock")
        self.addCleanup(self.keyboard.press_and_release, "Caps_Lock")

        self.unity.hud.ensure_visible()
        self.keyboard.press_and_release("Alt+F4")
        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_app_activate_on_enter(self):
        """Hud must close after activating a search item with Enter."""
        self.process_manager.start_app('Text Editor', locale='C')
        self.addCleanup(self.process_manager.close_all_app, "Text Editor")

        self.unity.hud.ensure_visible()

        self.keyboard.type("Quit")
        self.assertThat(self.unity.hud.search_string, Eventually(Equals("Quit")))
        hud_query_check = lambda: self.unity.hud.selected_hud_button.label_no_formatting
        self.assertThat(hud_query_check,
                        Eventually(Equals(u'Quit\u2002(File)')))

        self.keyboard.press_and_release("Enter")

        self.assertFalse(self.process_manager.app_is_running("Text Editor"))

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_hud_closes_on_escape(self):
        """Hud must close on escape after searchbox is cleared"""
        self.unity.hud.ensure_visible()

        self.keyboard.type("ThisText")
        self.keyboard.press_and_release("Escape")
        self.keyboard.press_and_release("Escape")

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_hud_closes_on_escape_shrunk(self):
        """Hud must close when escape key is pressed"""
        self.unity.hud.ensure_visible()
        self.keyboard.press_and_release("Escape")

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_alt_arrow_keys_not_eaten(self):
        """Tests that Alt+ArrowKey events are correctly passed to the
        active window when Unity is not responding to them."""

        term_win = self.process_manager.start_app_window("Terminal")
        self.assertProperty(term_win, is_focused=True)

        # Here we anyway need a sleep, since even though the terminal can have
        # keyboard focus, the shell inside might not be completely loaded yet
        # and keystrokes might not get registered
        sleep(1)

        #There's no easy way to read text from terminal, writing input
        #to a text file and then reading from there works.
        self.keyboard.type('echo "')

        #Terminal is receiving input with Alt+Arrowkeys
        self.keyboard.press("Alt")
        self.keyboard.press_and_release("Up")
        self.keyboard.press_and_release("Down")
        self.keyboard.press_and_release("Right")
        self.keyboard.press_and_release("Left")
        self.keyboard.release("Alt")

        self.keyboard.type('" > /tmp/ap_test_alt_keys')
        self.addCleanup(remove, '/tmp/ap_test_alt_keys')
        self.keyboard.press_and_release("Enter")

        file_contents = open('/tmp/ap_test_alt_keys', 'r').read().strip()

        self.assertThat(file_contents, Equals('ABCD'))

    def test_mouse_changes_selected_hud_button(self):
        """This tests moves the mouse from the top of the screen to the bottom, this must
        change the selected button from 1 to 5.
        """
        self.start_menu_app()
        self.unity.hud.ensure_visible()

        self.keyboard.type("e")
        self.assertThat(self.unity.hud.num_buttons, Eventually(Equals(5)))
        (x,y,w,h) = self.unity.hud.view.geometry

        # Specify a slower rate so that HUD can register the mouse movement properly
        self.mouse.move(w/2, 0, rate=5)
        self.assertThat(self.unity.hud.view.selected_button, Eventually(Equals(1)))

        self.mouse.move(w/2, h, rate=5)
        self.assertThat(self.unity.hud.view.selected_button, Eventually(Equals(5)))

    def test_keyboard_steals_focus_from_mouse(self):
        """This tests moves the mouse from the top of the screen to the bottom,
        then it presses the keyboard up 5 times, this must change the selected button from 5 to 1.
        """
        self.start_menu_app()
        self.unity.hud.ensure_visible()

        self.keyboard.type("e")
        self.assertThat(self.unity.hud.num_buttons, Eventually(Equals(5)))
        (x,y,w,h) = self.unity.hud.view.geometry

        self.mouse.move(w/2, 0)
        self.mouse.move(w/2, h)
        self.assertThat(self.unity.hud.view.selected_button, Eventually(Equals(5)))

        for i in range(5):
          self.keyboard.press_and_release('Up')

        self.assertThat(self.unity.hud.view.selected_button, Eventually(Equals(1)))

    def test_keep_focus_on_application_opens(self):
        """The Hud must keep key focus as well as stay open if an app gets opened from an external source. """

        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.process_manager.start_app_window("Calculator")
        sleep(1)

        self.keyboard.type("HasFocus")
        self.assertThat(self.unity.hud.search_string, Eventually(Equals("HasFocus")))

    def test_closes_mouse_down_outside(self):
        """Test that a mouse down outside of the hud closes the hud."""

        self.unity.hud.ensure_visible()
        current_monitor = self.unity.hud.monitor

        (x,y,w,h) = self.unity.hud.geometry
        (screen_x,screen_y,screen_w,screen_h) = self.display.get_screen_geometry(current_monitor)

        self.mouse.move(x + w + (screen_w-((screen_x-x)+w))/2, y + h + (screen_h-((screen_y-y)+h))/2)
        self.mouse.click()

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_closes_then_focuses_window_on_mouse_down(self):
        """If 2 windows are open with 1 maximized and the non-maxmized
        focused. Then from the Hud clicking on the maximized window
        must focus that window and close the hud.
        """
        char_win = self.process_manager.start_app("Character Map")
        self.assertProperty(char_win, is_active=True)
        self.keybinding("window/maximize")
        self.process_manager.start_app("Calculator")

        self.unity.hud.ensure_visible()

        #Click bottom right of the screen
        w = self.display.get_screen_width()
        h = self.display.get_screen_height()
        self.mouse.move(w,h)
        self.mouse.click()

        self.assertProperty(char_win, is_active=True)

    def test_hud_does_not_focus_wrong_window_after_alt_tab(self):
        """Test the Hud focuses the correct window after an Alt+Tab."""

        char_win = self.process_manager.start_app('Character Map')
        self.process_manager.start_app('Calculator')

        self.keybinding("switcher/reveal_normal")
        self.assertProperty(char_win, is_active=True)

        self.unity.hud.ensure_visible()
        self.unity.hud.ensure_hidden()

        self.assertProperty(char_win, is_active=True)

    def test_mouse_does_not_steal_button_focus(self):
        """When typing in the hud the mouse must not steal button focus."""
        self.start_menu_app()
        self.unity.hud.ensure_visible()

        (x,y,w,h) = self.unity.hud.view.geometry
        self.mouse.move(w/4, h/4)

        self.keyboard.type("e")
        self.assertThat(self.unity.hud.view.selected_button, Eventually(Equals(1)))


class HudLauncherInteractionsTests(HudTestsBase):

    launcher_modes = [('Launcher autohide', {'launcher_autohide': False}),
                      ('Launcher never hide', {'launcher_autohide': True})]

    scenarios = multiply_scenarios(_make_monitor_scenarios(), launcher_modes)

    def setUp(self):
        super(HudLauncherInteractionsTests, self).setUp()
        # Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', int(self.launcher_autohide))

        move_mouse_to_screen(self.hud_monitor)
        sleep(0.5)

    def test_multiple_hud_reveal_does_not_break_launcher(self):
        """Multiple Hud reveals must not cause the launcher to set multiple
        apps as active.

        """
        launcher = self.unity.launcher.get_launcher_for_monitor(self.hud_monitor)

        # We need an app to switch to:
        self.process_manager.start_app('Character Map')
        # We need an application to play with - I'll use the calculator.
        self.process_manager.start_app('Calculator')
        sleep(1)

        # before we start, make sure there's zero or one active icon:
        num_active = self.get_num_active_launcher_icons()
        self.assertThat(num_active, LessThan(2), "Invalid number of launcher icons active before test has run!")

        # reveal and hide hud several times over:
        for i in range(3):
            self.unity.hud.ensure_visible()
            self.unity.hud.ensure_hidden()

        # click application icons for running apps in the launcher:
        icon = self.unity.launcher.model.get_icon(desktop_id="gucharmap.desktop")
        launcher.click_launcher_icon(icon)

        # see how many apps are marked as being active:
        num_active = self.get_num_active_launcher_icons()
        self.assertLessEqual(num_active, 1, "More than one launcher icon active after test has run!")

    def test_hud_does_not_change_launcher_status(self):
        """Opening the HUD must not change the launcher visibility."""

        launcher = self.unity.launcher.get_launcher_for_monitor(self.hud_monitor)

        launcher_shows_pre = launcher.is_showing
        self.unity.hud.ensure_visible()
        launcher_shows_post = launcher.is_showing
        self.assertThat(launcher_shows_pre, Equals(launcher_shows_post))


class HudLockedLauncherInteractionsTests(HudTestsBase):

    scenarios = _make_monitor_scenarios()

    def setUp(self):
        super(HudLockedLauncherInteractionsTests, self).setUp()
        # Locked Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', 0)

        move_mouse_to_screen(self.hud_monitor)
        sleep(0.5)

    def test_hud_launcher_icon_hides_bfb(self):
        """BFB icon must be hidden when the HUD launcher icon is shown."""

        hud_icon = self.unity.hud.get_launcher_icon()
        bfb_icon = self.unity.launcher.model.get_bfb_icon()

        self.assertThat(bfb_icon.visible, Eventually(Equals(True)))
        self.assertTrue(bfb_icon.is_on_monitor(self.hud_monitor))
        self.assertThat(hud_icon.visible, Eventually(Equals(False)))

        self.unity.hud.ensure_visible()

        self.assertThat(hud_icon.visible, Eventually(Equals(True)))
        self.assertTrue(hud_icon.is_on_monitor(self.hud_monitor))
        # For some reason the BFB icon is always visible :-/
        #bfb_icon.visible, Eventually(Equals(False)

    def test_hud_desaturates_launcher_icons(self):
        """Launcher icons must desaturate when the HUD is opened."""

        self.unity.hud.ensure_visible()

        for icon in self.unity.launcher.model.get_launcher_icons_for_monitor(self.hud_monitor):
            if isinstance(icon, HudLauncherIcon):
                self.assertThat(icon.desaturated, Eventually(Equals(False)))
            else:
                self.assertThat(icon.desaturated, Eventually(Equals(True)))

    def test_hud_launcher_icon_click_hides_hud(self):
        """Clicking the Hud Icon should hide the HUD"""

        hud_icon = self.unity.hud.get_launcher_icon()
        self.unity.hud.ensure_visible()

        launcher = self.unity.launcher.get_launcher_for_monitor(self.hud_monitor)
        launcher.click_launcher_icon(hud_icon)

        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))
        self.assertThat(hud_icon.visible, Eventually(Equals(False)))


class HudVisualTests(HudTestsBase):

    launcher_modes = [('Launcher autohide', {'launcher_autohide': False}),
                      ('Launcher never hide', {'launcher_autohide': True})]

    launcher_screen = [('Launcher on primary monitor', {'launcher_primary_only': False}),
                       ('Launcher on all monitors', {'launcher_primary_only': True})]

    scenarios = multiply_scenarios(_make_monitor_scenarios(), launcher_modes, launcher_screen)

    def setUp(self):
        super(HudVisualTests, self).setUp()
        move_mouse_to_screen(self.hud_monitor)
        self.set_unity_option('launcher_hide_mode', int(self.launcher_autohide))
        self.set_unity_option('num_launchers', int(self.launcher_primary_only))
        self.hud_monitor_is_primary = (self.display.get_primary_screen() == self.hud_monitor)
        self.hud_locked = (not self.launcher_autohide and (not self.launcher_primary_only or self.hud_monitor_is_primary))
        sleep(0.5)

    def test_initially_hidden(self):
        self.assertFalse(self.unity.hud.visible)

    def test_hud_is_on_right_monitor(self):
        """HUD must be drawn on the monitor where the mouse is."""
        self.unity.hud.ensure_visible()
        self.assertThat(self.unity.hud.monitor, Eventually(Equals(self.hud_monitor)))
        self.assertTrue(is_rect_on_screen(self.unity.hud.monitor, self.unity.hud.geometry))

    def test_hud_geometries(self):
        """Tests the HUD geometries for the given monitor and status."""
        self.unity.hud.ensure_visible()
        monitor_geo = self.display.get_screen_geometry(self.hud_monitor)
        monitor_x = monitor_geo[0]
        monitor_w = monitor_geo[2]
        hud_x = self.unity.hud.geometry[0]
        hud_w = self.unity.hud.geometry[2]

        if self.hud_locked:
            self.assertThat(hud_x, GreaterThan(monitor_x))
            self.assertThat(hud_x, LessThan(monitor_x + monitor_w))
            self.assertThat(hud_w, Equals(monitor_x + monitor_w - hud_x))
        else:
            self.assertThat(hud_x, Equals(monitor_x))
            self.assertThat(hud_w, Equals(monitor_w))

    def test_hud_is_locked_to_launcher(self):
        """Tests if the HUD is locked to launcher as we expect or not."""
        self.unity.hud.ensure_visible()
        self.assertThat(self.unity.hud.is_locked_launcher, Eventually(Equals(self.hud_locked)))

    def test_hud_icon_is_shown(self):
        """Tests that the correct HUD icon is shown."""
        self.unity.hud.ensure_visible()
        hud_launcher_icon = self.unity.hud.get_launcher_icon()
        hud_embedded_icon = self.unity.hud.get_embedded_icon()

        if self.unity.hud.is_locked_launcher:
            self.assertThat(hud_launcher_icon.visible, Eventually(Equals(True)))
            self.assertTrue(hud_launcher_icon.is_on_monitor(self.hud_monitor))
            self.assertTrue(hud_launcher_icon.active)
            self.assertThat(hud_launcher_icon.monitor, Equals(self.hud_monitor))
            self.assertFalse(hud_launcher_icon.desaturated)
            self.assertThat(hud_embedded_icon, Equals(None))
        else:
            self.assertThat(hud_launcher_icon.visible, Eventually(Equals(False)))
            self.assertFalse(hud_launcher_icon.active)
            # the embedded icon has no visible property.
            self.assertThat(hud_embedded_icon, NotEquals(None))

    def test_hud_icon_shows_the_focused_application_emblem(self):
        """Tests that the correct HUD icon is shown."""
        self.process_manager.close_all_app("Calculator")
        calc = self.process_manager.start_app("Calculator")
        self.assertTrue(calc.is_active)
        self.unity.hud.ensure_visible()

        self.assertThat(self.unity.hud.icon.icon_name, Eventually(Equals(calc.icon)))

    def test_hud_icon_shows_the_ubuntu_emblem_on_empty_desktop(self):
        """When in 'show desktop' mode the hud icon must be the BFB icon."""
        self.start_placeholder_app()
        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)
        self.unity.hud.ensure_visible()

        self.assertThat(self.unity.hud.icon.icon_name, Eventually(EndsWith("launcher_bfb.png")))

    def test_switch_dash_hud_does_not_break_the_focused_application_emblem(self):
        """Switching from Dash to HUD must still show the correct HUD icon."""
        self.process_manager.close_all_app("Calculator")
        calc = self.process_manager.start_app("Calculator")
        self.assertTrue(calc.is_active)

        self.unity.dash.ensure_visible()
        self.unity.hud.ensure_visible()

        self.assertThat(self.unity.hud.icon.icon_name, Eventually(Equals(calc.icon)))

    def test_switch_hud_dash_does_not_break_the_focused_application_emblem(self):
        """Switching from HUD to Dash and back must still show the correct HUD icon."""
        self.process_manager.close_all_app("Calculator")
        calc = self.process_manager.start_app("Calculator")
        self.assertTrue(calc.is_active)

        self.unity.hud.ensure_visible()
        self.unity.dash.ensure_visible()
        self.unity.hud.ensure_visible()
        self.assertThat(self.unity.hud.icon.icon_name, Eventually(Equals(calc.icon)))

    def test_dash_hud_only_uses_icon_from_current_desktop(self):
        """
        Switching from the dash to Hud must pick an icon from applications
        from the current desktop. As the Hud must go through the entire window
        stack to find the top most window.
        """
        if self.workspace.num_workspaces <= 1:
            self.skipTest("This test requires enabled more than one workspace.")
        self.start_placeholder_app()
        initial_workspace = self.workspace.current_workspace
        self.addCleanup(self.workspace.switch_to, initial_workspace)
        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        calc = self.process_manager.start_app("Calculator")
        self.assertTrue(calc.is_active)
        self.workspace.switch_to((initial_workspace + 1) % self.workspace.num_workspaces)
        self.unity.dash.ensure_visible()
        self.unity.hud.ensure_visible()

        self.assertThat(self.unity.hud.icon.icon_name, Eventually(EndsWith("launcher_bfb.png")))


class HudAlternativeKeybindingTests(HudTestsBase):

    def alternateCheck(self, keybinding="Alt", hide=False):
        """Nasty workaround for LP: #1157738"""
        # So, since we have no way of making sure that after changing the unity option
        # the change will be already 'applied' on the system, let's try the keybinding
        # a few times before deciding that it does not work and we have a regression
        # Seems better than a sleep(some_value_here)
        for i in range(1, 4):
            try:
                self.keyboard.press_and_release(keybinding)
                self.assertThat(self.unity.hud.visible, Eventually(Equals(True), timeout=3))
                break
            except MismatchError:
                continue
        if hide:
            self.unity.hud.ensure_hidden()

    def test_super_h(self):
        """Test hud reveal on <Super>h."""
        self.addCleanup(self.alternateCheck, hide=True)
        self.set_unity_option("show_hud", "<Super>h")
        # Don't use reveal_hud, but be explicit in the keybindings.
        self.alternateCheck("Super+h")
        self.assertThat(self.unity.hud.visible, Eventually(Equals(True)))

    def test_ctrl_alt_h(self):
        """Test hud reveal on <Contrl><Alt>h."""
        self.addCleanup(self.alternateCheck, hide=True)
        self.set_unity_option("show_hud", "<Control><Alt>h")
        # Don't use reveal_hud, but be explicit in the keybindings.
        self.alternateCheck("Ctrl+Alt+h")
        self.assertThat(self.unity.hud.visible, Eventually(Equals(True)))


class HudCrossMonitorsTests(HudTestsBase):
    """Multi-monitor hud tests."""

    def setUp(self):
        super(HudCrossMonitorsTests, self).setUp()
        if self.display.get_num_screens() < 2:
            self.skipTest("This test requires more than 1 monitor.")

    def test_hud_stays_on_same_monitor(self):
        """If the hud is opened, then the mouse is moved to another monitor and
        the keyboard is used. The hud must not move to that monitor.
        """

        current_monitor = self.unity.hud.ideal_monitor

        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        move_mouse_to_screen((current_monitor + 1) % self.display.get_num_screens())
        self.keyboard.type("abc")

        self.assertThat(self.unity.hud.ideal_monitor, Eventually(Equals(current_monitor)))

    def test_hud_close_on_cross_monitor_click(self):
        """Hud must close when clicking on a window in a different screen."""

        self.addCleanup(self.unity.hud.ensure_hidden)

        for monitor in range(self.display.get_num_screens()-1):
            move_mouse_to_screen(monitor)
            self.unity.hud.ensure_visible()

            move_mouse_to_screen(monitor+1)
            sleep(.5)
            self.mouse.click()

            self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))
