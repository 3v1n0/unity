# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards,
#         Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
import os.path
from testtools.matchers import Contains, Equals, NotEquals
from time import sleep
from xdg.DesktopEntry import DesktopEntry

from unity.emulators.quicklist import QuicklistMenuItemLabel
from unity.tests import UnityTestCase


class QuicklistActionTests(UnityTestCase):
    """Tests for quicklist actions."""

    scenarios = [
        ('remmina', {'app_name': 'Remmina'}),
    ]

    def open_quicklist_for_icon(self, launcher_icon):
        """Open the quicklist for the given launcher icon.

        Returns the quicklist that was opened.

        """
        launcher = self.unity.launcher.get_launcher_for_monitor(0)
        launcher.click_launcher_icon(launcher_icon, button=3)
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.assertThat(launcher_icon.get_quicklist, Eventually(NotEquals(None)))
        return launcher_icon.get_quicklist()

    def get_desktop_entry(self, application):
        # load the desktop file from disk:
        desktop_id = self.KNOWN_APPS[application]['desktop-file']
        desktop_file = os.path.join('/usr/share/applications', desktop_id)
        return DesktopEntry(desktop_file)

    def test_quicklist_actions(self):
        """Test that all actions present in the destop file are shown in the quicklist."""
        app = self.start_app(self.app_name)

        # get the launcher icon from the launcher:
        launcher_icon = self.unity.launcher.model.get_icon(desktop_id=app.desktop_file)
        self.assertThat(launcher_icon, NotEquals(None))

        # open the icon quicklist, and get all the text labels:
        de = self.get_desktop_entry(self.app_name)
        ql = self.open_quicklist_for_icon(launcher_icon)
        ql_item_texts = [i.text for i in ql.items if type(i) is QuicklistMenuItemLabel]

        # iterate over all the actions from the desktop file, make sure they're
        # present in the quicklist texts.
        for action in de.getActions():
            key = 'Desktop Action ' + action
            self.assertThat(de.content, Contains(key))
            name = de.get('Name', group=key, locale=True)
            self.assertThat(ql_item_texts, Contains(name))

    def test_quicklist_action_uses_startup_notification(self):
        """Tests that quicklist uses startup notification protocol."""
        self.register_nautilus()
        self.addCleanup(self.close_all_windows, "Nautilus")

        self.start_app_window("Calculator")
        self.start_app(self.app_name)

        nautilus_icon = self.unity.launcher.model.get_icon(desktop_id="nautilus.desktop")
        ql = self.open_quicklist_for_icon(nautilus_icon)
        de = self.get_desktop_entry("Nautilus")

        new_window_action_name = de.get("Name", group="Desktop Action Window", locale=True)
        self.assertThat(new_window_action_name, NotEquals(None))
        new_win_ql_item_fn = lambda : ql.get_quicklist_item_by_text(new_window_action_name)
        self.assertThat(new_win_ql_item_fn, Eventually(NotEquals(None)))
        new_win_ql_item = new_win_ql_item_fn()

        ql.click_item(new_win_ql_item)

        nautilus_windows_fn = lambda: self.get_open_windows_by_application("Nautilus")
        self.assertThat(lambda: len(nautilus_windows_fn()), Eventually(Equals(1)))
        [nautilus_window] = nautilus_windows_fn()

        self.assertThat(lambda: self.get_startup_notification_timestamp(nautilus_window),
                        Eventually(Equals(new_win_ql_item.activate_timestamp)))

    def test_quicklist_application_item_focus_last_active_window(self):
        """This tests shows that when you activate a quicklist application item
        only the last focused instance of that application is rasied.

        This is tested by opening 2 Mahjongg and a Calculator.
        Then we activate the Calculator quicklist item.
        Then we actiavte the Mahjongg launcher icon.
        """
        char_win1 = self.start_app_window("Character Map")
        calc_win = self.start_app_window("Calculator")
        char_win2 = self.start_app_window("Character Map")

        self.assertVisibleWindowStack([char_win2, calc_win, char_win1])

        char_icon = self.unity.launcher.model.get_icon(
            desktop_id=char_win1.application.desktop_file)
        calc_icon = self.unity.launcher.model.get_icon(
            desktop_id=calc_win.application.desktop_file)

        calc_ql = self.open_quicklist_for_icon(calc_icon)
        calc_ql.get_quicklist_application_item(calc_win.application.name).mouse_click()

        self.assertProperty(calc_win, is_focused=True)
        self.assertVisibleWindowStack([calc_win, char_win2, char_win1])

        char_ql = self.open_quicklist_for_icon(char_icon)
        char_ql.get_quicklist_application_item(char_win1.application.name).mouse_click()

        self.assertProperty(char_win2, is_focused=True)
        self.assertVisibleWindowStack([char_win2, calc_win, char_win1])

    def test_quicklist_application_item_initiate_spread(self):
        """This tests shows that when you activate a quicklist application item
        when an application window is focused, the spread is initiated.
        """
        char_win1 = self.start_app_window("Character Map")
        char_win2 = self.start_app_window("Character Map")
        char_app = char_win1.application

        self.assertVisibleWindowStack([char_win2, char_win1])
        self.assertProperty(char_win2, is_focused=True)

        char_icon = self.unity.launcher.model.get_icon(desktop_id=char_app.desktop_file)

        char_ql = self.open_quicklist_for_icon(char_icon)
        app_item = char_ql.get_quicklist_application_item(char_app.name)

        self.addCleanup(self.keybinding, "spread/cancel")
        app_item.mouse_click()
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))
        self.assertThat(self.unity.window_manager.scale_active_for_group, Eventually(Equals(True)))

    def test_quicklist_item_triggered_closes_dash(self):
        """When any quicklist item is triggered it must close the dash."""

        calc_win = self.start_app_window("Calculator")
        self.assertProperty(calc_win, is_focused=True)

        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        calc_icon = self.unity.launcher.model.get_icon(
            desktop_id=calc_win.application.desktop_file)
        self.open_quicklist_for_icon(calc_icon)

        self.keyboard.press_and_release("Down")
        self.keyboard.press_and_release("Enter")
        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))

    def test_quicklist_closes_when_hud_opens(self):
        """When a quicklist is open you must still be able to open the Hud."""
        calc = self.start_app("Calculator")

        calc_icon = self.unity.launcher.model.get_icon(desktop_id=calc.desktop_file)
        self.open_quicklist_for_icon(calc_icon)

        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)
        self.assertThat(self.unity.hud.visible, Eventually(Equals(True)))

    def test_quicklist_closes_when_dash_opens(self):
        """When the quicklist is open you must still be able to open the dash."""
        calc = self.start_app("Calculator")

        calc_icon = self.unity.launcher.model.get_icon(desktop_id=calc.desktop_file)
        self.open_quicklist_for_icon(calc_icon)

        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)
        self.assertThat(self.unity.dash.visible, Eventually(Equals(True)))

    def test_right_click_opens_quicklist_if_already_open(self):
        """A right click to another icon in the launcher must
        close the current open quicklist and open the other
        icons quicklist.
        lp:890991
        """

        calc_win = self.start_app_window("Calculator")
        mahj_win = self.start_app_window("Mahjongg")

        calc_icon = self.unity.launcher.model.get_icon(
            desktop_id=calc_win.application.desktop_file)
        mahj_icon = self.unity.launcher.model.get_icon(
            desktop_id=mahj_win.application.desktop_file)

        calc_ql = self.open_quicklist_for_icon(calc_icon)
        self.assertThat(calc_ql.active, Eventually(Equals(True)))

        mahj_ql = self.open_quicklist_for_icon(mahj_icon)
        self.assertThat(mahj_ql.active, Eventually(Equals(True)))
        self.assertThat(calc_ql.active, Eventually(Equals(False)))

    def test_right_clicking_same_icon_doesnt_reopen_ql(self):
        """A right click to the same icon in the launcher must
        not re-open the quicklist if already open. It must hide.
        """

        calc_win = self.start_app_window("Calculator")

        calc_icon = self.unity.launcher.model.get_icon(
            desktop_id=calc_win.application.desktop_file)

        calc_ql = self.open_quicklist_for_icon(calc_icon)
        self.assertThat(calc_ql.active, Eventually(Equals(True)))

        calc_ql = self.open_quicklist_for_icon(calc_icon)
        self.assertThat(calc_ql.active, Eventually(Equals(False)))


