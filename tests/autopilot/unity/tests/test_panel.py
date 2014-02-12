# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.display import Display
#from autopilot.emulators.bamf import BamfWindow
from autopilot.process import Window
from autopilot.matchers import Eventually
import logging
import os
from testtools.matchers import Equals,  GreaterThan, NotEquals
from time import sleep

from unity.emulators.panel import IndicatorEntry
from unity.emulators.X11 import drag_window_to_screen
from unity.tests import UnityTestCase

import gettext

logger = logging.getLogger(__name__)


def _make_monitor_scenarios():
    num_monitors = Display.create().get_num_screens()
    scenarios = []

    if num_monitors == 1:
        scenarios = [('Single Monitor', {'panel_monitor': 0})]
    else:
        for i in range(num_monitors):
            scenarios += [('Monitor %d' % (i), {'panel_monitor': i})]

    return scenarios


class PanelTestsBase(UnityTestCase):

    panel_monitor = 0

    def setUp(self):
        super(PanelTestsBase, self).setUp()
        self.panel = self.unity.panels.get_panel_for_monitor(self.panel_monitor)
        self.panel.move_mouse_below_the_panel()
        self.addCleanup(self.panel.move_mouse_below_the_panel)

    def open_new_application_window(self, app_name, maximized=False, move_to_monitor=True):
        """Opens a new instance of the requested application, ensuring that only
        one window is opened.

        Returns the opened BamfWindow

        """
        self.process_manager.close_all_app(app_name)
        app_win = self.process_manager.start_app_window(app_name, locale="C")
        app = app_win.application

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

    def move_window_to_panel_monitor(self, window, restore_position=False):
        """Drags a window to another monitor, eventually restoring it before"""
        if not isinstance(window, Window):
            raise TypeError("Window must be a autopilot.process.Window")

        if window.monitor == self.panel_monitor:
            return

        if window.is_maximized:
            self.keybinding("window/restore")
            self.addCleanup(self.keybinding, "window/maximize")
            sleep(.1)

        if restore_position:
            self.addCleanup(drag_window_to_screen, window, window.monitor)

        drag_window_to_screen(window, self.panel_monitor)
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
        self.assertThat(indicator.active, Eventually(Equals(True)))

    def assertWinButtonsInOverlayMode(self, overlay_mode):
        """Assert that there are three panel window buttons and all of them are
        in the specified overlay mode.

        """
        if type(overlay_mode) is not bool:
            raise TypeError("overlay_mode must be True or False")

        buttons = self.panel.window_buttons.get_buttons()
        self.assertThat(len(buttons), Equals(3))
        for button in buttons:
            self.assertThat(button.overlay_mode, Eventually(Equals(overlay_mode)))

    def assertNoWindowOpenWithXid(self, x_id):
        """Assert that Bamf doesn't know of any open windows with the given xid."""
        # We can't check text_win.closed since we've just destroyed the window.
        # Instead we make sure no window with it's x_id exists.
        refresh_fn = lambda: [w for w in self.process_manager.get_open_windows() if w.x_id == x_id]
        self.assertThat(refresh_fn, Eventually(Equals([])))

    def sleep_menu_settle_period(self):
        """Sleep long enough for the menus to fade in and fade out again."""
        sleep(self.panel.menus.fadein_duration / 1000.0)
        sleep(self.panel.menus.discovery_duration)
        sleep(self.panel.menus.fadeout_duration / 1000.0)

    # Unable to exit SDM without any active apps, need a placeholder.
    # See bug LP:1079460
    def start_placeholder_app(self):
        window_spec = {
            "Title": "Placeholder application",
        }
        self.launch_test_window(window_spec)


class PanelTitleTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def test_panel_title_on_empty_desktop(self):
        """With no windows shown, the panel must display the default title."""
        gettext.install("unity", unicode=True)
        self.start_placeholder_app()
        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        self.assertThat(self.panel.desktop_is_active, Eventually(Equals(True)))
        self.assertThat(self.panel.title, Equals(_("Ubuntu Desktop")))

    def test_panel_title_with_restored_application(self):
        """Panel must display application name for a non-maximised application."""
        calc_win = self.open_new_application_window("Calculator", maximized=False)

        self.assertThat(self.panel.title, Eventually(Equals(calc_win.application.name)))

    def test_panel_title_with_maximized_application(self):
        """Panel must display application name for a maximised application."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.assertThat(self.panel.title, Eventually(Equals(text_win.title)))

    def test_panel_title_with_maximized_window_restored_child(self):
        """Tests the title shown in the panel when opening the restored child of
        a maximized application.
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        # Ctrl+h opens the replace dialog.
        self.keyboard.press_and_release("Ctrl+h")
        self.addCleanup(self.keyboard.press_and_release, "Escape")

        self.assertThat(lambda: len(text_win.application.get_windows()),
                        Eventually(Equals(2)))
        self.assertThat(self.panel.title, Equals(text_win.application.name))

    def test_panel_shows_app_title_with_maximised_app(self):
        """Tests app titles are shown in the panel with a non-focused maximized application."""
        self.open_new_application_window("Text Editor", maximized=True)
        calc_win = self.open_new_application_window("Calculator", maximized=False)

        self.assertThat(self.panel.title, Eventually(Equals(calc_win.application.name)))

    def test_panel_title_updates_when_switching_to_maximized_app(self):
        """Switching to a maximised app from a restored one must update the panel title."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)
        self.open_new_application_window("Calculator", maximized=False)

        icon = self.unity.launcher.model.get_icon(desktop_id=text_win.application.desktop_file)
        launcher = self.unity.launcher.get_launcher_for_monitor(self.panel_monitor)
        launcher.click_launcher_icon(icon)

        self.assertProperty(text_win, is_focused=True)
        self.assertThat(self.panel.title, Eventually(Equals(text_win.title)))

    def test_panel_title_updates_on_maximized_window_title_changes(self):
        """Panel title must change when the title of a maximized application changes."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)
        old_title = text_win.title

        text_win.set_focus()
        self.keyboard.press_and_release("Ctrl+n")

        self.assertThat(lambda: text_win.title, Eventually(NotEquals(old_title)))
        self.assertThat(self.panel.title, Eventually(Equals(text_win.title)))

    def test_panel_title_doesnt_change_with_switcher(self):
        """Switching between apps must not change the Panels title."""
        calc_win = self.open_new_application_window("Calculator")
        text_win = self.open_new_application_window("Text Editor")
        current_title = self.panel.title

        self.unity.switcher.initiate()
        self.addCleanup(self.unity.switcher.terminate)
        self.unity.switcher.next_icon()

        self.assertThat(self.panel.title,
                        Eventually(Equals(current_title)))


class PanelWindowButtonsTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def setUp(self):
        super(PanelWindowButtonsTests, self).setUp()
        # Locked Launchers on all monitors
        self.set_unity_option('num_launchers', 0)
        self.set_unity_option('launcher_hide_mode', 0)

    def test_window_buttons_dont_show_on_empty_desktop(self):
        """Tests that the window buttons are not shown on clean desktop."""
        self.start_placeholder_app()
        self.unity.window_manager.enter_show_desktop()
        self.addCleanup(self.unity.window_manager.leave_show_desktop)

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

        self.panel.move_mouse_over_window_buttons()
        # Sleep twice as long as the timeout, just to be sure. timeout is in
        # mS, we need seconds, hence the divide by 500.0
        sleep(self.panel.menus.fadein_duration / 500.0)
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_window_buttons_dont_show_for_restored_window(self):
        """Window buttons must not show for a restored window."""
        self.open_new_application_window("Calculator")
        self.panel.move_mouse_below_the_panel()

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_window_buttons_dont_show_for_restored_window_with_mouse_in_panel(self):
        """Window buttons must not show for a restored window with the mouse in
        the panel."""
        self.open_new_application_window("Calculator")
        self.panel.move_mouse_over_window_buttons()

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_window_buttons_dont_show_for_maximized_window_on_mouse_out(self):
        """Window buttons must not show for a maximized window when the mouse is
        outside the panel.
        """
        self.open_new_application_window("Text Editor", maximized=True)

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_window_buttons_show_for_maximized_window_on_mouse_in(self):
        """Window buttons must show when a maximized window is focused and the
        mouse is over the menu-view panel areas.

        """
        self.open_new_application_window("Text Editor", maximized=True)
        self.panel.move_mouse_over_window_buttons()

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(True)))
        self.assertWinButtonsInOverlayMode(False)

    def test_window_buttons_show_with_dash(self):
        """Window buttons must be shown when the dash is open."""
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        self.assertThat(self.unity.dash.view.overlay_window_buttons_shown, Eventually(Equals(True)))

    def test_window_buttons_work_in_dash_after_launcher_resize(self):
        """When the launcher icons are resized, the window
        buttons must still work in the dash."""

        self.set_unity_option("icon_size", 25)
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        desired_max = not self.unity.dash.view.dash_maximized
        if desired_max:
            self.panel.window_buttons.maximize.mouse_click()
        else:
            self.panel.window_buttons.unmaximize.mouse_click()

        self.assertThat(self.unity.dash.view.dash_maximized, Eventually(Equals(desired_max)))

    def test_window_buttons_show_with_hud(self):
        """Window buttons must be shown when the HUD is open."""
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.assertThat(self.unity.hud.view.overlay_window_buttons_shown, Eventually(Equals(True)))

    def test_window_buttons_update_visual_state(self):
        """Window button must update its state in response to mouse events."""
        self.open_new_application_window("Text Editor", maximized=True, move_to_monitor=True)
        self.panel.move_mouse_over_window_buttons()
        button = self.panel.window_buttons.unmaximize

        self.assertThat(button.visual_state, Eventually(Equals("normal")))

        button.mouse_move_to()
        self.assertThat(button.visual_state, Eventually(Equals("prelight")))

        self.mouse.press()
        self.addCleanup(self.mouse.release)
        self.assertThat(button.visual_state, Eventually(Equals("pressed")))

    def test_window_buttons_cancel(self):
        """Window buttons must ignore clicks when the mouse released outside
        their area.
        """
        win = self.open_new_application_window("Text Editor", maximized=True, move_to_monitor=True)
        self.panel.move_mouse_over_window_buttons()

        button = self.panel.window_buttons.unmaximize
        button.mouse_move_to()
        self.mouse.press()
        self.assertThat(button.visual_state, Eventually(Equals("pressed")))
        self.panel.move_mouse_below_the_panel()
        self.mouse.release()

        self.assertThat(win.is_maximized, Equals(True))

    def test_window_buttons_close_button_works_for_window(self):
        """Close window button must actually closes a window."""
        text_win = self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)
        win_xid = text_win.x_id

        self.panel.window_buttons.close.mouse_click()
        self.assertNoWindowOpenWithXid(win_xid)

    def test_window_buttons_close_follows_fitts_law(self):
        """Tests that the 'Close' button is activated when clicking at 0,0.

        See bug #839690
        """
        text_win = self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)
        win_xid = text_win.x_id

        self.panel.move_mouse_over_window_buttons()
        screen_x, screen_y = self.display.get_screen_geometry(self.panel_monitor)[:2]
        self.mouse.move(screen_x, screen_y)
        self.mouse.click()

        self.assertNoWindowOpenWithXid(win_xid)

    def test_window_buttons_minimize_button_works_for_window(self):
        """Tests that the window button 'Minimize' actually minimizes a window."""
        text_win = self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)

        self.panel.window_buttons.minimize.mouse_click()

        self.assertProperty(text_win, is_hidden=True)

    def test_window_buttons_minimize_follows_fitts_law(self):
        """Tests that the 'Minimize' button is conform to Fitts's Law.

        See bug #839690
        """
        text_win = self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)

        self.panel.move_mouse_over_window_buttons()
        button = self.panel.window_buttons.minimize
        target_x = button.x + button.width / 2
        target_y = self.display.get_screen_geometry(self.panel_monitor)[1]
        self.mouse.move(target_x, target_y)
        self.mouse.click()

        self.assertProperty(text_win, is_hidden=True)

    def test_window_buttons_unmaximize_button_works_for_window(self):
        """Tests that the window button 'Unmaximize' actually unmaximizes a window."""
        text_win = self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)

        self.panel.window_buttons.unmaximize.mouse_click()

        self.assertProperties(text_win, is_maximized=False, is_focused=True)
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_window_buttons_unmaximize_follows_fitts_law(self):
        """Tests that the 'Unmaximize' button is conform to Fitts's Law.

        See bug #839690
        """
        text_win = self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)

        button = self.panel.window_buttons.unmaximize
        button.mouse_move_to()
        target_x = button.x + button.width / 2
        target_y = self.display.get_screen_geometry(self.panel_monitor)[1]
        self.mouse.move(target_x, target_y)
        sleep(1)
        self.mouse.click()

        self.assertProperty(text_win, is_maximized=False)

    def test_window_buttons_close_button_works_for_hud(self):
        """Tests that the window 'Close' actually closes the HUD."""
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.panel.window_buttons.close.mouse_click()
        self.assertThat(self.unity.hud.visible, Eventually(Equals(False)))

    def test_minimize_button_disabled_for_hud(self):
        """Minimize button must be disabled for the HUD."""
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.assertThat(self.panel.window_buttons.minimize.enabled, Eventually(Equals(False)))

    def test_minimize_button_does_nothing_for_hud(self):
        """Minimize button must not affect the Hud."""
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.panel.window_buttons.minimize.mouse_click()

        self.assertThat(self.unity.hud.visible, Eventually(Equals(True)))

    def test_maximize_button_disabled_for_hud(self):
        """Maximize button must be disabled for the HUD."""
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.assertThat(self.panel.window_buttons.maximize.enabled, Eventually(Equals(False)))

    def test_maximize_button_does_nothing_for_hud(self):
        """Maximize button must not affect the Hud."""
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.panel.window_buttons.maximize.mouse_click()

        self.assertThat(self.unity.hud.visible, Eventually(Equals(True)))

    def test_hud_maximize_button_does_not_change_dash_form_factor(self):
        """Clicking on the 'Maximize' button of the HUD must not change the dash
        layout.

        See bug #939054
        """
        inital_form_factor = self.unity.dash.view.form_factor
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.panel.window_buttons.maximize.mouse_click()
        # long sleep here to make sure that any change that might happen will
        # have already happened.
        sleep(5)
        self.assertThat(self.unity.dash.view.form_factor, Equals(inital_form_factor))

    def test_window_buttons_close_button_works_for_dash(self):
        """Tests that the window 'Close' actually closes the Dash."""
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)
        self.panel.window_buttons.close.mouse_click()

        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))

    def test_minimize_button_disabled_for_dash(self):
        """Tests that the 'Minimize' button is disabled for the dash."""
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        self.assertThat(self.panel.window_buttons.minimize.enabled, Eventually(Equals(False)))

    def test_minimize_button_does_nothing_for_dash(self):
        """Tests that the 'Minimize' button is disabled for the dash."""
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        self.panel.window_buttons.minimize.mouse_click()
        sleep(5)
        self.assertThat(self.unity.dash.visible, Eventually(Equals(True)))

    def test_window_buttons_maximize_or_restore_dash(self):
        """Tests that the Maximize/Restore button works for the dash."""

        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        desired_max = not self.unity.dash.view.dash_maximized
        if desired_max:
            self.panel.window_buttons.maximize.mouse_click()
        else:
            self.panel.window_buttons.unmaximize.mouse_click()

        self.assertThat(self.unity.dash.view.dash_maximized, Eventually(Equals(desired_max)))

    def test_window_buttons_active_inactive_states(self):
        """Tests that the maximized/restore buttons are in the correct state when the
        dash is open. Asserting states: visible, sensitive, enabled.
        """

        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        unmaximize = self.unity.dash.view.window_buttons.unmaximize
        maximize = self.unity.dash.view.window_buttons.maximize

        desired_max = not self.unity.dash.view.dash_maximized
        if desired_max:
            active   = maximize
            inactive = unmaximize
        else:
            active   = unmaximize
            inactive = maximize

        self.assertThat(active.visible, Eventually(Equals(True)))
        self.assertThat(active.sensitive, Eventually(Equals(True)))
        self.assertThat(active.enabled, Eventually(Equals(True)))
        self.assertThat(inactive.visible, Eventually(Equals(False)))

    def test_window_buttons_state_switch_on_click(self):
        """Tests that when clicking the maximize/restore window button it
        switchs its state from either maximize to restore, or restore to
        maximize.
        """

        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        unmaximize = self.unity.dash.view.window_buttons.unmaximize
        maximize = self.unity.dash.view.window_buttons.maximize

        desired_max = not self.unity.dash.view.dash_maximized
        if desired_max:
            active   = maximize
            inactive = unmaximize
        else:
            active   = unmaximize
            inactive = maximize

        active.mouse_click()

        self.assertThat(inactive.visible, Eventually(Equals(True)))
        self.assertThat(inactive.sensitive, Eventually(Equals(True)))
        self.assertThat(inactive.enabled, Eventually(Equals(True)))
        self.assertThat(active.visible, Eventually(Equals(False)))
        self.assertThat(self.unity.dash.view.dash_maximized, Eventually(Equals(desired_max)))

    def test_minimize_button_disabled_for_non_minimizable_windows(self):
        """Minimize button must be disabled for windows that don't support minimization."""
        text_win = self.open_new_application_window("Text Editor",
            maximized=False,
            move_to_monitor=True)

        self.keyboard.press_and_release("Ctrl+S")
        self.addCleanup(self.keyboard.press_and_release, "Escape")

        wins = text_win.application.get_windows()
        self.assertThat(len(wins), Equals(2))
        [target_win] = [w for w in wins if w.x_id != text_win.x_id]
        self.move_window_to_panel_monitor(target_win, restore_position=False)

        self.keybinding("window/maximize")
        self.assertProperty(target_win, is_maximized=True)

        self.assertThat(self.panel.window_buttons.close.enabled, Eventually(Equals(True)))
        self.assertThat(self.panel.window_buttons.minimize.enabled, Eventually(Equals(False)))

    def test_window_buttons_show_when_indicator_active_and_mouse_over_panel(self):
        """Window buttons must be shown when mouse is over panel area with an
        indicator open.
        """
        self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)

        indicator = self.panel.indicators.get_indicator_by_name_hint("indicator-session")
        self.mouse_open_indicator(indicator)
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

        self.panel.move_mouse_below_the_panel()
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

        self.panel.move_mouse_over_grab_area()
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(True)))

    def test_window_buttons_show_when_holding_show_menu_key(self):
        """Window buttons must show when we press the show-menu keybinding."""
        self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)

        self.sleep_menu_settle_period()
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

        self.keybinding_hold("panel/show_menus")
        self.addCleanup(self.keybinding_release, "panel/show_menus")

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(True)))

        self.keybinding_release("panel/show_menus")
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_window_buttons_cant_accept_keynav_focus(self):
        """On a mouse down event over the window buttons
        you must still be able to type into the Hud.

        """
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.keyboard.type("Hello")
        self.panel.window_buttons.minimize.mouse_click()
        self.keyboard.type("World")

        self.assertThat(self.unity.hud.search_string, Eventually(Equals("HelloWorld")))

    def test_double_click_unmaximize_window(self):
        """Double clicking the grab area must unmaximize a maximized window."""
        gedit_win = self.open_new_application_window("Text Editor", maximized=True)

        self.panel.move_mouse_over_grab_area()
        self.mouse.click()
        self.mouse.click()

        self.assertThat(self.panel.title, Eventually(Equals(gedit_win.application.name)))


