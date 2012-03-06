# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from gtk import Clipboard
from time import sleep

from autopilot.emulators.unity.dash import Dash
from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.tests import AutopilotTestCase


class DashRevealTests(AutopilotTestCase):
    """Test the unity Dash Reveal."""

    def setUp(self):
        super(DashRevealTests, self).setUp()
        self.dash = Dash()

    def tearDown(self):
        super(DashRevealTests, self).tearDown()
        self.dash.ensure_hidden()

    def test_dash_reveal(self):
        """Ensure we can show and hide the dash."""
        self.dash.ensure_hidden()

        self.assertFalse(self.dash.get_is_visible())
        self.dash.toggle_reveal()
        self.assertTrue(self.dash.get_is_visible())
        self.dash.toggle_reveal()
        self.assertFalse(self.dash.get_is_visible())

    def test_dash_keyboard_focus(self):
        """Dash must put keyboard focus on the search bar at all times."""
        self.dash.ensure_visible()
        kb = Keyboard()
        kb.type("Hello")
        sleep(1)
        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, u'Hello')

    def test_application_lens_shortcut(self):
        """Application lense must reveal when Super+a is pressed."""
        self.dash.ensure_hidden()
        self.dash.reveal_application_lens()
        lensbar = self.dash.view.get_lensbar()
        self.assertEqual(lensbar.active_lens, u'applications.lens')

    def test_music_lens_shortcut(self):
        """Music lense must reveal when Super+w is pressed."""
        self.dash.ensure_hidden()
        self.dash.reveal_music_lens()
        lensbar = self.dash.view.get_lensbar()
        self.assertEqual(lensbar.active_lens, u'music.lens')

    def test_file_lens_shortcut(self):
        """File lense must reveal when Super+f is pressed."""
        self.dash.ensure_hidden()
        self.dash.reveal_file_lens()
        lensbar = self.dash.view.get_lensbar()
        self.assertEqual(lensbar.active_lens, u'files.lens')

    def test_command_lens_shortcut(self):
        """Run Command lens must reveat on alt+F2."""
        self.dash.ensure_hidden()
        self.dash.reveal_command_lens()
        lensbar = self.dash.view.get_lensbar()
        self.assertEqual(lensbar.active_lens, u'commands.lens')


