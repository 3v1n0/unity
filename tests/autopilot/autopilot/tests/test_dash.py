# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from gtk import Clipboard
from testtools.matchers import Equals

from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.tests import AutopilotTestCase


class DashTestCase(AutopilotTestCase):
    def setUp(self):
        super(DashTestCase, self).setUp()
        self.set_unity_log_level("unity.shell", "DEBUG")
        self.set_unity_log_level("unity.launcher", "DEBUG")
        self.dash.ensure_hidden()
        # On shutdown, ensure hidden too.  Also add a delay.  Cleanup is LIFO.
        self.addCleanup(self.dash.ensure_hidden)
        self.addCleanup(sleep, 1)


class DashRevealTests(DashTestCase):
    """Test the Unity dash Reveal."""

    def test_dash_reveal(self):
        """Ensure we can show and hide the dash."""
        self.dash.ensure_visible()
        self.dash.ensure_hidden()

    def test_application_lens_shortcut(self):
        """Application lense must reveal when Super+a is pressed."""
        self.dash.reveal_application_lens()
        self.assertThat(self.dash.active_lens, Equals('applications.lens'))

    def test_music_lens_shortcut(self):
        """Music lense must reveal when Super+w is pressed."""
        self.dash.reveal_music_lens()
        self.assertThat(self.dash.active_lens, Equals('music.lens'))

    def test_file_lens_shortcut(self):
        """File lense must reveal when Super+f is pressed."""
        self.dash.reveal_file_lens()
        self.assertThat(self.dash.active_lens, Equals('files.lens'))

    def test_command_lens_shortcut(self):
        """Run Command lens must reveat on alt+F2."""
        self.dash.reveal_command_lens()
        self.assertThat(self.dash.active_lens, Equals('commands.lens'))

    def test_alt_f4_close_dash(self):
        """Dash must close on alt+F4."""
        self.dash.ensure_visible()
        self.keyboard.press_and_release("Alt+F4")
        sleep(0.5)
        self.assertFalse(self.dash.visible)


class DashSearchInputTests(DashTestCase):
    """Test features involving input to the dash search"""

    def assertSearchText(self, text):
        sleep(0.5)
        self.assertThat(self.dash.search_string, Equals(text))

    def test_search_keyboard_focus(self):
        """Dash must put keyboard focus on the search bar at all times."""
        self.dash.ensure_visible()
        self.keyboard.type("Hello")
        self.assertSearchText("Hello")

    def test_multi_key(self):
        """Pressing 'Multi_key' must not add any characters to the search."""
        self.dash.reveal_application_lens()
        self.keyboard.press_and_release('Multi_key')
        self.keyboard.type("o")
        self.assertSearchText("")

    def test_multi_key_o(self):
        """Pressing the sequences 'Multi_key' + '^' + 'o' must produce 'ô'."""
        self.dash.reveal_application_lens()
        self.keyboard.press_and_release('Multi_key')
        self.keyboard.type("^o")
        self.assertSearchText("ô")

    def test_multi_key_copyright(self):
        """Pressing the sequences 'Multi_key' + 'c' + 'o' must produce '©'."""
        self.dash.reveal_application_lens()
        self.keyboard.press_and_release('Multi_key')
        self.keyboard.type("oc")
        self.assertSearchText("©")

    def test_multi_key_delete(self):
        """Pressing 'Multi_key' must not get stuck looking for a sequence."""
        self.dash.reveal_application_lens()
        self.keyboard.type("dd")
        self.keyboard.press_and_release('Multi_key')
        self.keyboard.press_and_release('BackSpace')
        self.keyboard.press_and_release('BackSpace')
        self.assertSearchText("d")


