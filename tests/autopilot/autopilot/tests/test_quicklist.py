# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards,
#         Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import os.path
from testtools.matchers import Contains, Equals, NotEquals
from xdg.DesktopEntry import DesktopEntry
from time import sleep

from autopilot.emulators.unity.quicklist import QuicklistMenuItemLabel
from autopilot.matchers import Eventually
from autopilot.tests import AutopilotTestCase


class QuicklistActionTests(AutopilotTestCase):
    """Tests for quicklist actions."""

    scenarios = [
        ('remmina', {'app_name': 'Remmina'}),
    ]

    def open_quicklist_for_icon(self, launcher_icon):
        launcher = self.launcher.get_launcher_for_monitor(0)
        launcher.click_launcher_icon(launcher_icon, button=3)
        self.addCleanup(self.keyboard.press_and_release, "Escape")

    def test_quicklist_actions(self):
        """Test that all actions present in the destop file are shown in the quicklist."""
        self.start_app(self.app_name)

        # load the desktop file from disk:
        desktop_id = self.KNOWN_APPS[self.app_name]['desktop-file']
        desktop_file = os.path.join('/usr/share/applications', desktop_id)
        de = DesktopEntry(desktop_file)
        # get the launcher icon from the launcher:
        launcher_icon = self.launcher.model.get_icon_by_desktop_id(desktop_id)
        self.assertThat(launcher_icon, NotEquals(None))

        # open the icon quicklist, and get all the text labels:
        self.open_quicklist_for_icon(launcher_icon)
        ql = launcher_icon.get_quicklist()
        ql_item_texts = [i.text for i in ql.items if type(i) is QuicklistMenuItemLabel]

        # iterate over all the actions from the desktop file, make sure they're
        # present in the quicklist texts.
        # FIXME, this doesn't work using a locale other than English.
        actions = de.getActions()
        for action in actions:
            key = 'Desktop Action ' + action
            self.assertThat(de.content, Contains(key))
            name = de.content[key]['Name']
            self.assertThat(ql_item_texts, Contains(name))

    def test_quicklist_application_item_focus_last_active_window(self):
        """This tests shows that when you activate a quicklist application item
        only the last focused instance of that application is rasied.

        This is tested by opening 2 Mahjongg and a Calculator.
        Then we activate the Calculator quicklist item.
        Then we actiavte the Mahjongg launcher icon.
        """
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

        mahj_icon = self.launcher.model.get_icon_by_desktop_id(mahj.desktop_file)
        calc_icon = self.launcher.model.get_icon_by_desktop_id(calc.desktop_file)

        self.open_quicklist_for_icon(calc_icon)
        calc_ql = calc_icon.get_quicklist()
        calc_ql.get_quicklist_application_item(calc.name).mouse_click()
        sleep(1)
        self.assertTrue(calc_win.is_focused)
        self.assertVisibleWindowStack([calc_win, mah_win2, mah_win1])

        self.open_quicklist_for_icon(mahj_icon)
        mahj_ql = mahj_icon.get_quicklist()
        mahj_ql.get_quicklist_application_item(mahj.name).mouse_click()
        sleep(1)
        self.assertTrue(mah_win2.is_focused)
        self.assertVisibleWindowStack([mah_win2, calc_win, mah_win1])

    def test_quicklist_application_item_initiate_spread(self):
        """This tests shows that when you activate a quicklist application item
        when an application window is focused, the spread is initiated.
        """
        calc = self.start_app("Calculator")
        [calc_win1] = calc.get_windows()
        self.assertTrue(calc_win1.is_focused)

        self.start_app("Calculator")
        # Sleeping due to the start_app only waiting for the bamf model to be
        # updated with the application.  Since the app has already started,
        # and we are just waiting on a second window, however a defined sleep
        # here is likely to be problematic.
        # TODO: fix bamf emulator to enable waiting for new windows.
        sleep(1)
        [calc_win2] = [w for w in calc.get_windows() if w.x_id != calc_win1.x_id]

        self.assertVisibleWindowStack([calc_win2, calc_win1])
        self.assertTrue(calc_win2.is_focused)

        calc_icon = self.launcher.model.get_icon_by_desktop_id(calc.desktop_file)

        self.open_quicklist_for_icon(calc_icon)
        calc_ql = calc_icon.get_quicklist()
        app_item = calc_ql.get_quicklist_application_item(calc.name)

        self.addCleanup(self.keybinding, "spread/cancel")
        app_item.mouse_click()
        self.assertThat(self.window_manager.scale_active, Eventually(Equals(True)))
        self.assertThat(self.window_manager.scale_active_for_group, Eventually(Equals(True)))


