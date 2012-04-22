# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import logging
import os
from testtools.matchers import Equals,  GreaterThan, NotEquals
from time import sleep

from autopilot.emulators.X11 import ScreenGeometry
from autopilot.emulators.bamf import BamfWindow
from autopilot.emulators.unity.panel import IndicatorEntry
from autopilot.tests import AutopilotTestCase


logger = logging.getLogger(__name__)


def _make_monitor_scenarios():
    num_monitors = ScreenGeometry().get_num_monitors()
    scenarios = []

    if num_monitors == 1:
        scenarios = [('Single Monitor', {'panel_monitor': 0})]
    else:
        for i in range(num_monitors):
            scenarios += [('Monitor %d' % (i), {'panel_monitor': i})]

    return scenarios


class PanelTestsBase(AutopilotTestCase):

    panel_monitor = 0

    def setUp(self):
        super(PanelTestsBase, self).setUp()
        self.panel = self.panels.get_panel_for_monitor(self.panel_monitor)
        self.panel.move_mouse_below_the_panel()
        self.addCleanup(self.panel.move_mouse_below_the_panel)

    def open_new_application_window(self, app_name, maximized=False, move_to_monitor=True):
        """Opens a new instance of the requested application, ensuring that only
        one window is opened.

        Returns the opened BamfWindow

        """
        self.close_all_app(app_name)
        app = self.start_app(app_name, locale="C")

        [app_win] = app.get_windows()

        app_win.set_focus()
        self.assertTrue(app.is_active)
        self.assertTrue(app_win.is_focused)
        self.assertThat(app.desktop_file, Equals(app_win.application.desktop_file))

        if move_to_monitor:
            self.move_window_to_panel_monitor(app_win)

        if maximized and not app_win.is_maximized:
            self.keybinding("window/maximize")
            self.addCleanup(self.keybinding, "window/restore")
        elif not maximized and app_win.is_maximized:
            self.keybinding("window/restore")
            self.addCleanup(self.keybinding, "window/maximize")

        app_win.set_focus()
        sleep(.25)

        self.assertThat(app_win.is_maximized, Equals(maximized))

        return app_win

    def move_window_to_panel_monitor(self, window, restore_position=True):
        """Drags a window to another monitor, eventually restoring it before"""
        if not isinstance(window, BamfWindow):
            raise TypeError("Window must be a BamfWindow")

        if window.monitor == self.panel_monitor:
            return

        if window.is_maximized:
            self.keybinding("window/restore")
            self.addCleanup(self.keybinding, "window/maximize")
            sleep(.1)

        if restore_position:
            self.addCleanup(self.screen_geo.drag_window_to_monitor, window, window.monitor)

        self.screen_geo.drag_window_to_monitor(window, self.panel_monitor)
        sleep(.25)
        self.assertThat(window.monitor, Equals(self.panel_monitor))

    def mouse_open_indicator(self, indicator):
        """This is an utility function that safely opens an indicator,
        ensuring that it is closed at the end of the test and that the pointer
        is moved outside the panel area (to make the panel hide the menus)
        """
        if not isinstance(indicator, IndicatorEntry):
            raise TypeError("Window must be a IndicatorEntry")

        indicator.mouse_click()
        self.addCleanup(self.panel.move_mouse_below_the_panel)
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(.5)
        self.assertTrue(indicator.active)


class PanelTitleTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def test_panel_title_on_empty_desktop(self):
        """With no windows shown, the panel must display the default title."""
        self.keybinding("window/show_desktop")
        # We need this sleep to give the time to showdesktop to properly resume
        # the initial status without getting a false-negative result
        self.addCleanup(sleep, 2)
        self.addCleanup(self.keybinding, "window/show_desktop")
        sleep(2)

        self.assertTrue(self.panel.desktop_is_active)

    def test_panel_title_with_restored_application(self):
        """Panel must display application name for a non-maximised application."""
        calc_win = self.open_new_application_window("Calculator")

        self.assertFalse(calc_win.is_maximized)
        self.assertThat(self.panel.title, Equals(calc_win.application.name))

    def test_panel_title_with_maximized_application(self):
        """Panel must display application name for a maximised application."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))

    def test_panel_title_with_maximized_window_restored_child(self):
        """Tests the title shown in the panel when opening the restored child of
        a maximized application.
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))

        self.keyboard.press_and_release("Ctrl+h")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(.25)
        self.assertThat(self.panel.title, Equals(text_win.application.name))

    def test_panel_title_switching_active_window(self):
        """Tests the title shown in the panel with a maximized application."""
        # Locked Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', 0)

        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))
        sleep(.25)

        calc_win = self.open_new_application_window("Calculator")
        self.assertThat(self.panel.title, Equals(calc_win.application.name))

        icon = self.launcher.model.get_icon_by_desktop_id(text_win.application.desktop_file)
        launcher = self.launcher.get_launcher_for_monitor(self.panel_monitor)
        launcher.click_launcher_icon(icon)

        self.assertTrue(text_win.is_focused)
        self.assertThat(self.panel.title, Equals(text_win.title))

    def test_panel_title_updates_on_maximized_window_title_changes(self):
        """Tests that the title of a maximized application updates with
        window title changes.
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.assertThat(self.panel.title, Equals(text_win.title))
        old_title = text_win.title
        sleep(.25)

        text_win.set_focus()
        self.keyboard.type("Unity rocks!")
        self.addCleanup(os.remove, "/tmp/autopilot-awesome-test.txt")
        self.keyboard.press_and_release("Ctrl+S")
        sleep(.25)
        self.keyboard.type("/tmp/autopilot-awesome-test.txt")
        self.keyboard.press_and_release("Return")
        sleep(1)

        self.assertThat(old_title, NotEquals(text_win.title))
        self.assertThat(self.panel.title, Equals(text_win.title))


class PanelWindowButtonsTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def setUp(self):
        super(PanelWindowButtonsTests, self).setUp()
        # Locked Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', 0)

    def test_window_buttons_dont_show_on_empty_desktop(self):
        """Tests that the window buttons are not shown on clean desktop."""
        # THis initially used Show Desktop mode, but it's very buggy from within
        # autopilot. We assume that workspace 2 is empty (which is safe for the
        # jenkins runs at least.)
        self.workspace.switch_to(2)
        sleep(.5)
        self.assertFalse(self.panel.window_buttons_shown)
        self.panel.move_mouse_over_window_buttons()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_dont_show_for_restored_window(self):
        """Tests that the window buttons are not shown for a restored window."""
        self.open_new_application_window("Calculator")

        self.assertFalse(self.panel.window_buttons_shown)

        self.panel.move_mouse_over_window_buttons()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_dont_show_for_maximized_window_on_mouse_out(self):
        """Tests that the windows button arenot shown for a maximized window
        when the mouse is outside the panel.
        """
        self.open_new_application_window("Text Editor", maximized=True)

        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.discovery_fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_fadeout_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_show_for_maximized_window_on_mouse_in(self):
        """Tests that the window buttons are shown when a maximized window
        is focused and the mouse is over the menu-view panel areas.
        """
        self.open_new_application_window("Text Editor", maximized=True)

        sleep(self.panel.menus.discovery_duration)

        self.panel.move_mouse_over_window_buttons()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertTrue(self.panel.window_buttons_shown)

        buttons = self.panel.window_buttons.get_buttons()
        self.assertThat(len(buttons), Equals(3))
        for button in buttons:
            self.assertFalse(button.overlay_mode)

    def test_window_buttons_show_with_dash(self):
        """Tests that the window buttons are shown when opening the dash."""
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(.5)
        self.assertTrue(self.panel.window_buttons_shown)

        buttons = self.panel.window_buttons.get_buttons()
        self.assertThat(len(buttons), Equals(3))
        for button in buttons:
            self.assertTrue(button.overlay_mode)

    def test_window_buttons_show_with_hud(self):
        """Tests that the window buttons are shown when opening the HUD."""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(1)
        self.assertTrue(self.panel.window_buttons_shown)

        buttons = self.panel.window_buttons.get_buttons()
        self.assertThat(len(buttons), Equals(3))
        for button in buttons:
            self.assertTrue(button.overlay_mode)

    def test_window_buttons_update_visual_state(self):
        """Tests that the window button updates its visual state."""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        button = self.panel.window_buttons.close

        self.assertThat(button.visual_state, Equals("normal"))

        button.mouse_move_to()
        self.assertTrue(button.visible)
        self.assertTrue(button.enabled)
        sleep(.25)
        self.assertThat(button.visual_state, Equals("prelight"))

        self.mouse.press()
        self.addCleanup(self.mouse.release)
        sleep(.25)
        self.assertThat(button.visual_state, Equals("pressed"))

    def test_window_buttons_cancel(self):
        """Tests how the buttons ignore clicks when the mouse is pressed over
        them and released outside their area.
        """
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        button = self.panel.window_buttons.close

        button.mouse_move_to()
        self.mouse.press()
        self.addCleanup(self.mouse.release)
        sleep(.25)
        self.panel.move_mouse_below_the_panel()
        sleep(.25)
        self.assertTrue(self.hud.visible)

    def test_window_buttons_close_button_works_for_window(self):
        """Tests that the window button 'Close' actually closes a window."""
        text_win = self.open_new_application_window("Text Editor", maximized=True, move_to_monitor=False)
        self.move_window_to_panel_monitor(text_win, restore_position=False)
        self.keybinding("window/maximize")

        self.panel.move_mouse_over_window_buttons()
        self.panel.window_buttons.close.mouse_click()
        sleep(1)

        self.assertTrue(text_win.closed)

    def test_window_buttons_close_follows_fitts_law(self):
        """Tests that the 'Close' button is activated when clicking at 0,0.

        See bug #839690
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True, move_to_monitor=False)
        self.move_window_to_panel_monitor(text_win, restore_position=False)
        self.keybinding("window/maximize")

        self.panel.move_mouse_over_window_buttons()
        screen_x, screen_y, _, _ = self.screen_geo.get_monitor_geometry(self.panel_monitor)
        self.mouse.move(screen_x, screen_y, rate=20, time_between_events=0.005)
        sleep(.5)
        self.mouse.click(press_duration=.1)
        sleep(1)

        self.assertTrue(text_win.closed)

    def test_window_buttons_minimize_button_works_for_window(self):
        """Tests that the window button 'Minimize' actually minimizes a window."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.panel.window_buttons.minimize.mouse_click()
        sleep(.5)

        self.assertTrue(text_win.is_hidden)

        icon = self.launcher.model.get_icon_by_desktop_id(text_win.application.desktop_file)
        launcher = self.launcher.get_launcher_for_monitor(self.panel_monitor)
        launcher.click_launcher_icon(icon)

    def test_window_buttons_minimize_follows_fitts_law(self):
        """Tests that the 'Minimize' button is conform to Fitts's Law.

        See bug #839690
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        button = self.panel.window_buttons.minimize
        button.mouse_move_to()
        target_x = button.x + button.width / 2
        target_y = self.screen_geo.get_monitor_geometry(self.panel_monitor)[1]
        self.mouse.move(target_x, target_y, rate=20, time_between_events=0.005)
        sleep(.5)
        self.mouse.click(press_duration=.1)
        sleep(1)

        self.assertTrue(text_win.is_hidden)
        icon = self.launcher.model.get_icon_by_desktop_id(text_win.application.desktop_file)
        launcher = self.launcher.get_launcher_for_monitor(self.panel_monitor)
        launcher.click_launcher_icon(icon)

    def test_window_buttons_unmaximize_button_works_for_window(self):
        """Tests that the window button 'Unmaximize' actually unmaximizes a window."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.panel.window_buttons.unmaximize.mouse_click()
        sleep(.5)

        self.assertFalse(text_win.is_maximized)
        self.assertTrue(text_win.is_focused)
        sleep(self.panel.menus.fadeout_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_unmaximize_follows_fitts_law(self):
        """Tests that the 'Unmaximize' button is conform to Fitts's Law.

        See bug #839690
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        button = self.panel.window_buttons.unmaximize
        button.mouse_move_to()
        target_x = button.x + button.width / 2
        target_y = self.screen_geo.get_monitor_geometry(self.panel_monitor)[1]
        self.mouse.move(target_x, target_y, rate=20, time_between_events=0.005)
        sleep(.5)
        self.mouse.click(press_duration=.1)
        sleep(1)

        self.assertFalse(text_win.is_maximized)

    def test_window_buttons_close_button_works_for_hud(self):
        """Tests that the window 'Close' actually closes the HUD."""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(.5)

        self.panel.window_buttons.close.mouse_click()
        sleep(.75)

        self.assertFalse(self.hud.visible)

    def test_window_buttons_minimize_button_disabled_for_hud(self):
        """Tests that the window 'Minimize' does nothing to the HUD."""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(.5)

        button = self.panel.window_buttons.minimize
        button.mouse_click()
        sleep(.5)

        self.assertFalse(button.enabled)
        self.assertTrue(self.hud.visible)

    def test_window_buttons_maximize_button_disabled_for_hud(self):
        """Tests that the window 'Maximize' does nothing to the HUD."""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(.5)

        button = self.panel.window_buttons.maximize
        button.mouse_click()
        sleep(.5)

        self.assertFalse(button.enabled)
        self.assertTrue(self.hud.visible)

    def test_window_buttons_maximize_in_hud_does_not_change_dash_form_factor(self):
        """Clicking on the 'Maximize' button of the HUD must do nothing.

        See bug #939054
        """
        inital_form_factor = self.dash.view.form_factor
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(.5)

        self.panel.window_buttons.maximize.mouse_click()
        sleep(.5)

        self.assertThat(self.dash.view.form_factor, Equals(inital_form_factor))

    def test_window_buttons_close_button_works_for_dash(self):
        """Tests that the window 'Close' actually closes the Dash."""
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(.5)

        self.panel.window_buttons.close.mouse_click()
        sleep(.75)

        self.assertFalse(self.dash.visible)

    def test_window_buttons_minimize_button_disabled_for_dash(self):
        """Tests that the 'Minimize' button is disabled for the dash."""
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(.5)

        button = self.panel.window_buttons.minimize
        button.mouse_click()
        sleep(.5)

        self.assertFalse(button.enabled)
        self.assertTrue(self.dash.visible)

    def test_window_buttons_maximization_buttons_works_for_dash(self):
        """'Maximize' and 'Restore' buttons (when both enabled) must work as expected."""
        self.dash.ensure_visible()
        self.addCleanup(self.panel.window_buttons.close.mouse_click)
        sleep(.5)

        unmaximize = self.panel.window_buttons.unmaximize
        maximize = self.panel.window_buttons.maximize
        netbook_dash = (self.dash.view.form_factor == "netbook")

        if netbook_dash and not unmaximize.enabled:
            unmaximize.mouse_click()
            sleep(.5)
            self.assertThat(self.dash.view.form_factor, Equals("netbook"))
        else:
            if maximize.visible:
                active_button = maximize
                unactive_button = unmaximize
            else:
                active_button = unmaximize
                unactive_button = maximize

            self.assertTrue(active_button.visible)
            self.assertTrue(active_button.sensitive)
            self.assertTrue(active_button.enabled)
            self.assertFalse(unactive_button.visible)

            self.addCleanup(unactive_button.mouse_click)
            active_button.mouse_click()
            sleep(.5)

            self.assertTrue(unactive_button.visible)
            self.assertTrue(unactive_button.sensitive)
            self.assertTrue(unactive_button.enabled)
            self.assertFalse(active_button.visible)
            sleep(.5)

            if netbook_dash:
                self.assertThat(self.dash.view.form_factor, Equals("desktop"))
            else:
                self.assertThat(self.dash.view.form_factor, Equals("netbook"))

            self.addCleanup(active_button.mouse_click)
            unactive_button.mouse_click()
            sleep(.5)

            self.assertTrue(active_button.visible)
            self.assertFalse(unactive_button.visible)

            if netbook_dash:
                self.assertThat(self.dash.view.form_factor, Equals("netbook"))
            else:
                self.assertThat(self.dash.view.form_factor, Equals("desktop"))

    def test_window_buttons_minimize_button_disabled_for_non_minimizable_windows(self):
        """Tests that if a maximized window doesn't support the minimization,
        then the 'Minimize' window button should be disabled.
        """
        text_win = self.open_new_application_window("Text Editor")
        sleep(.25)
        self.keyboard.press_and_release("Ctrl+S")
        self.addCleanup(self.keyboard.press_and_release, "Escape")

        wins = text_win.application.get_windows()
        self.assertThat(len(wins), Equals(2))
        for win in wins:
            if win.x_id != text_win.x_id:
                target_win = win
                break

        target_win.set_focus()
        self.assertTrue(target_win.is_focused)
        self.move_window_to_panel_monitor(target_win, restore_position=False)

        self.keybinding("window/maximize")

        self.assertThat(target_win.monitor, Equals(self.panel_monitor))
        self.assertTrue(target_win.is_maximized)

        self.assertTrue(self.panel.window_buttons.close.enabled)
        self.assertFalse(self.panel.window_buttons.minimize.enabled)

    def test_window_buttons_show_when_indicator_active_and_mouse_over_panel(self):
        """Tests that when an indicator is opened, and the mouse goes over the
        panel view, then the window buttons are revealed.
        """
        self.open_new_application_window("Text Editor", maximized=True)
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        indicator = self.panel.indicators.get_indicator_by_name_hint("indicator-session-devices")
        self.mouse_open_indicator(indicator)

        self.assertFalse(self.panel.window_buttons_shown)
        self.panel.move_mouse_below_the_panel()
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.assertFalse(self.panel.window_buttons_shown)
        self.panel.move_mouse_over_grab_area()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertTrue(self.panel.window_buttons_shown)

    def test_window_buttons_show_when_holding_show_menu_key(self):
        self.open_new_application_window("Text Editor", maximized=True)
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.keybinding_hold("panel/show_menus")
        self.addCleanup(self.keybinding_release, "panel/show_menus")
        sleep(1)
        self.assertTrue(self.panel.window_buttons_shown)

        self.keybinding_release("panel/show_menus")
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.assertFalse(self.panel.window_buttons_shown)