class DashKeyNavTests(DashTestCase):
    """Test the unity Dash keyboard navigation."""

    def test_lensbar_gets_keyfocus(self):
        """Test that the lensbar gets key focus after using Down keypresses."""
        self.dash.ensure_visible()

        # Make sure that the lens bar can get the focus
        for i in range(self.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        lensbar = self.dash.view.get_lensbar()
        self.assertIsNot(lensbar.focused_lens_icon, '')

    def test_lensbar_focus_changes(self):
        """Lensbar focused icon should change with Left and Right keypresses."""
        self.dash.ensure_visible()

        for i in range(self.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        lensbar = self.dash.view.get_lensbar()
        current_focused_icon = lensbar.focused_lens_icon
        self.keyboard.press_and_release("Right");
        lensbar.refresh_state()
        self.assertNotEqual(lensbar.focused_lens_icon, current_focused_icon)
        self.keyboard.press_and_release("Left")
        lensbar.refresh_state()
        self.assertEqual(lensbar.focused_lens_icon, current_focused_icon)

    def test_lensbar_enter_activation(self):
        """Must be able to activate LensBar icons that have focus with an Enter keypress."""
        self.dash.ensure_visible()

        for i in range(self.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        self.keyboard.press_and_release("Right");
        lensbar = self.dash.view.get_lensbar()
        focused_icon = lensbar.focused_lens_icon
        self.keyboard.press_and_release("Enter");
        lensbar.refresh_state()
        self.assertEqual(lensbar.active_lens, focused_icon)

        # lensbar should lose focus after activation.
        # TODO this should be a different test to make sure focus
        # returns to the correct place.
        self.assertEqual(lensbar.focused_lens_icon, "")

    def test_category_header_keynav_autoscroll(self):
        """Tests that the dash scrolls a category header into view when scrolling via
        the keyboard.

        Test for lp:919563
        """

        reason = """
        False assumptions. Expanding the first category will not necessarily expand enough
        to force the next category header off screen.
        """
        self.skipTest(reason)

        self.dash.ensure_visible()
        lens = self.dash.get_current_lens()

        # Expand the first category
        self.keyboard.press_and_release("Down")
        self.keyboard.press_and_release("Enter")
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
        self.keyboard.press_and_release("Down")
        self.keyboard.press_and_release("Enter")
        category = app_lens.get_focused_category()
        y = category.header_y

        # Make sure the dash autoscroll
        self.assertTrue(abs(y - cached_y) < 30)

    def test_category_header_keynav(self):
        """ Tests that a category header gets focus when 'down' is pressed after the
        dash is opened

        OK important to note that this test only tests that A category is
        focused, not the first and from doing this it seems that it's common
        for a header other than the first to get focus.
        """
        self.dash.ensure_visible()
        # Make sure that a category have the focus.
        self.keyboard.press_and_release("Down")
        lens = self.dash.get_current_lens()
        category = lens.get_focused_category()
        self.assertIsNot(category, None)
        # Make sure that the category is highlighted.
        self.assertTrue(category.header_is_highlighted)

    def test_maintain_highlight(self):
        # Get the geometry of that category header.
        self.skipTest('Not implemented at all. Broken out of another test but not reworked')
        mouse = Mouse()

        x = category.header_x
        y = category.header_y
        w = category.header_width
        h = category.header_height

        # Move the mouse close the view, and press down.
        mouse.move(x + w + 10,
                    y + h / 2,
                    True)
        sleep(1)
        self.keyboard.press_and_release("Down")
        lens = self.dash.get_current_lens()
        category = lens.get_focused_category()
        self.assertEqual(category, None)

    def test_control_tab_lens_cycle(self):
        """ This test makes sure that Ctlr + Tab cycles lenses."""
        self.dash.ensure_visible()

        self.keyboard.press('Control')
        self.keyboard.press_and_release('Tab')
        self.keyboard.release('Control')

        lensbar = self.dash.view.get_lensbar()
        self.assertEqual(lensbar.active_lens, u'applications.lens')

        self.keyboard.press('Control')
        self.keyboard.press('Shift')
        self.keyboard.press_and_release('Tab')
        self.keyboard.release('Control')
        self.keyboard.release('Shift')

        lensbar.refresh_state()
        self.assertEqual(lensbar.active_lens, u'home.lens')

    def test_tab_cycle_category_headers(self):
        """ Makes sure that pressing tab cycles through the category headers"""
        self.dash.ensure_visible()
        lens = self.dash.get_current_lens()

        # Test that tab cycles through the categories.
        # + 1 is to cycle back to first header
        for i in range(lens.get_num_visible_categories() + 1):
            self.keyboard.press_and_release('Tab')
            category = lens.get_focused_category()
            self.assertIsNot(category, None)

    def test_tab_with_filter_bar(self):
        """ This test makes sure that Tab works well with the filter bara."""
        self.dash.reveal_application_lens()
        lens = self.dash.get_current_lens()

        # Tabs to last category
        for i in range(lens.get_num_visible_categories()):
            self.keyboard.press_and_release('Tab')

        self.keyboard.press_and_release('Tab')
        searchbar = self.dash.get_searchbar()
        self.assertTrue(searchbar.expander_has_focus)

        filter_bar = lens.get_filterbar()
        if not searchbar.showing_filters:
            self.keyboard.press_and_release('Enter')
            self.assertTrue(searchbar.showing_filters)
            self.addCleanup(filter_bar.ensure_collapsed)

        for i in range(filter_bar.get_num_filters()):
            self.keyboard.press_and_release('Tab')
            new_focused_filter = filter_bar.get_focused_filter()
            self.assertIsNotNone(new_focused_filter)

        # Ensure that tab cycles back to a category header
        self.keyboard.press_and_release('Tab')
        category = lens.get_focused_category()
        self.assertIsNot(category, None)


class DashClipboardTests(DashTestCase):
    """Test the Unity clipboard"""

    def test_ctrl_a(self):
        """ This test if ctrl+a selects all text """
        self.dash.ensure_visible()

        self.keyboard.type("SelectAll")
        sleep(1)
        self.assertThat(self.dash.search_string, Equals('SelectAll'))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Delete")
        self.assertThat(self.dash.search_string, Equals(''))

    def test_ctrl_c(self):
        """ This test if ctrl+c copies text into the clipboard """
        self.dash.ensure_visible()

        self.keyboard.type("Copy")
        sleep(1)

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+c")

        cb = Clipboard(selection="CLIPBOARD")

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, cb.wait_for_text())

    def test_ctrl_x(self):
        """ This test if ctrl+x deletes all text and copys it """
        self.dash.ensure_visible()

        self.keyboard.type("Cut")
        sleep(1)

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+x")
        sleep(1)

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, u'')

        cb = Clipboard(selection="CLIPBOARD")
        self.assertEqual(cb.wait_for_text(), u'Cut')

    def test_ctrl_c_v(self):
        """ This test if ctrl+c and ctrl+v copies and pastes text"""
        self.dash.ensure_visible()

        self.keyboard.type("CopyPaste")
        sleep(1)

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+c")
        self.keyboard.press_and_release("Ctrl+v")
        self.keyboard.press_and_release("Ctrl+v")

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, u'CopyPasteCopyPaste')

    def test_ctrl_x_v(self):
        """ This test if ctrl+x and ctrl+v cuts and pastes text"""
        self.dash.ensure_visible()

        self.keyboard.type("CutPaste")
        sleep(1)

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+x")
        self.keyboard.press_and_release("Ctrl+v")
        self.keyboard.press_and_release("Ctrl+v")

        searchbar = self.dash.get_searchbar()
        self.assertEqual(searchbar.search_string, u'CutPasteCutPaste')


