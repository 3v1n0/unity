# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import logging
import os
from testtools.matchers import Equals, NotEquals, LessThan, GreaterThan
from time import sleep

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.X11 import ScreenGeometry

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

    def open_new_application_window(self, app_name, maximize=False):
        """Opens a new instance of the requested application, ensuring
        that only one window is opened.

        Returns the opened BamfWindow
        
        """
        self.close_all_app(app_name)
        app = self.start_app(app_name)

        wins = app.get_windows()
        self.assertThat(len(wins), Equals(1))
        app_win = wins[0]

        app_win.set_focus()
        self.assertTrue(app.is_active)
        self.assertTrue(app_win.is_focused)
        self.assertThat(app.desktop_file, Equals(app_win.application.desktop_file))

        if app_win.monitor != self.panel_monitor:
            if app_win.is_maximized:
                self.addCleanup(self.keybinding, "window/maximize")
                self.keybinding("window/restore")
                sleep(.1)

            self.addCleanup(self.screen_geo.drag_window_to_monitor, app_win, app_win.monitor)
            self.screen_geo.drag_window_to_monitor(app_win, self.panel_monitor)
            sleep(.25)
            self.assertThat(app_win.monitor, Equals(self.panel_monitor))

        if maximize:
            self.keybinding("window/maximize")
            self.addCleanup(self.keybinding, "window/restore")

        app_win.set_focus()

        self.assertThat(app_win.is_maximized, Equals(maximize))

        return app_win

class PanelTitleTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def test_panel_title_on_empty_desktop(self):
        sleep(1)
        self.keybinding("window/show_desktop")
        self.addCleanup(self.keybinding, "window/show_desktop")
        sleep(1)

        self.assertTrue(self.panel.desktop_is_active)

    def test_panel_title_with_restored_application(self):
        """Tests the title shown in the panel with a restored application"""
        calc_win = self.open_new_application_window("Calculator")

        self.assertFalse(calc_win.is_maximized)
        self.assertThat(self.panel.title, Equals(calc_win.application.name))

    def test_panel_title_with_maximized_application(self):
        """Tests the title shown in the panel with a maximized application"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))

    def test_panel_title_with_maximized_window_restored_child(self):
        """Tests the title shown in the panel when opening the restored child of
        a maximized application
        """
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        self.assertTrue(text_win.is_maximized)
        self.assertThat(self.panel.title, Equals(text_win.title))

        self.keyboard.press_and_release("Ctrl+S")
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        sleep(.25)
        self.assertThat(self.panel.title, Equals(text_win.application.name))

    def test_panel_title_switching_active_window(self):
        """Tests the title shown in the panel with a maximized application"""
        # Locked Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', 0)

        text_win = self.open_new_application_window("Text Editor", maximize=True)

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
        window title changes"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)

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
    #panel_monitor = 1

    def setUp(self):
        super(PanelWindowButtonsTests, self).setUp()
        # Locked Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', 0)

    def test_window_buttons_dont_show_on_empty_desktop(self):
        """Tests that the window buttons are not shown on clean desktop"""
        sleep(1)
        self.keybinding("window/show_desktop")
        self.addCleanup(self.keybinding, "window/show_desktop")
        sleep(1)

        self.assertFalse(self.panel.window_buttons_shown)
        self.panel.move_mouse_over_window_buttons()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_dont_show_for_restored_window(self):
        """Tests that the window buttons are not shown for a restored window"""
        calc_win = self.open_new_application_window("Calculator")

        self.assertFalse(self.panel.window_buttons_shown)

        self.panel.move_mouse_over_window_buttons()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_dont_show_for_maximized_window_on_mouse_out(self):
        """Tests that the windows button arenot shown for a maximized window
        when the mouse is outside the panel
        """
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.discovery_fadein_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_show_for_maximized_window_on_mouse_in(self):
        """Tests that the window buttons are shown when a maximized window
        is focused and the mouse is over the whitelisted panel areas
        """
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        sleep(self.panel.menus.discovery_duration)

        self.panel.move_mouse_over_window_buttons()
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.assertTrue(self.panel.window_buttons_shown)

        self.panel.move_mouse_over_menus()
        sleep(self.panel.menus.fadeout_duration / 1000.0)
        self.assertTrue(self.panel.window_buttons_shown)

        if self.panel.grab_area.width > 0:
            self.panel.move_mouse_over_grab_area()
            sleep(self.panel.menus.fadeout_duration / 1000.0)
            self.assertTrue(self.panel.window_buttons_shown)

        self.panel.move_mouse_over_indicators()
        sleep(self.panel.menus.fadeout_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_show_with_dash(self):
        """Tests that the window buttons are shown when opening the dash"""
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(.5)
        self.assertTrue(self.panel.window_buttons_shown)

    def test_window_buttons_show_with_hud(self):
        """Tests that the window buttons are shown when opening the HUD"""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(.5)
        self.assertTrue(self.panel.window_buttons_shown)

    def test_window_buttons_update_visual_state(self):
        """Tests that the window button updates its visual state"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)
        button = self.panel.window_buttons.close

        self.assertThat(button.visual_state, Equals("normal"))

        button.mouse_move_to()
        self.assertTrue(button.visible)
        self.assertTrue(button.enabled)
        sleep(.25)
        self.assertThat(button.visual_state, Equals("prelight"))

        self.mouse.press()
        self.addCleanup(self.mouse.release)
        self.addCleanup(self.panel.move_mouse_below_the_panel)
        sleep(.25)
        self.assertThat(button.visual_state, Equals("pressed"))

    def test_window_buttons_close_button_works_for_window(self):
        """Tests that the window button 'Close' actually closes a window"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        self.panel.window_buttons.close.mouse_click()
        sleep(.5)

        self.assertTrue(text_win.closed)

    def test_window_buttons_minimize_button_works_for_window(self):
        """Tests that the window button 'Minimize' actually minimizes a window"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        self.panel.window_buttons.minimize.mouse_click()
        sleep(.5)

        self.assertTrue(text_win.is_hidden)

    def test_window_buttons_unmaximize_button_works_for_window(self):
        """Tests that the window button 'Unmaximize' actually unmaximizes a window"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        self.panel.window_buttons.unmaximize.mouse_click()
        sleep(.5)

        self.assertFalse(text_win.is_maximized)
        self.assertTrue(text_win.is_focused)
        sleep(self.panel.menus.fadeout_duration / 1000.0)
        self.assertFalse(self.panel.window_buttons_shown)

    def test_window_buttons_close_button_works_for_hud(self):
        """Tests that the window 'Close' actually closes the HUD"""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(.5)

        self.panel.window_buttons.close.mouse_click()
        sleep(.75)

        self.assertFalse(self.hud.visible)

    def test_window_buttons_minimize_button_disabled_for_hud(self):
        """Tests that the window 'Minimize' does nothing to the HUD"""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(.5)

        button = self.panel.window_buttons.minimize
        button.mouse_click()
        sleep(.5)

        self.assertFalse(button.enabled)
        self.assertTrue(self.hud.visible)

    def test_window_buttons_maximize_button_disabled_for_hud(self):
        """Tests that the window 'Maximize' does nothing to the HUD"""
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(.5)

        button = self.panel.window_buttons.maximize
        button.mouse_click()
        sleep(.5)

        self.assertFalse(button.enabled)
        self.assertTrue(self.hud.visible)

    def test_window_buttons_close_button_works_for_dash(self):
        """Tests that the window 'Close' actually closes the Dash"""
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(.5)

        self.panel.window_buttons.close.mouse_click()
        sleep(.75)

        self.assertFalse(self.dash.visible)

    def test_window_buttons_minimize_button_disabled_for_dash(self):
        """Tests that the 'Minimize' button is disabled for the dash"""
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(.5)

        button = self.panel.window_buttons.minimize
        button.mouse_click()
        sleep(.5)

        self.assertFalse(button.enabled)
        self.assertTrue(self.dash.visible)

    def test_window_buttons_maximization_buttons_works_for_dash(self):
        """Tests that the 'Maximize' and 'Unmaximize' buttons (when
        both enabled) work as expected"""
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


class PanelTitleCrossMonitorsTests(PanelTestsBase):

    scenarios = []
    if ScreenGeometry().get_num_monitors() > 1:
        scenarios = [("Multimonitor only tests", {})]

    def test_panel_title_updates_moving_window(self):
        """Tests the title shown in the panel, moving a restored window around them"""
        calc_win = self.open_new_application_window("Calculator")

        prev_monitor = -1
        for monitor in range(0, self.screen_geo.get_num_monitors()):
            if calc_win.monitor != monitor:
                self.addCleanup(self.screen_geo.drag_window_to_monitor, calc_win, calc_win.monitor)
                self.screen_geo.drag_window_to_monitor(calc_win, monitor)

            if prev_monitor >= 0:
                self.assertFalse(self.panels.get_panel_for_monitor(prev_monitor).active)

            panel = self.panels.get_panel_for_monitor(monitor)
            self.assertTrue(panel.active)
            self.assertThat(panel.title, Equals(calc_win.application.name))

            prev_monitor = monitor

    def test_window_buttons_dont_show_for_maximized_window_on_mouse_in(self):
        """Test that window buttons are not showing when the mouse is hovering
        the panel in other monitors"""
        text_win = self.open_new_application_window("Text Editor", maximize=True)

        sleep(self.panel.menus.discovery_duration)

        for monitor in range(0, self.screen_geo.get_num_monitors()):
            panel = self.panels.get_panel_for_monitor(monitor)
            panel.move_mouse_over_window_buttons()
            sleep(self.panel.menus.fadein_duration / 1000.0)

            if self.panel_monitor == monitor:
                self.assertTrue(panel.window_buttons_shown)
            else:
                self.assertFalse(panel.window_buttons_shown)