class PanelHoveringTests(PanelTestsBase):
    """Tests with the mouse pointer hovering the panel area."""

    scenarios = _make_monitor_scenarios()

    def test_only_menus_show_for_restored_window_on_mouse_in(self):
        """Tests that only menus of a restored window are shown only when
        the mouse pointer is over the panel menu area.
        """
        self.open_new_application_window("Calculator")
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.panel.move_mouse_over_window_buttons()
        sleep(self.panel.menus.fadeout_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)
        self.assertTrue(self.panel.menus_shown)

        self.panel.move_mouse_over_menus()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)
        self.assertTrue(self.panel.menus_shown)

        if self.panel.grab_area.width > 0:
            self.panel.move_mouse_over_grab_area()
            sleep(self.panel.menus.fadeout_duration / 1000.0)
            self.assertFalse(self.panel.window_buttons_shown)
            self.assertTrue(self.panel.menus_shown)

        self.panel.move_mouse_over_indicators()
        sleep(self.panel.menus.fadeout_duration / 1000.0)
        self.assertFalse(self.panel.menus_shown)

    def test_menus_show_for_maximized_window_on_mouse_in(self):
        """Tests that menus and window buttons of a maximized window are shown
        only when the mouse pointer is over the panel menu area.
        """
        self.open_new_application_window("Text Editor", maximized=True)
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.panel.move_mouse_over_window_buttons()
        sleep(self.panel.menus.fadeout_duration / 1000.0)
        self.assertTrue(self.panel.window_buttons_shown)
        self.assertTrue(self.panel.menus_shown)

        self.panel.move_mouse_over_menus()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertTrue(self.panel.window_buttons_shown)
        self.assertTrue(self.panel.menus_shown)

        if self.panel.grab_area.width > 0:
            self.panel.move_mouse_over_grab_area()
            sleep(self.panel.menus.fadeout_duration / 1000.0)
            self.assertTrue(self.panel.window_buttons_shown)
            self.assertTrue(self.panel.menus_shown)

        self.panel.move_mouse_over_indicators()
        sleep(self.panel.menus.fadeout_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)
        self.assertFalse(self.panel.menus_shown)

    def test_hovering_indicators_open_menus(self):
        """Opening an indicator entry, and then hovering on other entries must
        open them.
        """
        self.open_new_application_window("Text Editor")
        entries = self.panel.get_indicator_entries(include_hidden_menus=True)

        self.assertThat(len(entries), GreaterThan(0))
        self.mouse_open_indicator(entries[0])

        for entry in entries:
            entry.mouse_move_to()
            sleep(.25)
            self.assertTrue(entry.active)
            self.assertThat(entry.menu_y, NotEquals(0))


class PanelMenuTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def test_menus_are_added_on_new_application(self):
        """Tests that menus are added when a new application is opened."""
        self.open_new_application_window("Calculator")
        sleep(.5)
        menu_entries = self.panel.menus.get_entries()
        self.assertThat(len(menu_entries), Equals(3))

        menu_view = self.panel.menus
        self.assertThat(menu_view.get_menu_by_label("_Calculator"), NotEquals(None))
        self.assertThat(menu_view.get_menu_by_label("_Mode"), NotEquals(None))
        self.assertThat(menu_view.get_menu_by_label("_Help"), NotEquals(None))

    def test_menus_are_not_shown_if_the_application_has_no_menus(self):
        """Tests that if an application has no menus, then they are not
        shown or added.
        """
        # TODO: This doesn't test what it says on the tin. Setting MENUPROXY to ''
        # just makes the menu appear inside the app. That's fine, but it's not
        # what is described in the docstring or test id.
        old_env = os.environ["UBUNTU_MENUPROXY"]
        os.putenv("UBUNTU_MENUPROXY", "")
        self.addCleanup(os.putenv, "UBUNTU_MENUPROXY", old_env)
        calc_win = self.open_new_application_window("Calculator")
        sleep(1)

        self.assertThat(len(self.panel.menus.get_entries()), Equals(0))

        self.panel.move_mouse_over_grab_area()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertThat(self.panel.title, Equals(calc_win.application.name))

    def test_menus_shows_when_new_application_is_opened(self):
        """This tests the menu discovery feature on new application."""

        self.open_new_application_window("Calculator")
        sleep(self.panel.menus.fadein_duration / 1000.0)

        self.assertTrue(self.panel.menus_shown)

        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.assertFalse(self.panel.menus_shown)

    def test_menus_dont_show_if_a_new_application_window_is_opened(self):
        """This tests the menu discovery feature on new window for a know application."""
        self.open_new_application_window("Calculator")
        sleep(self.panel.menus.fadein_duration / 1000.0)

        self.assertTrue(self.panel.menus_shown)

        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.start_app("Calculator")
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertFalse(self.panel.menus_shown)

    def test_menus_dont_show_for_restored_window_on_mouse_out(self):
        """Restored window menus must not show when the mouse is outside the
        panel menu area.
        """
        self.open_new_application_window("Calculator")
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.assertFalse(self.panel.menus_shown)

    def test_menus_show_for_restored_window_on_mouse_in(self):
        """Restored window menus must show only when the mouse is over the panel
        menu area.
        """
        self.open_new_application_window("Calculator")
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.panel.move_mouse_over_menus()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertTrue(self.panel.menus_shown)

    def test_menus_dont_show_for_maximized_window_on_mouse_out(self):
        """Maximized window menus must not show when the mouse is outside the
        panel menu area.
        """
        self.open_new_application_window("Text Editor", maximized=True)
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.assertFalse(self.panel.menus_shown)

    def test_menus_show_for_maximized_window_on_mouse_in(self):
        """Maximized window menus must only show when the mouse is over the
        panel menu area.
        """
        self.open_new_application_window("Text Editor", maximized=True)
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.panel.move_mouse_over_menus()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertTrue(self.panel.menus_shown)

    def test_menus_dont_show_with_dash(self):
        """Tests that menus are not showing when opening the dash."""
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(1)

        self.assertFalse(self.panel.menus_shown)

    def test_menus_dont_show_with_hud(self):
        """Tests that menus are not showing when opening the HUD."""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(1)

        self.assertFalse(self.panel.menus_shown)

    def test_menus_show_after_closing_an_entry(self):
        """Opening a menu entry, and then hovering on other entries must open them.

        We also check that the menus are still drawn when closed.
        """
        # TODO - should be split into multiple tests.
        self.open_new_application_window("Calculator")
        entries = self.panel.menus.get_entries()

        self.assertThat(len(entries), GreaterThan(0))
        self.mouse_open_indicator(entries[0])

        for entry in entries:
            entry.mouse_move_to()
            sleep(.25)
            self.assertTrue(entry.active)
            self.assertThat(entry.menu_y, NotEquals(0))
            last_entry = entry

        last_entry.mouse_click()
        sleep(.25)
        self.assertFalse(last_entry.active)
        self.assertThat(last_entry.menu_y, Equals(0))

    def test_menus_show_when_indicator_active_and_mouse_over_panel(self):
        """When an indicator is opened, and the mouse goes over the panel view,
        the menus must be revealed.
        """
        self.open_new_application_window("Calculator")
        indicator = self.panel.indicators.get_indicator_by_name_hint("indicator-session-devices")
        self.mouse_open_indicator(indicator)
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.assertFalse(self.panel.menus_shown)
        self.panel.move_mouse_below_the_panel()
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.assertFalse(self.panel.menus_shown)
        self.panel.move_mouse_over_grab_area()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertTrue(self.panel.menus_shown)

    def test_menus_show_when_holding_show_menu_key(self):
        self.open_new_application_window("Calculator")
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.keybinding_hold("panel/show_menus")
        self.addCleanup(self.keybinding_release, "panel/show_menus")
        sleep(1)
        self.assertTrue(self.panel.menus_shown)

        self.keybinding_release("panel/show_menus")
        sleep(self.panel.menus.fadeout_duration / 1000.0)

        self.assertFalse(self.panel.menus_shown)