class QuicklistKeyNavigationTests(AutopilotTestCase):
    """Tests for the quicklist key navigation."""

    def setUp(self):
        super(QuicklistKeyNavigationTests, self).setUp()

        self.ql_app = self.start_app("Text Editor")

        self.ql_launcher_icon = self.launcher.model.get_icon_by_desktop_id(self.ql_app.desktop_file)
        self.assertThat(self.ql_launcher_icon, NotEquals(None))

        self.ql_launcher = self.launcher.get_launcher_for_monitor(0)

    def open_quicklist_with_mouse(self):
        """Opens a quicklist with the mouse."""
        self.ql_launcher.click_launcher_icon(self.ql_launcher_icon, button=3)
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.quicklist = self.ql_launcher_icon.get_quicklist()
        self.assertThat(self.quicklist, NotEquals(None))
        self.quicklist.move_mouse_to_right()
        self.assertThat(self.quicklist.selected_item, Equals(None))

    def open_quicklist_with_keyboard(self):
        """Opens a quicklist using the keyboard."""
        self.screen_geo.move_mouse_to_monitor(0)
        self.ql_launcher.key_nav_start()
        self.addCleanup(self.ql_launcher.key_nav_cancel)

        for icon in self.launcher.model.get_launcher_icons():
            if icon.tooltip_text != self.ql_app.name:
                self.ql_launcher.key_nav_next()
            else:
                self.keybinding("launcher/keynav/open-quicklist")
                self.addCleanup(self.keybinding, "launcher/keynav/close-quicklist")
                break

        self.quicklist = self.ql_launcher_icon.get_quicklist()
        self.assertThat(self.quicklist, NotEquals(None))
        self.assertThat(self.quicklist.selected_item, NotEquals(None))

    def test_keynav_selects_first_item_when_unselected(self):
        """Home key MUST select the first selectable item in a quicklist."""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/first")

        expected_item = self.quicklist.selectable_items[0]
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

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
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_next_selects_first_item_when_unselected(self):
        """Down key MUST select the first valid item when nothing is selected."""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/next")

        expected_item = self.quicklist.selectable_items[0]
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_selects_last_item_when_unselected(self):
        """End key MUST select the last selectable item in a quicklist."""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/last")

        expected_item = self.quicklist.selectable_items[-1]
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

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
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_prev_selects_last_item_when_unselected(self):
        """Up key MUST select the last valid item when nothing is selected."""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/prev")

        expected_item = self.quicklist.selectable_items[-1]
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_launcher_keynav_selects_first_item(self):
        """The first selectable item of the quicklist must be selected when
        opening the quicklist using the launcher key navigation.
        """
        self.open_quicklist_with_keyboard()

        expected_item = self.quicklist.selectable_items[0]
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_next_selection_works(self):
        """Down key MUST select the next valid item."""
        self.open_quicklist_with_mouse()

        for item in self.quicklist.selectable_items:
            self.keybinding("quicklist/keynav/next")
            self.assertThat(item.selected, Eventually(Equals(True)))
            self.assertThat(self.quicklist.selected_item.id, Equals(item.id))

    def test_keynav_prev_selection_works(self):
        """Up key MUST select the previous valid item."""
        self.open_quicklist_with_mouse()

        for item in reversed(self.quicklist.selectable_items):
            self.keybinding("quicklist/keynav/prev")
            self.assertThat(item.selected, Eventually(Equals(True)))
            self.assertThat(self.quicklist.selected_item.id, Equals(item.id))

    def test_keynav_prev_is_cyclic(self):
        """Up key MUST select the last item, when the first one is selected."""
        self.open_quicklist_with_mouse()

        mouse_item = self.quicklist.selectable_items[0]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        self.keybinding("quicklist/keynav/prev")
        expected_item = self.quicklist.selectable_items[-1]
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_next_is_cyclic(self):
        """Down key MUST select the first item, when the last one is selected."""
        self.open_quicklist_with_mouse()

        mouse_item = self.quicklist.selectable_items[-1]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))

        self.keybinding("quicklist/keynav/next")
        expected_item = self.quicklist.selectable_items[0]
        self.assertThat(expected_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

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
        self.assertThat(key_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(key_item.id))

        # Moving the mouse horizontally doesn't change the selection
        self.mouse.move(mouse_item.x + mouse_item.width - 10, mouse_item.y + mouse_item.height / 2)
        self.assertThat(self.quicklist.selected_item.id, Equals(key_item.id))

        # Moving the mouse outside doesn't change the selection
        self.mouse.move(mouse_item.x + mouse_item.width + 50, mouse_item.y + mouse_item.height / 2)
        self.assertThat(self.quicklist.selected_item.id, Equals(key_item.id))

        # Moving the mouse to another entry, changes the selection
        mouse_item = self.quicklist.selectable_items[-2]
        mouse_item.mouse_move_to()
        self.assertThat(mouse_item.selected, Eventually(Equals(True)))
        self.assertThat(self.quicklist.selected_item.id, Equals(mouse_item.id))