class PanelHoverTests(PanelTestsBase):
    """Tests with the mouse pointer hovering the panel area."""

    scenarios = _make_monitor_scenarios()

    def test_only_menus_show_for_restored_window_on_mouse_in_window_btn_area(self):
        """Restored windows should only show menus when the mouse is in the window
        button area.
        """
        self.open_new_application_window("Calculator",
            maximized=False,
            move_to_monitor=True)
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_window_buttons()

        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_only_menus_show_for_restored_window_on_mouse_in_menu_area(self):
        """Restored windows should only show menus when the mouse is in the window
        menu area.
        """
        self.open_new_application_window("Calculator",
            maximized=False,
            move_to_monitor=True)
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_menus()

        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_only_menus_show_for_restored_window_on_mouse_in_grab_area(self):
        """Restored windows should only show menus when the mouse is in the panel
        grab area.
        """
        self.open_new_application_window("Calculator",
            maximized=False,
            move_to_monitor=True)
        self.sleep_menu_settle_period()

        if self.panel.grab_area.width <= 0:
            self.skipTest("Grab area is too small to run test!")

        self.panel.move_mouse_over_grab_area()

        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))

    def test_hovering_over_indicators_does_not_show_app_menus(self):
        """Hovering the mouse over the indicators must not show app menus."""
        self.open_new_application_window("Calculator",
            maximized=False,
            move_to_monitor=True)
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_menus()
        # This assert is repeated from above, but we use it to make sure that
        # the menus are shown before we move over the indicators.
        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))

        self.panel.move_mouse_over_indicators()

        self.assertThat(self.panel.menus_shown, Eventually(Equals(False)))

    def test_menus_show_for_maximized_window_on_mouse_in_btn_area(self):
        """Menus and window buttons must be shown when the mouse is in the window
        button area for a maximised application.
        """
        self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_window_buttons()

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(True)))
        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))

    def test_menus_show_for_maximized_window_on_mouse_in_menu_area(self):
        """Menus and window buttons must be shown when the mouse is in the menu
        area for a maximised application.
        """
        self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_menus()

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(True)))
        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))

    def test_menus_show_for_maximized_window_on_mouse_in_grab_area(self):
        """Menus and window buttons must be shown when the mouse is in the grab
        area for a maximised application.
        """
        if self.panel.grab_area.width <= 0:
            self.skipTest("Grab area is too small to run this test!")

        self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_grab_area()

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(True)))
        self.assertTrue(self.panel.menus_shown, Eventually(Equals(True)))

    def test_menus_and_btns_hidden_with_mouse_over_indicators(self):
        """Hovering the mouse over the indicators must hide the menus and window
        buttons.
        """
        self.open_new_application_window("Text Editor",
            maximized=True,
            move_to_monitor=True)
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_menus()
        # We use this assert to make sure that the menus are visible before we
        # move the mouse:
        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(True)))

        self.panel.move_mouse_over_indicators()

        self.assertThat(self.panel.window_buttons_shown, Eventually(Equals(False)))
        self.assertThat(self.panel.menus_shown, Eventually(Equals(False)))

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
            self.assertThat(entry.active, Eventually(Equals(True)))
            self.assertThat(entry.menu_y, Eventually(NotEquals(0)))


class PanelMenuTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def start_test_app_with_menus(self):
        window_spec = {
            "Title": "Test Application with Menus",
            "Menu": [
                {
                    "Title": "&File",
                    "Menu": ["Open", "Save", "Save As", "Quit"]
                },
                {"Title": "&Edit"},
                {"Title": "&Quit"}
                ]
        }
        self.launch_test_window(window_spec)

    def test_menus_are_added_on_new_application(self):
        """Tests that menus are added when a new application is opened."""
        self.start_test_app_with_menus()

        refresh_fn = lambda: len(self.panel.menus.get_entries())
        self.assertThat(refresh_fn, Eventually(Equals(3)))

        menu_view = self.panel.menus
        self.assertThat(lambda: menu_view.get_menu_by_label("_File"), Eventually(NotEquals(None)))
        self.assertThat(lambda: menu_view.get_menu_by_label("_Edit"), Eventually(NotEquals(None)))
        self.assertThat(lambda: menu_view.get_menu_by_label("_Quit"), Eventually(NotEquals(None)))

    def test_menus_are_not_shown_if_the_application_has_no_menus(self):
        """Applications with no menus must not show menus in the panel."""

        test_win = self.launch_test_window()

        self.assertThat(
            lambda: len(self.panel.menus.get_entries()),
            Eventually(Equals(0)),
            "Current panel entries are: %r" % self.panel.menus.get_entries())

        self.panel.move_mouse_over_grab_area()
        self.assertThat(self.panel.title, Eventually(Equals(test_win.application.name)))

    def test_menus_shows_when_new_application_is_opened(self):
        """When starting a new application, menus must first show, then hide."""

        self.start_test_app_with_menus()

        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))
        self.sleep_menu_settle_period()
        self.assertThat(self.panel.menus_shown, Eventually(Equals(False)))

    def test_menus_dont_show_if_a_new_application_window_is_opened(self):
        """This tests the menu discovery feature on new window for a know application."""
        self.open_new_application_window("Character Map")
        self.sleep_menu_settle_period()

        self.process_manager.start_app("Character Map")
        sleep(self.panel.menus.fadein_duration / 1000.0)
        # Not using Eventually here since this is time-critical. Need to work
        # out a better way to do this.
        self.assertThat(self.panel.menus_shown, Equals(False))

    def test_menus_dont_show_for_restored_window_on_mouse_out(self):
        """Restored window menus must not show when the mouse is outside the
        panel menu area.
        """
        self.open_new_application_window("Calculator")
        self.sleep_menu_settle_period()

        self.assertThat(self.panel.menus_shown, Eventually(Equals(False)))

    def test_menus_show_for_restored_window_on_mouse_in(self):
        """Restored window menus must show only when the mouse is over the panel
        menu area.
        """
        self.open_new_application_window("Calculator")
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_menus()

        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))

    def test_menus_dont_show_for_maximized_window_on_mouse_out(self):
        """Maximized window menus must not show when the mouse is outside the
        panel menu area.
        """
        self.open_new_application_window("Text Editor", maximized=True)

        self.assertThat(self.panel.menus_shown, Eventually(Equals(False)))

    def test_menus_show_for_maximized_window_on_mouse_in(self):
        """Maximized window menus must only show when the mouse is over the
        panel menu area.
        """
        self.open_new_application_window("Text Editor", maximized=True)
        self.sleep_menu_settle_period()

        self.panel.move_mouse_over_menus()
        self.assertThat(self.panel.menus_shown, Eventually(Equals(True)))

    def test_menus_dont_show_with_dash(self):
        """Tests that menus are not showing when opening the dash."""
        self.open_new_application_window("Text Editor", maximized=True)
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        self.assertThat(self.panel.menus_shown, Eventually(Equals(False)))

    def test_menus_dont_show_with_hud(self):
        """Tests that menus are not showing when opening the HUD."""
        self.open_new_application_window("Text Editor", maximized=True)
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.assertThat(self.panel.menus_shown, Eventually(Equals(False)))