class PanelIndicatorEntriesTests(PanelTestsBase):
    """Tests for the indicator entries, including both menu and indicators."""

    scenarios = _make_monitor_scenarios()

    def test_menu_opens_on_click(self):
        """Tests that clicking on a menu entry, opens a menu."""
        self.open_new_application_window("Calculator")
        sleep(.5)

        menu_entry = self.panel.menus.get_entries()[0]
        self.mouse_open_indicator(menu_entry)

        self.assertTrue(menu_entry.active)
        self.assertThat(menu_entry.menu_x, Equals(menu_entry.x))
        self.assertThat(menu_entry.menu_y, Equals(self.panel.height))

    def test_menu_opens_closes_on_click(self):
        """Clicking on an open menu entru must close it again."""
        self.open_new_application_window("Calculator")

        menu_entry = self.panel.menus.get_entries()[0]
        self.mouse_open_indicator(menu_entry)

        self.mouse.click()
        sleep(.25)
        self.assertFalse(menu_entry.active)
        self.assertThat(menu_entry.menu_x, Equals(0))
        self.assertThat(menu_entry.menu_y, Equals(0))

    def test_menu_closes_on_click_outside(self):
        """Clicking outside an open menu must close it."""
        self.open_new_application_window("Calculator")

        menu_entry = self.panel.menus.get_entries()[0]
        self.mouse_open_indicator(menu_entry)

        self.assertTrue(menu_entry.active)
        target_x = menu_entry.menu_x + menu_entry.menu_width/2
        target_y = menu_entry.menu_y + menu_entry.menu_height + 10
        self.mouse.move(target_x, target_y)
        self.mouse.click()
        sleep(.5)

        self.assertFalse(menu_entry.active)
        self.assertThat(menu_entry.menu_x, Equals(0))
        self.assertThat(menu_entry.menu_y, Equals(0))


class PanelKeyNavigationTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def test_panel_first_menu_show_works(self):
        """Pressing the open-menus keybinding must open the first indicator."""
        self.open_new_application_window("Calculator")
        sleep(1)
        self.keybinding("panel/open_first_menu")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(1)

        open_indicator = self.panel.get_active_indicator()
        expected_indicator = self.panel.get_indicator_entries(include_hidden_menus=True)[0]
        self.assertThat(open_indicator, NotEquals(None))
        self.assertThat(open_indicator.entry_id, Equals(expected_indicator.entry_id))

        self.keybinding("panel/open_first_menu")
        sleep(.5)
        self.assertThat(self.panel.get_active_indicator(), Equals(None))

    def test_panel_menu_accelerators_work(self):
        self.open_new_application_window("Calculator")
        sleep(1)
        self.keyboard.press_and_release("Alt+c")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(.5)

        open_indicator = self.panel.get_active_indicator()
        self.assertThat(open_indicator, NotEquals(None))
        self.assertThat(open_indicator.label, Equals("_Calculator"))

    def test_panel_indicators_key_navigation_next_works(self):
        self.open_new_application_window("Calculator")
        available_indicators = self.panel.get_indicator_entries(include_hidden_menus=True)
        sleep(1)

        self.keybinding("panel/open_first_menu")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(1)

        open_indicator = self.panel.get_active_indicator()
        expected_indicator = available_indicators[0]
        self.assertThat(open_indicator.entry_id, Equals(expected_indicator.entry_id))
        sleep(.5)

        self.keybinding("panel/next_indicator")
        open_indicator = self.panel.get_active_indicator()
        expected_indicator = available_indicators[1]
        sleep(.5)
        self.assertThat(open_indicator.entry_id, Equals(expected_indicator.entry_id))

    def test_panel_indicators_key_navigation_prev_works(self):
        self.open_new_application_window("Calculator")
        available_indicators = self.panel.get_indicator_entries(include_hidden_menus=True)
        sleep(1)

        self.keybinding("panel/open_first_menu")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(1)

        open_indicator = self.panel.get_active_indicator()
        expected_indicator = available_indicators[0]
        self.assertThat(open_indicator.entry_id, Equals(expected_indicator.entry_id))
        sleep(.5)

        self.keybinding("panel/prev_indicator")
        open_indicator = self.panel.get_active_indicator()
        expected_indicator = available_indicators[-1]
        sleep(.5)
        self.assertThat(open_indicator.entry_id, Equals(expected_indicator.entry_id))

    def test_mouse_does_not_break_key_navigation(self):
        self.open_new_application_window("Calculator")
        available_indicators = self.panel.get_indicator_entries(include_hidden_menus=True)
        sleep(1)

        self.keybinding("panel/open_first_menu")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(1)

        available_indicators[2].mouse_move_to()
        self.addCleanup(self.panel.move_mouse_below_the_panel)
        sleep(.25)
        self.assertTrue(available_indicators[2].active)
        sleep(1)

        self.keybinding("panel/prev_indicator")
        self.assertTrue(available_indicators[1].active)