class DashKeyNavTests(AutopilotTestCase):
    """Test the unity Dash keyboard navigation."""

    def setUp(self):
        super(DashKeyNavTests, self).setUp()
        self.dash = Dash()

    def tearDown(self):
        super(DashKeyNavTests, self).tearDown()
        self.dash.ensure_hidden()

    def test_lensbar_gets_keyfocus(self):
        """Test that the lensbar gets key focus after using Down keypresses."""
        self.dash.ensure_visible()
        kb = Keyboard()
        # Make sure that the lens bar can get the focus
        for i in range(self.dash.get_num_rows()):
            kb.press_and_release("Down")
        lensbar = self.dash.view.get_lensbar()
        self.assertIsNot(lensbar.focused_lens_icon, '')

    def test_lensbar_focus_changes(self):
        """Lensbar focused icon should change with Left and Right keypresses."""
        self.dash.ensure_visible()
        kb = Keyboard()
        #put KB focus on lensbar again:
        for i in range(self.dash.get_num_rows()):
            kb.press_and_release("Down")

        lensbar = self.dash.view.get_lensbar()
        current_focused_icon = lensbar.focused_lens_icon
        kb.press_and_release("Right");
        lensbar.refresh_state()
        self.assertNotEqual(lensbar.focused_lens_icon, current_focused_icon)
        kb.press_and_release("Left")
        lensbar.refresh_state()
        self.assertEqual(lensbar.focused_lens_icon, current_focused_icon)

    def test_lensbar_enter_activation(self):
        """Must be able to activate LensBar icons that have focus with an Enter keypress."""
        self.dash.ensure_visible()
        kb = Keyboard()
        #put KB focus on lensbar again:
        for i in range(self.dash.get_num_rows()):
            kb.press_and_release("Down")
        kb.press_and_release("Right");
        lensbar = self.dash.view.get_lensbar()
        focused_icon = lensbar.focused_lens_icon
        kb.press_and_release("Enter");
        lensbar.refresh_state()
        self.assertEqual(lensbar.active_lens, focused_icon)
        self.assertEqual(lensbar.focused_lens_icon, "")

    def test_category_header_keynav_autoscroll(self):
        """Test that the dash autoscroll when a category header gets
        the focus.
        """
        self.dash.ensure_hidden()
        self.dash.reveal_application_lens()
        app_lens = self.dash.get_current_lens()

        kb = Keyboard()
        mouse = Mouse()

        # Expand the first category
        kb.press_and_release("Down")
        kb.press_and_release("Enter")
        category = app_lens.get_focused_category()

        # Get the geometry of that category header.
        x = category.header_x
        y = category.header_y
        w = category.header_width

        # Manually scroll the dash.
        mouse.move(x+w+10, y+w+10, True)
        mouse.click(5)
        mouse.click(5)
        mouse.click(5)

        cached_y = y

        # Focus the search bar with the mouse
        searchbar = self.dash.get_searchbar()
        mouse.move(searchbar.x + 100,
                searchbar.y + searchbar.height / 2,
                True)
        mouse.click()
        sleep(2)

        # Then focus again the first category header
        kb.press_and_release("Down")
        kb.press_and_release("Enter")
        category = app_lens.get_focused_category()
        y = category.header_y

        # Make sure the dash autoscroll
        self.assertTrue(abs(y - cached_y) < 30)

    def test_category_header_keynav(self):
        """ This test makes sure that:
        1. A category header can get the focus.
        2. A category header stays highlight when it loses the focus
         and mouse is close to it (but not inside).
        """
        self.dash.ensure_hidden()
        self.dash.reveal_application_lens()

        kb = Keyboard()
        mouse = Mouse()

        # Make sure that a category have the focus.
        kb.press_and_release("Down")
        app_lens = self.dash.get_current_lens()
        category = app_lens.get_focused_category()
        self.assertIsNot(category, None)

        # Make sure that the category is highlighted.
        self.assertTrue(category.header_is_highlighted)

        # Get the geometry of that category header.
        x = category.header_x
        y = category.header_y
        w = category.header_width
        h = category.header_height

        # Move the mouse close the view, and press down.
        mouse.move(x + w + 10,
                    y + h / 2,
                    True)
        sleep(1)
        kb.press_and_release("Down")
        sleep(1)
        category = app_lens.get_focused_category()
        self.assertEqual(category, None)

    def test_cltr_tab(self):
        """ This test makes sure that Ctlr + Tab works well."""
        self.dash.ensure_hidden()
        self.dash.toggle_reveal()

        kb = Keyboard()
        lensbar = self.dash.view.get_lensbar()

        kb.press('Control')
        kb.press_and_release('Tab')
        kb.release('Control')
        lensbar.refresh_state()
        self.assertEqual(lensbar.active_lens, u'applications.lens')

        kb.press('Control')
        kb.press('Shift')
        kb.press_and_release('Tab')
        kb.release('Control')
        kb.release('Shift')

        lensbar.refresh_state()
        self.assertEqual(lensbar.active_lens, u'home.lens')

    def test_tab(self):
        """ This test makes sure that Tab works well."""
        self.dash.ensure_hidden()
        self.dash.reveal_application_lens()
        app_lens = self.dash.get_current_lens()

        kb = Keyboard()

        for i in range(app_lens.get_num_visible_categories()):
            kb.press_and_release('Tab')
            category = app_lens.get_focused_category()
            self.assertIsNot(category, None)

        kb.press_and_release('Tab')
        searchbar = self.dash.get_searchbar()
        self.assertTrue(searchbar.expander_has_focus)

        if not searchbar.showing_filters:
            kb.press_and_release('Enter')
            searchbar.refresh_state()
            self.assertTrue(searchbar.showing_filters)

        filter_bar = app_lens.get_filterbar()
        for i in range(filter_bar.get_num_filters()):
            kb.press_and_release('Tab')
            new_focused_filter = filter_bar.get_focused_filter()
            self.assertIsNotNone(new_focused_filter)

        kb.press_and_release('Tab')
        category = app_lens.get_focused_category()
        self.assertIsNot(category, None)