class PanelIndicatorEntryTests(PanelTestsBase):
    """Tests for the indicator entries, including both menu and indicators."""

    scenarios = _make_monitor_scenarios()

    def open_app_and_get_menu_entry(self):
        """Open the test app and wait for the menu entry to appear."""
        self.open_new_application_window("Calculator")
        refresh_fn = lambda: len(self.panel.menus.get_entries())
        self.assertThat(refresh_fn, Eventually(GreaterThan(0)))
        menu_entry = self.panel.menus.get_entries()[0]
        return menu_entry

    def test_menu_opens_on_click(self):
        """Tests that clicking on a menu entry, opens a menu."""
        menu_entry = self.open_app_and_get_menu_entry()
        self.mouse_open_indicator(menu_entry)

        self.assertThat(menu_entry.active, Eventually(Equals(True)))
        self.assertThat(menu_entry.menu_x, Eventually(Equals(menu_entry.x)))
        self.assertThat(menu_entry.menu_y, Eventually(Equals(self.panel.height)))

    def test_menu_opens_closes_on_click(self):
        """Clicking on an open menu entru must close it again."""
        menu_entry = self.open_app_and_get_menu_entry()
        self.mouse_open_indicator(menu_entry)

        # This assert is for timing purposes only:
        self.assertThat(menu_entry.active, Eventually(Equals(True)))
        # Make sure we wait at least enough time that the menu appeared as well
        sleep(self.panel.menus.fadein_duration / 1000.0)
        self.mouse.click()

        self.assertThat(menu_entry.active, Eventually(Equals(False)))
        self.assertThat(menu_entry.menu_x, Eventually(Equals(0)))
        self.assertThat(menu_entry.menu_y, Eventually(Equals(0)))

    def test_menu_closes_on_click_outside(self):
        """Clicking outside an open menu must close it."""
        menu_entry = self.open_app_and_get_menu_entry()
        self.mouse_open_indicator(menu_entry)

        # This assert is for timing purposes only:
        self.assertThat(menu_entry.active, Eventually(Equals(True)))
        target_x = menu_entry.menu_x + menu_entry.menu_width/2
        target_y = menu_entry.menu_y + menu_entry.menu_height + 10
        self.mouse.move(target_x, target_y)
        self.mouse.click()

        self.assertThat(menu_entry.active, Eventually(Equals(False)))
        self.assertThat(menu_entry.menu_x, Eventually(Equals(0)))
        self.assertThat(menu_entry.menu_y, Eventually(Equals(0)))

    def test_menu_closes_on_new_focused_application(self):
        """Clicking outside an open menu must close it."""
        menu_entry = self.open_app_and_get_menu_entry()
        self.mouse_open_indicator(menu_entry)

        # This assert is for timing purposes only:
        self.assertThat(menu_entry.active, Eventually(Equals(True)))

        self.open_new_application_window("Text Editor")
        self.assertThat(self.unity.panels.get_active_indicator, Eventually(Equals(None)))

    def test_indicator_opens_when_dash_is_open(self):
        """When the dash is open and a click is on an indicator the dash
        must close and the indicator must open.
        """
        self.unity.dash.ensure_visible()

        indicator = self.panel.indicators.get_indicator_by_name_hint("indicator-session")
        self.mouse_open_indicator(indicator)

        self.assertThat(indicator.active, Eventually(Equals(True)))
        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))