class DashKeyboardFocusTests(DashTestCase):
    """Tests that keyboard focus works."""

    def test_filterbar_expansion_leaves_kb_focus(self):
        """Expanding or collapsing the filterbar must keave keyboard focus in the
        search bar.
        """
        self.dash.reveal_application_lens()
        filter_bar = self.dash.get_current_lens().get_filterbar()
        filter_bar.ensure_collapsed()

        self.keyboard.type("hello")
        filter_bar.ensure_expanded()
        self.addCleanup(filter_bar.ensure_collapsed)
        self.keyboard.type(" world")
        self.assertThat(self.dash.search_string, Equals("hello world"))


class DashLensResultsTests(DashTestCase):
    """Tests results from the lens view."""

    def test_results_message_empty_search(self):
        """This tests a message is not shown when there is no text."""
        self.dash.reveal_application_lens()
        lens = self.dash.get_current_lens()
        self.assertFalse(lens.no_results_active)

    def test_results_message(self):
        """This test no mesage will be shown when results are there."""
        self.dash.reveal_application_lens()
        self.keyboard.type("Terminal")
        sleep(1)
        lens = self.dash.get_current_lens()
        self.assertFalse(lens.no_results_active)

    def test_no_results_message(self):
        """This test shows a message will appear in the lens."""
        self.dash.reveal_application_lens()
        self.keyboard.type("qwerlkjzvxc")
        sleep(1)
        lens = self.dash.get_current_lens()
        self.assertTrue(lens.no_results_active)

    def test_results_update_on_filter_changed(self):
        """This test makes sure the results change when filters change."""
        self.dash.reveal_application_lens()
        lens = self.dash.get_current_lens()
        self.keyboard.type(" ")
        sleep(1)
        results_category = lens.get_category_by_name("Installed")
        old_results = results_category.get_results()

        
        def activate_filter(add_cleanup = False):
            # Tabs to last category
            for i in range(lens.get_num_visible_categories()):
                self.keyboard.press_and_release('Tab')

            self.keyboard.press_and_release('Tab')
            searchbar = self.dash.get_searchbar()
            self.assertTrue(searchbar.expander_has_focus)

            filter_bar = lens.get_filterbar()
            if not searchbar.showing_filters:
                self.keyboard.press_and_release('Enter')
                self.assertTrue(searchbar.showing_filters)
                if add_cleanup: self.addCleanup(filter_bar.ensure_collapsed)

            # Tab to the "Type" filter in apps lens
            self.keyboard.press_and_release('Tab')
            new_focused_filter = filter_bar.get_focused_filter()
            self.assertIsNotNone(new_focused_filter)

            self.keyboard.press_and_release("Down")
            self.keyboard.press_and_release("Down")
            self.keyboard.press_and_release("Down")
            # We should be on the Education category
            self.keyboard.press_and_release('Enter')

        activate_filter(True)
        self.addCleanup(activate_filter)

        sleep(1)
        results_category = lens.get_category_by_name("Installed")
        results = results_category.get_results()
        self.assertIsNot(results, old_results)

        # so we can clean up properly
        self.keyboard.press_and_release('BackSpace')


class DashVisualTests(DashTestCase):
    """Tests that the dash visual is correct."""

    def test_see_more_result_alignment(self):
        """The see more results label should be baseline aligned
        with the category name label.
        """
        self.dash.reveal_application_lens()

        lens = self.dash.get_current_lens()
        groups = lens.get_groups()

        for group in groups:
            if (group.is_visible and group.expand_label_is_visible):
                expand_label_y = group.expand_label_y + group.expand_label_baseline
                name_label_y = group.name_label_y + group.name_label_baseline
                self.assertThat(expand_label_y, Equals(name_label_y))
