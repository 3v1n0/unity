# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards,
#         Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import os.path
from testtools.matchers import Not, Is, Contains, Equals
from xdg.DesktopEntry import DesktopEntry
from time import sleep

from autopilot.emulators.unity.quicklist import QuicklistMenuItemLabel
from autopilot.tests import AutopilotTestCase


class QuicklistActionTests(AutopilotTestCase):
    """Tests for quicklist actions."""

    scenarios = [
        ('remmina', {'app_name': 'Remmina'}),
    ]

    def test_quicklist_actions(self):
        """Test that all actions present in the destop file are shown in the quicklist."""
        self.start_app(self.app_name)

        # load the desktop file from disk:
        desktop_file = os.path.join('/usr/share/applications',
            self.KNOWN_APPS[self.app_name]['desktop-file']
            )
        de = DesktopEntry(desktop_file)
        # get the launcher icon from the launcher:
        launcher_icon = self.launcher.model.get_icon_by_tooltip_text(de.getName())
        self.assertThat(launcher_icon, Not(Is(None)))

        # open the icon quicklist, and get all the text labels:
        launcher = self.launcher.get_launcher_for_monitor(0)
        launcher.click_launcher_icon(launcher_icon, button=3)
        ql = launcher_icon.get_quicklist()
        ql_item_texts = [i.text for i in ql.items if type(i) is QuicklistMenuItemLabel]

        # iterate over all the actions from the desktop file, make sure they're
        # present in the quicklist texts.
        actions = de.getActions()
        for action in actions:
            key = 'Desktop Action ' + action
            self.assertThat(de.content, Contains(key))
            name = de.content[key]['Name']
            self.assertThat(ql_item_texts, Contains(name))


class QuicklistKeyNavigationTests(AutopilotTestCase):
    """Tests for the quicklist key navigation"""

    def setUp(self):
        super(QuicklistKeyNavigationTests, self).setUp()

        self.ql_app = self.start_app("Text Editor")

        self.ql_launcher_icon = self.launcher.model.get_icon_by_desktop_id(self.ql_app.desktop_file)
        self.assertThat(self.ql_launcher_icon, Not(Is(None)))

        self.ql_launcher = self.launcher.get_launcher_for_monitor(0)

    def open_quicklist_with_mouse(self):
        """Opens a quicklist with the mouse"""
        self.ql_launcher.click_launcher_icon(self.ql_launcher_icon, button=3)
        self.addCleanup(self.keyboard.press_and_release, "Escape")
        self.quicklist = self.ql_launcher_icon.get_quicklist()
        self.assertThat(self.quicklist, Not(Is(None)))
        self.quicklist.move_mouse_to_right()
        self.assertThat(self.quicklist.selected_item, Is(None))

    def open_quicklist_with_keyboard(self):
        """Opens a quicklist using the keyboard"""
        self.screen_geo.move_mouse_to_monitor(0)
        self.ql_launcher.key_nav_start()
        self.addCleanup(self.ql_launcher.key_nav_cancel)

        for icon in self.launcher.model.get_launcher_icons():
            if (icon.tooltip_text != self.ql_app.name):
                self.ql_launcher.key_nav_next()
            else:
                self.keybinding("launcher/keynav/open-quicklist")
                self.addCleanup(self.keybinding, "launcher/keynav/close-quicklist")
                break

        self.quicklist = self.ql_launcher_icon.get_quicklist()
        self.assertThat(self.quicklist, Not(Is(None)))
        self.assertThat(self.quicklist.selected_item, Not(Is(None)))

    def get_selectable_items(self):
        """Returns a list of items that are selectable"""
        items = self.quicklist.get_items()
        return filter(lambda it: it.is_selectable == True, items)

    def test_keynav_selects_first_item_when_unselected(self):
        """Tests that the quicklist Home key selects the first valid item when
        no item is selected
        """
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/first")

        expected_item = self.get_selectable_items()[0]
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_selects_first_item_when_selected(self):
        """Tests that the quicklist Home key selects the first valid item when
        an item is selected
        """
        self.open_quicklist_with_mouse()
        mouse_item = self.get_selectable_items()[-1]
        mouse_item.mouse_move_to()
        self.assertTrue(mouse_item.selected)

        self.keybinding("quicklist/keynav/first")

        expected_item = self.get_selectable_items()[0]
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_next_selects_first_item_when_unselected(self):
        """Tests that the quicklist Down key selects the first valid item"""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/next")

        expected_item = self.get_selectable_items()[0]
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_selects_last_item_when_unselected(self):
        """Tests that the quicklist End key selects the last valid item when
        no item is selected
        """
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/last")

        expected_item = self.get_selectable_items()[-1]
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_selects_last_item_when_selected(self):
        """Tests that the quicklist End key selects the last valid item when an
        item is selected
        """
        self.open_quicklist_with_mouse()
        mouse_item = self.get_selectable_items()[0]
        mouse_item.mouse_move_to()
        self.assertTrue(mouse_item.selected)

        self.keybinding("quicklist/keynav/last")

        expected_item = self.get_selectable_items()[-1]
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_prev_selects_last_item_when_unselected(self):
        """Tests that the quicklist Up key selects the last valid item"""
        self.open_quicklist_with_mouse()

        self.keybinding("quicklist/keynav/prev")

        expected_item = self.get_selectable_items()[-1]
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_launcher_keynav_selects_first_item(self):
        """Tests that a quicklist opened using the launcher keybinding has
        the first item selected
        """
        self.open_quicklist_with_keyboard()

        expected_item = self.get_selectable_items()[0]
        self.assertThat(self.quicklist.selected_item.id, Equals(expected_item.id))

    def test_keynav_mouse_interaction(self):
        """Tests that the interaction between key-navigation and mouse works as
        expected. See bug #911561
        """
        self.open_quicklist_with_mouse()
        mouse_item = self.get_selectable_items()[-1]
        mouse_item.mouse_move_to()
        self.assertTrue(mouse_item.selected)

        self.keybinding("quicklist/keynav/prev")
        sleep(.1)
        self.keybinding("quicklist/keynav/prev")

        key_item = self.get_selectable_items()[-3]
        self.assertTrue(key_item.selected)
        self.assertThat(self.quicklist.selected_item.id, Equals(key_item.id))
        sleep(.5)

        # Moving the mouse horizontally doesn't change the selection
        self.mouse.move(mouse_item.x + mouse_item.width - 10, mouse_item.y + mouse_item.height / 2)
        self.assertThat(self.quicklist.selected_item.id, Equals(key_item.id))

        # Moving the mouse outside doesn't change the selection
        self.mouse.move(mouse_item.x + mouse_item.width + 50, mouse_item.y + mouse_item.height / 2)
        self.assertThat(self.quicklist.selected_item.id, Equals(key_item.id))

        # Moving the mouse to another entry, changes the selection
        mouse_item = self.get_selectable_items()[-2]
        mouse_item.mouse_move_to()
        self.assertTrue(mouse_item.selected)
        self.assertThat(self.quicklist.selected_item.id, Equals(mouse_item.id))