class DashClipboardTests(AutopilotTestCase):
    """Test the Unity clipboard"""

    def setUp(self):
        super(DashClipboardTests, self).setUp()
        self.dash = Dash()

    def tearDown(self):
        super(DashClipboardTests, self).tearDown()
        self.dash.ensure_hidden()

    def test_ctrl_a(self):
        """ This test if ctrl+a selects all text """
        self.dash.ensure_hidden()
        self.dash.toggle_reveal()

        kb = Keyboard();
        kb.type("SelectAll")
        sleep(1)

        kb.press_and_release("Ctrl+a")
        kb.press_and_release("Delete")

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, u'')

    def test_ctrl_c(self):
        """ This test if ctrl+c copies text into the clipboard """
        self.dash.ensure_hidden()
        self.dash.toggle_reveal()

        kb = Keyboard();
        kb.type("Copy")
        sleep(1)

        kb.press_and_release("Ctrl+a")
        kb.press_and_release("Ctrl+c")

        cb = Clipboard(selection="CLIPBOARD")

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, cb.wait_for_text())

    def test_ctrl_x(self):
        """ This test if ctrl+x deletes all text and copys it """
        self.dash.ensure_hidden()
        self.dash.toggle_reveal()

        kb = Keyboard();
        kb.type("Cut")
        sleep(1)

        kb.press_and_release("Ctrl+a")
        kb.press_and_release("Ctrl+x")
        sleep(1)

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, u'')

        cb = Clipboard(selection="CLIPBOARD")
        self.assertEqual(cb.wait_for_text(), u'Cut')

    def test_ctrl_c_v(self):
        """ This test if ctrl+c and ctrl+v copies and pastes text"""
        self.dash.ensure_hidden()
        self.dash.toggle_reveal()

        kb = Keyboard();
        kb.type("CopyPaste")
        sleep(1)

        kb.press_and_release("Ctrl+a")
        kb.press_and_release("Ctrl+c")
        kb.press_and_release("Ctrl+v")
        kb.press_and_release("Ctrl+v")

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, u'CopyPasteCopyPaste')

    def test_ctrl_x_v(self):
        """ This test if ctrl+x and ctrl+v cuts and pastes text"""
        self.dash.ensure_hidden()
        self.dash.toggle_reveal()

        kb = Keyboard();
        kb.type("CutPaste")
        sleep(1)

        kb.press_and_release("Ctrl+a")
        kb.press_and_release("Ctrl+x")
        kb.press_and_release("Ctrl+v")
        kb.press_and_release("Ctrl+v")

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, u'CutPasteCutPaste')


class DashKeyboardFocusTests(AutopilotTestCase):
    """Tests that keyboard focus works."""

    def setUp(self):
        super(DashKeyboardFocusTests, self).setUp()
        self.dash = Dash()

    def tearDown(self):
        super(DashKeyboardFocusTests, self).tearDown()
        self.dash.ensure_hidden()

    def test_filterbar_expansion_leaves_kb_focus(self):
        """Expanding or collapsing the filterbar must keave keyboard focus in the
        search bar.
        """
        self.dash.reveal_application_lens()
        filter_bar = self.dash.get_current_lens().get_filterbar()
        filter_bar.ensure_collapsed()

        kb = Keyboard()
        kb.type("hello")
        filter_bar.ensure_expanded()
        kb.type(" world")

        searchbar = self.dash.get_searchbar()
        self.assertEqual("hello world", searchbar.search_string)