class PanelGrabAreaTests(PanelTestsBase):
    """Panel grab area tests."""

    scenarios = _make_monitor_scenarios()

    def move_mouse_over_grab_area(self):
        self.panel.move_mouse_over_grab_area()
        self.addCleanup(self.panel.move_mouse_below_the_panel)
        sleep(.1)

    def test_unmaximize_from_grab_area_works(self):
        """Dragging a window down from the panel must unmaximize it."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.move_mouse_over_grab_area()
        self.mouse.press()
        self.panel.move_mouse_below_the_panel()
        self.mouse.release()
        sleep(.5)

        self.assertFalse(text_win.is_maximized)

    def test_focus_the_maximized_window_works(self):
        """Clicking on the grab area must put a maximized window in focus."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)
        sleep(.5)
        self.open_new_application_window("Calculator")
        self.assertFalse(text_win.is_focused)

        self.move_mouse_over_grab_area()
        self.mouse.click()
        sleep(.5)

        self.assertTrue(text_win.is_focused)

    def test_lower_the_maximized_window_works(self):
        """Middle-clicking on the panel grab area must lower a maximized window."""
        calc_win = self.open_new_application_window("Calculator")
        sleep(.5)
        self.open_new_application_window("Text Editor", maximized=True)
        self.assertFalse(calc_win.is_focused)

        self.move_mouse_over_grab_area()
        self.mouse.click(2)
        sleep(.5)

        self.assertTrue(calc_win.is_focused)