class QuicklistKeyNavigationTests(UnityTestCase):
    """Tests for the quicklist key navigation."""

    def setUp(self):
        super(QuicklistKeyNavigationTests, self).setUp()

        desktop_file = self.KNOWN_APPS["Text Editor"]["desktop-file"]
        icon_refresh_fn = lambda : self.unity.launcher.model.get_icon(
            desktop_id=desktop_file)

        self.assertThat(icon_refresh_fn, Eventually(Equals(None)))
        self.ql_app = self.start_app("Text Editor")

        self.assertThat(icon_refresh_fn, Eventually(NotEquals(None)))
        self.ql_launcher_icon = icon_refresh_fn()

        self.ql_launcher = self.unity.launcher.get_launcher_for_monitor(0)

    def open_quicklist_with_mouse(self):
        """Opens a quicklist with the mouse."""
        self.ql_launcher.click_launcher_icon(self.ql_launcher_icon, button=3)
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.assertThat(self.ql_launcher_icon.get_quicklist,
                        Eventually(NotEquals(None)))
        self.quicklist = self.ql_launcher_icon.get_quicklist()
        self.quicklist.move_mouse_to_right()
        self.assertThat(lambda: self.quicklist.selected_item,
                        Eventually(Equals(None)))

    def open_quicklist_with_keyboard(self):
        """Opens a quicklist using the keyboard."""
        self.screen_geo.move_mouse_to_monitor(0)
        self.ql_launcher.key_nav_start()
        self.addCleanup(self.ql_launcher.key_nav_cancel)

        self.ql_launcher.keyboard_select_icon(tooltip_text=self.ql_app.name)
        self.keybinding("launcher/keynav/open-quicklist")
        self.addCleanup(self.keybinding, "launcher/keynav/close-quicklist")

        self.assertThat(self.ql_launcher_icon.get_quicklist,
                        Eventually(NotEquals(None)))
        self.quicklist = self.ql_launcher_icon.get_quicklist()
        self.assertThat(lambda: self.quicklist.selected_item,
                        Eventually(NotEquals(None)))

    def assertCorrectItemSelected(self, item):
        """Ensure the item considers itself selected and that quicklist agrees."""
        self.assertThat(item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(item.id))

    def test_keynav_selects_first_item_when_unselected(self):
        """Home key MUST select the first selectable item in a quicklist."""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/first")

        expected_item = self.quicklist.selectable_items[0]
        self.assertCorrectItemSelected(expected_item)

    def test_keynav_selects_first_item_when_selected(self):
        """Home key MUST select the first selectable item in a quicklist when
        another item is selected.
        """
        self.open_quicklist_with_mouse()
        mouse_item = self.quicklist.selectable_items[-1]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        self.keybinding("quicklist/keynav/first")

        expected_item = self.quicklist.selectable_items[0]
        self.assertCorrectItemSelected(expected_item)

    def test_keynav_next_selects_first_item_when_unselected(self):
        """Down key MUST select the first valid item when nothing is selected."""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/next")

        expected_item = self.quicklist.selectable_items[0]
        self.assertCorrectItemSelected(expected_item)

    def test_keynav_selects_last_item_when_unselected(self):
        """End key MUST select the last selectable item in a quicklist."""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/last")

        expected_item = self.quicklist.selectable_items[-1]
        self.assertCorrectItemSelected(expected_item)

    def test_keynav_selects_last_item_when_selected(self):
        """End key MUST select the last selectable item in a quicklist when
        another item is selected.
        """
        self.open_quicklist_with_mouse()
        mouse_item = self.quicklist.selectable_items[0]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        self.keybinding("quicklist/keynav/last")

        expected_item = self.quicklist.selectable_items[-1]
        self.assertCorrectItemSelected(expected_item)

    def test_keynav_prev_selects_last_item_when_unselected(self):
        """Up key MUST select the last valid item when nothing is selected."""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/prev")

        expected_item = self.quicklist.selectable_items[-1]
        self.assertCorrectItemSelected(expected_item)

    def test_launcher_keynav_selects_first_item(self):
        """The first selectable item of the quicklist must be selected when
        opening the quicklist using the launcher key navigation.
        """
        self.open_quicklist_with_keyboard()

        expected_item = self.quicklist.selectable_items[0]
        self.assertCorrectItemSelected(expected_item)

    def test_keynav_next_selection_works(self):
        """Down key MUST select the next valid item."""
        self.open_quicklist_with_mouse()

        for item in self.quicklist.selectable_items:
            self.keybinding("quicklist/keynav/next")
            self.assertCorrectItemSelected(item)

    def test_keynav_prev_selection_works(self):
        """Up key MUST select the previous valid item."""
        self.open_quicklist_with_mouse()

        for item in reversed(self.quicklist.selectable_items):
            self.keybinding("quicklist/keynav/prev")
            self.assertCorrectItemSelected(item)

    def test_keynav_prev_is_cyclic(self):
        """Up key MUST select the last item, when the first one is selected."""
        self.open_quicklist_with_mouse()

        mouse_item = self.quicklist.selectable_items[0]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        self.keybinding("quicklist/keynav/prev")
        expected_item = self.quicklist.selectable_items[-1]
        self.assertCorrectItemSelected(expected_item)

    def test_keynav_next_is_cyclic(self):
        """Down key MUST select the first item, when the last one is selected."""
        self.open_quicklist_with_mouse()

        mouse_item = self.quicklist.selectable_items[-1]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        self.keybinding("quicklist/keynav/next")
        expected_item = self.quicklist.selectable_items[0]
        self.assertCorrectItemSelected(expected_item)

    def test_keynav_mouse_interaction(self):
        """Tests that the interaction between key-navigation and mouse works as
        expected. See bug #911561.
        """
        self.open_quicklist_with_mouse()
        mouse_item = self.quicklist.selectable_items[-1]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        self.keybinding("quicklist/keynav/prev")
        sleep(.1)
        self.keybinding("quicklist/keynav/prev")

        key_item = self.quicklist.selectable_items[-3]
        self.assertCorrectItemSelected(key_item)

        # Moving the mouse horizontally doesn't change the selection
        self.mouse.move(mouse_item.x + mouse_item.width - 10, mouse_item.y + mouse_item.height / 2)
        self.assertThat(self.quicklist.selected_item.id, Equals(key_item.id))

        # Moving the mouse outside doesn't change the selection
        self.mouse.move(mouse_item.x + mouse_item.width + 50, mouse_item.y + mouse_item.height / 2)
        self.assertThat(self.quicklist.selected_item.id, Equals(key_item.id))

        # Moving the mouse to another entry, changes the selection
        mouse_item = self.quicklist.selectable_items[-2]
        mouse_item.mouse_move_to()
        self.assertCorrectItemSelected(mouse_item)

    def test_moving_mouse_during_grab_select_correct_menuitem(self):
        """Test that moving the mouse during grabbing selects the
        correct menu item. See bug #1027955.
        """
        self.open_quicklist_with_mouse()
        mouse_item = self.quicklist.selectable_items[0]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        # Dragging the mouse horizontally doesn't change the selection
        self.mouse.press()
        self.addCleanup(self.mouse.release)
        self.mouse.move(mouse_item.x + mouse_item.width - 10, mouse_item.y + mouse_item.height / 2)
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        # Moving the mouse down selects the next item
        mouse_item = self.quicklist.selectable_items[1]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))