class PanelKeyNavigationTests(PanelTestsBase):

    scenarios = _make_monitor_scenarios()

    def get_active_indicator(self):
        """Get the active indicator in a safe manner.

        This method will wait until the active indicator has been set.

        """
        self.assertThat(self.panel.get_active_indicator, Eventually(NotEquals(None)))
        return self.panel.get_active_indicator()

    def test_panel_first_menu_show_works(self):
        """Pressing the open-menus keybinding must open the first indicator."""
        self.open_new_application_window("Calculator")
        refresh_fn = lambda: len(self.panel.menus.get_entries())
        self.assertThat(refresh_fn, Eventually(GreaterThan(0)))
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.keybinding("panel/open_first_menu")

        open_indicator = self.get_active_indicator()
        expected_indicator = self.panel.get_indicator_entries(include_hidden_menus=True)[0]
        self.assertThat(open_indicator.entry_id, Eventually(Equals(expected_indicator.entry_id)))

    def test_panel_hold_show_menu_works(self):
        """Holding the show menu key must reveal the menu with mnemonics."""
        self.open_new_application_window("Text Editor")
        refresh_fn = lambda: len(self.panel.menus.get_entries())
        self.assertThat(refresh_fn, Eventually(GreaterThan(0)))
        self.addCleanup(self.keyboard.press_and_release, "Escape")

        # Wait for menu to fade out first
        self.assertThat(self.panel.menus.get_entries()[0].visible, Eventually(Equals(0)))

        self.keyboard.press("Alt")
        self.addCleanup(self.keyboard.release, "Alt")
        self.assertTrue(self.panel.menus.get_entries()[0].visible)
        self.assertThat(self.panel.menus.get_entries()[0].label, Equals("_File"))

    def test_panel_menu_accelerators_work(self):
        """Pressing a valid menu accelerator must open the correct menu item."""
        self.open_new_application_window("Text Editor")
        refresh_fn = lambda: len(self.panel.menus.get_entries())
        self.assertThat(refresh_fn, Eventually(GreaterThan(0)))
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.keyboard.press_and_release("Alt+f")

        open_indicator = self.get_active_indicator()
        self.assertThat(open_indicator.label, Eventually(Equals("_File")))

    def test_panel_indicators_key_navigation_next_works(self):
        """Right arrow key must open the next menu."""
        calc_win = self.open_new_application_window("Calculator")
        self.assertProperty(calc_win, is_focused=True)

        available_indicators = self.panel.get_indicator_entries(include_hidden_menus=True)

        self.keybinding("panel/open_first_menu")
        self.addCleanup(self.keyboard.press_and_release, "Escape")

        self.keybinding("panel/next_indicator")
        open_indicator = self.get_active_indicator()
        expected_indicator = available_indicators[1]
        self.assertThat(open_indicator.entry_id, Eventually(Equals(expected_indicator.entry_id)))

    def test_panel_indicators_key_navigation_prev_works(self):
        """Left arrow key must open the previous menu."""
        calc_win = self.open_new_application_window("Calculator")
        self.assertProperty(calc_win, is_focused=True)

        available_indicators = self.panel.get_indicator_entries(include_hidden_menus=True)

        self.keybinding("panel/open_first_menu")
        self.addCleanup(self.keyboard.press_and_release, "Escape")

        self.keybinding("panel/prev_indicator")
        open_indicator = self.get_active_indicator()
        expected_indicator = available_indicators[-1]

        self.assertThat(open_indicator.entry_id, Eventually(Equals(expected_indicator.entry_id)))

    def test_mouse_does_not_break_key_navigation(self):
        """Must be able to use the mouse to open indicators after they've been
        opened with the keyboard.
        """
        self.open_new_application_window("Calculator")
        available_indicators = self.panel.get_indicator_entries(include_hidden_menus=True)

        self.keybinding("panel/open_first_menu")
        self.addCleanup(self.keyboard.press_and_release, "Escape")

        available_indicators[2].mouse_move_to()
        self.addCleanup(self.panel.move_mouse_below_the_panel)

        self.assertThat(available_indicators[2].active, Eventually(Equals(True)))

        self.keybinding("panel/prev_indicator")
        self.assertThat(available_indicators[1].active, Eventually(Equals(True)))


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

        self.assertProperty(text_win, is_maximized=False)

    def test_focus_the_maximized_window_works(self):
        """Clicking on the grab area must put a maximized window in focus."""
        text_win = self.open_new_application_window("Text Editor", maximized=True)
        calc_win = self.open_new_application_window("Calculator")

        self.assertProperty(text_win, is_focused=False)
        self.assertProperty(calc_win, is_focused=True)

        self.move_mouse_over_grab_area()
        self.mouse.click()

        self.assertProperty(text_win, is_focused=True)

    def test_lower_the_maximized_window_works(self):
        """Middle-clicking on the panel grab area must lower a maximized window."""
        calc_win = self.open_new_application_window("Calculator")
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        self.assertProperty(text_win, is_focused=True)
        self.assertProperty(calc_win, is_focused=False)

        self.move_mouse_over_grab_area()
        self.mouse.click(2)

        self.assertProperty(calc_win, is_focused=True)

    def test_panels_dont_steal_keynav_foucs_from_hud(self):
        """On a mouse click event on the panel you must still be able to type into the Hud."""
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.keyboard.type("Hello")
        self.move_mouse_over_grab_area()
        self.mouse.click()
        self.keyboard.type("World")

        self.assertThat(self.unity.hud.search_string, Eventually(Equals("HelloWorld")))