class PanelCrossMonitorsTests(PanelTestsBase):
    """Multimonitor panel tests."""

    def setUp(self):
        super(PanelCrossMonitorsTests, self).setUp()
        if self.screen_geo.get_num_monitors() < 2:
            self.skipTest("This test requires a multimonitor setup")

    def test_panel_title_updates_moving_window(self):
        """Tests the title shown in the panel, moving a restored window around them."""
        calc_win = self.open_new_application_window("Calculator")

        prev_monitor = -1
        for monitor in range(0, self.screen_geo.get_num_monitors()):
            if calc_win.monitor != monitor:
                self.addCleanup(self.screen_geo.drag_window_to_monitor, calc_win, calc_win.monitor)
                self.screen_geo.drag_window_to_monitor(calc_win, monitor)
                sleep(.25)

            if prev_monitor >= 0:
                self.assertFalse(self.panels.get_panel_for_monitor(prev_monitor).active)

            panel = self.panels.get_panel_for_monitor(monitor)
            self.assertTrue(panel.active)
            self.assertThat(panel.title, Equals(calc_win.application.name))

            prev_monitor = monitor

    def test_window_buttons_dont_show_for_maximized_window_on_mouse_in(self):
        """Window buttons must not show when the mouse is hovering the panel in
        other monitors.
        """
        self.open_new_application_window("Text Editor", maximized=True)

        sleep(self.panel.menus.discovery_duration)

        for monitor in range(0, self.screen_geo.get_num_monitors()):
            panel = self.panels.get_panel_for_monitor(monitor)
            panel.move_mouse_over_window_buttons()
            sleep(self.panel.menus.fadein_duration / 1000.0)

            if self.panel_monitor == monitor:
                self.assertTrue(panel.window_buttons_shown)
            else:
                self.assertFalse(panel.window_buttons_shown)

    def test_window_buttons_dont_show_in_other_monitors_when_dash_is_open(self):
        """Window buttons must not show on the panels other than the one where
        the dash is opened.
        """
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)

        for monitor in range(0, self.screen_geo.get_num_monitors()):
            panel = self.panels.get_panel_for_monitor(monitor)

            if self.dash.monitor == monitor:
                self.assertTrue(panel.window_buttons_shown)
            else:
                self.assertFalse(panel.window_buttons_shown)

    def test_window_buttons_dont_show_in_other_monitors_when_hud_is_open(self):
        """Window buttons must not show on the panels other than the one where
        the hud is opened.
        """
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)

        for monitor in range(0, self.screen_geo.get_num_monitors()):
            panel = self.panels.get_panel_for_monitor(monitor)

            if self.hud.monitor == monitor:
                self.assertTrue(panel.window_buttons_shown)
            else:
                self.assertFalse(panel.window_buttons_shown)

    def test_window_buttons_close_inactive_when_clicked_in_another_monitor(self):
        """Clicking the close button must not affect the active maximized window on another monitor.

        See bug #865701
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        for monitor in range(self.screen_geo.get_num_monitors()):
            panel = self.panels.get_panel_for_monitor(monitor)

            if monitor != text_win.monitor:
                panel.window_buttons.close.mouse_click()
                sleep(.25)
                self.assertFalse(text_win.closed)

    def test_window_buttons_minimize_inactive_when_clicked_in_another_monitor(self):
        """Clicking the minimise button must not affect the active maximized window on another monitor.

        See bug #865701
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        for monitor in range(self.screen_geo.get_num_monitors()):
            panel = self.panels.get_panel_for_monitor(monitor)

            if monitor != text_win.monitor:
                panel.window_buttons.minimize.mouse_click()
                sleep(.25)
                self.assertFalse(text_win.is_hidden)

    def test_window_buttons_unmaximize_inactive_when_clicked_in_another_monitor(self):
        """Clicking the restore button must not affect the active maximized window on another monitor.

        See bug #865701
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        for monitor in range(0, self.screen_geo.get_num_monitors()):
            panel = self.panels.get_panel_for_monitor(monitor)

            if monitor != text_win.monitor:
                panel.window_buttons.unmaximize.mouse_click()
                sleep(.25)
                self.assertTrue(text_win.is_maximized)

    def test_hovering_indicators_on_multiple_monitors(self):
        """Opening an indicator entry and then hovering others entries must open them."""
        text_win = self.open_new_application_window("Text Editor")
        panel = self.panels.get_panel_for_monitor(text_win.monitor)
        indicator = panel.indicators.get_indicator_by_name_hint("indicator-session-devices")
        self.mouse_open_indicator(indicator)

        for monitor in range(0, self.screen_geo.get_num_monitors()):
            panel = self.panels.get_panel_for_monitor(monitor)

            entries = panel.get_indicator_entries(include_hidden_menus=True)
            self.assertThat(len(entries), GreaterThan(0))

            for entry in entries:
                entry.mouse_move_to()
                sleep(.5)

                if monitor != self.panel_monitor and entry.type == "menu":
                    self.assertFalse(entry.active)
                    self.assertFalse(entry.visible)
                    self.assertThat(entry.menu_y, Equals(0))
                else:
                    self.assertTrue(entry.visible)
                    self.assertTrue(entry.active)
                    self.assertThat(entry.menu_y, NotEquals(0))