class PanelCrossMonitorsTests(PanelTestsBase):
    """Multimonitor panel tests."""

    def setUp(self):
        super(PanelCrossMonitorsTests, self).setUp()
        if self.display.get_num_screens() < 2:
            self.skipTest("This test requires a multimonitor setup")

    def test_panel_title_updates_moving_window(self):
        """Panel must show the title of a restored window when moved to it's monitor."""
        calc_win = self.open_new_application_window("Calculator")

        prev_monitor = None
        for monitor in range(0, self.display.get_num_screens()):
            if calc_win.monitor != monitor:
                drag_window_to_screen(calc_win, monitor)

            if prev_monitor:
                prev_panel = self.unity.panels.get_panel_for_monitor(prev_monitor)
                self.assertThat(prev_panel.active, Eventually(Equals(False)))

            panel = self.unity.panels.get_panel_for_monitor(monitor)
            self.assertThat(panel.active, Eventually(Equals(True)))
            self.assertThat(panel.title, Eventually(Equals(calc_win.application.name)))

            prev_monitor = monitor

    def test_window_buttons_dont_show_for_maximized_window_on_mouse_in(self):
        """Window buttons must not show when the mouse is hovering the panel in
        other monitors.
        """
        self.open_new_application_window("Text Editor", maximized=True)
        self.sleep_menu_settle_period()

        for monitor in range(0, self.display.get_num_screens()):
            panel = self.unity.panels.get_panel_for_monitor(monitor)
            panel.move_mouse_over_window_buttons()

            self.sleep_menu_settle_period()

            if self.panel_monitor == monitor:
                self.assertThat(panel.window_buttons_shown, Eventually(Equals(True)))
            else:
                self.assertThat(panel.window_buttons_shown, Eventually(Equals(False)))

    def test_window_buttons_dont_show_in_other_monitors_when_dash_is_open(self):
        """Window buttons must not show on the panels other than the one where
        the dash is opened.
        """
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        for monitor in range(0, self.display.get_num_screens()):
            panel = self.unity.panels.get_panel_for_monitor(monitor)

            if self.unity.dash.monitor == monitor:
                self.assertThat(self.unity.dash.view.overlay_window_buttons_shown[monitor], Equals(True))
            else:
                self.assertThat(self.unity.dash.view.overlay_window_buttons_shown[monitor], Equals(False))

    def test_window_buttons_dont_show_in_other_monitors_when_hud_is_open(self):
        """Window buttons must not show on the panels other than the one where
        the hud is opened.
        """
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        for monitor in range(0, self.display.get_num_screens()):
            panel = self.unity.panels.get_panel_for_monitor(monitor)

            if self.unity.hud.monitor == monitor:
                self.assertThat(self.unity.hud.view.overlay_window_buttons_shown[monitor], Equals(True))
            else:
                self.assertThat(self.unity.hud.view.overlay_window_buttons_shown[monitor], Equals(False))

    def test_window_buttons_close_inactive_when_clicked_in_another_monitor(self):
        """Clicking the close button must not affect the active maximized window
        on another monitor.

        See bug #865701
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        for monitor in range(self.display.get_num_screens()):
            panel = self.unity.panels.get_panel_for_monitor(monitor)

            if monitor != text_win.monitor:
                panel.window_buttons.close.mouse_move_to()
                panel.window_buttons.close.mouse_click()
                self.assertThat(text_win.closed, Equals(False))

    def test_window_buttons_minimize_inactive_when_clicked_in_another_monitor(self):
        """Clicking the minimise button must not affect the active maximized
        window on another monitor.

        See bug #865701
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        for monitor in range(self.display.get_num_screens()):
            panel = self.unity.panels.get_panel_for_monitor(monitor)

            if monitor != text_win.monitor:
                panel.window_buttons.minimize.mouse_click()
                self.assertThat(text_win.is_hidden, Equals(False))

    def test_window_buttons_unmaximize_inactive_when_clicked_in_another_monitor(self):
        """Clicking the restore button must not affect the active maximized
        window on another monitor.

        See bug #865701
        """
        text_win = self.open_new_application_window("Text Editor", maximized=True)

        for monitor in range(0, self.display.get_num_screens()):
            panel = self.unity.panels.get_panel_for_monitor(monitor)

            if monitor != text_win.monitor:
                panel.window_buttons.unmaximize.mouse_click()
                self.assertThat(text_win.is_maximized, Equals(True))

    def test_hovering_indicators_on_multiple_monitors(self):
        """Opening an indicator entry and then hovering others entries must open them."""
        text_win = self.open_new_application_window("Text Editor")
        panel = self.unity.panels.get_panel_for_monitor(text_win.monitor)
        indicator = panel.indicators.get_indicator_by_name_hint("indicator-session")
        self.mouse_open_indicator(indicator)

        for monitor in range(0, self.display.get_num_screens()):
            panel = self.unity.panels.get_panel_for_monitor(monitor)

            entries = panel.get_indicator_entries(include_hidden_menus=True)
            self.assertThat(len(entries), GreaterThan(0))

            for entry in entries:
                entry.mouse_move_to()

                if monitor != self.panel_monitor and entry.type == "menu":
                    # we're on the "other" monitor, so the menu should be hidden.
                    self.assertThat(entry.active, Eventually(Equals(False)))
                    self.assertThat(entry.visible, Eventually(Equals(False)))
                    self.assertThat(entry.menu_y, Eventually(Equals(0)))
                else:
                    self.assertThat(entry.visible, Eventually(Equals(True)))
                    self.assertThat(entry.active, Eventually(Equals(True)))
                    self.assertThat(entry.menu_y, Eventually(NotEquals(0)))
