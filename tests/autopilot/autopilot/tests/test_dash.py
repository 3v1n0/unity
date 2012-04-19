# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from gtk import Clipboard
from testtools.matchers import Equals, NotEquals

from autopilot.matchers import Eventually
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
        self.assertThat(self.dash.active_lens, Eventually(Equals('applications.lens')))

    def test_music_lens_shortcut(self):
        """Music lense must reveal when Super+w is pressed."""
        self.dash.reveal_music_lens()
        self.assertThat(self.dash.active_lens, Eventually(Equals('music.lens')))

    def test_file_lens_shortcut(self):
        """File lense must reveal when Super+f is pressed."""
        self.dash.reveal_file_lens()
        self.assertThat(self.dash.active_lens, Eventually(Equals('files.lens')))

    def test_command_lens_shortcut(self):
        """Run Command lens must reveat on alt+F2."""
        self.dash.reveal_command_lens()
        self.assertThat(self.dash.active_lens, Eventually(Equals('commands.lens')))

    def test_alt_f4_close_dash(self):
        """Dash must close on alt+F4."""
        self.dash.ensure_visible()
        self.keyboard.press_and_release("Alt+F4")
        self.assertThat(self.dash.visible, Eventually(Equals(False)))


class DashSearchInputTests(DashTestCase):
    """Test features involving input to the dash search"""

    def assertSearchText(self, text):
        self.assertThat(self.dash.search_string, Eventually(Equals(text)))

    def test_search_keyboard_focus(self):
        """Dash must put keyboard focus on the search bar at all times."""
        self.dash.ensure_visible()
        self.keyboard.type("Hello")
        self.assertSearchText("Hello")


class DashMultiKeyTests(DashSearchInputTests):
    def setUp(self):
        # set the multi key first so that we're not getting a new _DISPLAY while keys are held down.
        old_value = self.call_gsettings_cmd('get', 'org.gnome.libgnomekbd.keyboard', 'options')
        self.addCleanup(self.call_gsettings_cmd, 'set', 'org.gnome.libgnomekbd.keyboard', 'options', old_value)
        self.call_gsettings_cmd('set', 'org.gnome.libgnomekbd.keyboard', 'options', "['Compose key\tcompose:caps']")

        super(DashMultiKeyTests, self).setUp()

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
        self.assertThat(lensbar.focused_lens_icon, Eventually(NotEquals('')))

    def test_lensbar_focus_changes(self):
        """Lensbar focused icon should change with Left and Right keypresses."""
        self.dash.ensure_visible()

        for i in range(self.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        lensbar = self.dash.view.get_lensbar()

        current_focused_icon = lensbar.focused_lens_icon

        self.keyboard.press_and_release("Right");
        self.assertThat(lensbar.focused_lens_icon, Eventually(NotEquals(current_focused_icon)))

        self.keyboard.press_and_release("Left")
        self.assertThat(lensbar.focused_lens_icon, Eventually(Equals(current_focused_icon)))

    def test_lensbar_enter_activation(self):
        """Must be able to activate LensBar icons that have focus with an Enter keypress."""
        self.dash.ensure_visible()

        for i in range(self.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        self.keyboard.press_and_release("Right");
        lensbar = self.dash.view.get_lensbar()
        focused_icon = lensbar.focused_lens_icon
        self.keyboard.press_and_release("Enter");

        self.assertThat(lensbar.active_lens, Eventually(Equals(focused_icon)))

        # lensbar should lose focus after activation.
        # TODO this should be a different test to make sure focus
        # returns to the correct place.
        self.assertThat(lensbar.focused_lens_icon, Eventually(Equals("")))

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

    def test_control_tab_lens_cycle(self):
        """This test makes sure that Ctrl+Tab cycles lenses."""
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

        self.assertThat(lensbar.active_lens, Eventually(Equals('home.lens')))

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
        self.assertThat(self.dash.searchbar.expander_has_focus, Eventually(Equals(True)))

        filter_bar = lens.get_filterbar()
        if not self.dash.searchbar.showing_filters:
            self.keyboard.press_and_release('Enter')
            self.assertThat(self.dash.searchbar.showing_filters, Eventually(Equals(True)))
            self.addCleanup(filter_bar.ensure_collapsed)

        for i in range(filter_bar.get_num_filters()):
            self.keyboard.press_and_release('Tab')
            new_focused_filter = filter_bar.get_focused_filter()
            self.assertIsNotNone(new_focused_filter)

        # Ensure that tab cycles back to a category header
        self.keyboard.press_and_release('Tab')
        category = lens.get_focused_category()
        self.assertIsNot(category, None)

    def test_alt_f1_disabled(self):
        """This test that Alt+F1 is disabled when the dash is opened."""
        self.dash.ensure_visible()
        # can't use launcher emulator since we'll fail to start keynav:
        self.keybinding("launcher/keynav")
        # can't use Eventually here - sleep long enough for the launcher controller
        # to react to the keypress (well, hopefully not)
        sleep(5)
        self.assertThat(self.launcher.key_nav_is_active, Equals(False))


class DashClipboardTests(DashTestCase):
    """Test the Unity clipboard"""

    def test_ctrl_a(self):
        """ This test if ctrl+a selects all text """
        self.dash.ensure_visible()

        self.keyboard.type("SelectAll")
        self.assertThat(self.dash.search_string, Eventually(Equals("SelectAll")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Delete")
        self.assertThat(self.dash.search_string, Eventually(Equals('')))

    def test_ctrl_c(self):
        """ This test if ctrl+c copies text into the clipboard """
        self.dash.ensure_visible()

        self.keyboard.type("Copy")
        self.assertThat(self.dash.search_string, Eventually(Equals("Copy")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+c")

        cb = Clipboard(selection="CLIPBOARD")
        self.assertThat(self.dash.search_string, Eventually(Equals(cb.wait_for_text())))

    def test_ctrl_x(self):
        """ This test if ctrl+x deletes all text and copys it """
        self.dash.ensure_visible()

        self.keyboard.type("Cut")
        self.assertThat(self.dash.search_string, Eventually(Equals("Cut")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+x")
        self.assertThat(self.dash.search_string, Eventually(Equals("")))

        cb = Clipboard(selection="CLIPBOARD")
        self.assertEqual(cb.wait_for_text(), u'Cut')

    def test_ctrl_c_v(self):
        """ This test if ctrl+c and ctrl+v copies and pastes text"""
        self.dash.ensure_visible()

        self.keyboard.type("CopyPaste")
        self.assertThat(self.dash.search_string, Eventually(Equals("CopyPaste")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+c")
        self.keyboard.press_and_release("Ctrl+v")
        self.keyboard.press_and_release("Ctrl+v")

        self.assertThat(self.dash.search_string, Eventually(Equals('CopyPasteCopyPaste')))

    def test_ctrl_x_v(self):
        """ This test if ctrl+x and ctrl+v cuts and pastes text"""
        self.dash.ensure_visible()

        self.keyboard.type("CutPaste")
        self.assertThat(self.dash.search_string, Eventually(Equals("CutPaste")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+x")
        self.keyboard.press_and_release("Ctrl+v")
        self.keyboard.press_and_release("Ctrl+v")

        self.assertThat(self.dash.search_string, Eventually(Equals('CutPasteCutPaste')))


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
        self.assertThat(self.dash.search_string, Eventually(Equals("hello world")))


class DashLensResultsTests(DashTestCase):
    """Tests results from the lens view."""

    def test_results_message_empty_search(self):
        """This tests a message is not shown when there is no text."""
        self.dash.reveal_application_lens()
        lens = self.dash.get_current_lens()
        self.assertThat(lens.no_results_active, Eventually(Equals(False)))

    def test_results_message(self):
        """This test no mesage will be shown when results are there."""
        self.dash.reveal_application_lens()
        self.keyboard.type("Terminal")
        self.assertThat(self.dash.search_string, Eventually(Equals("Terminal")))
        lens = self.dash.get_current_lens()
        self.assertThat(lens.no_results_active, Eventually(Equals(False)))

    def test_no_results_message(self):
        """This test shows a message will appear in the lens."""
        self.dash.reveal_application_lens()
        self.keyboard.type("qwerlkjzvxc")
        self.assertThat(self.dash.search_string, Eventually(Equals("qwerlkjzvxc")))
        lens = self.dash.get_current_lens()
        self.assertThat(lens.no_results_active, Eventually(Equals(True)))

    def test_results_update_on_filter_changed(self):
        """This test makes sure the results change when filters change."""
        self.dash.reveal_application_lens()
        lens = self.dash.get_current_lens()
        self.keyboard.type(" ")
        self.assertThat(self.dash.search_string, Eventually(Equals(" ")))
        results_category = lens.get_category_by_name("Installed")
        old_results = results_category.get_results()

        # FIXME: This should be a method on the dash emulator perhaps, or
        # maybe a proper method of this class. It should NOT be an inline
        # function that is only called once!
        def activate_filter(add_cleanup = False):
            # Tabs to last category
            for i in range(lens.get_num_visible_categories()):
                self.keyboard.press_and_release('Tab')

            self.keyboard.press_and_release('Tab')
            self.assertThat(self.dash.searchbar.expander_has_focus, Eventually(Equals(True)))

            filter_bar = lens.get_filterbar()
            if not self.dash.searchbar.showing_filters:
                self.keyboard.press_and_release('Enter')
                self.assertThat(self.dash.searchbar.showing_filters, Eventually(Equals(True)))
                if add_cleanup:
                    self.addCleanup(filter_bar.ensure_collapsed)

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


class DashLensBarTests(DashTestCase):
    """Tests that the lensbar works well."""

    def setUp(self):
        super(DashLensBarTests, self).setUp()
        self.dash.ensure_visible()
        self.lensbar = self.dash.view.get_lensbar()

    def test_click_inside_highlight(self):
        """Lens selection should work when clicking in
        the rectangle outside of the icon.
        """
        app_icon = self.lensbar.get_icon_by_name(u'applications.lens')

        self.mouse.move(app_icon.x + (app_icon.width / 2),
                        app_icon.y + (app_icon.height / 2))
        self.mouse.click()

        self.assertThat(self.lensbar.active_lens, Eventually(Equals('applications.lens')))


class DashBorderTests(DashTestCase):
    """Tests that the dash border works well.
    """
    def setUp(self):
        super(DashBorderTests, self).setUp()
        self.dash.ensure_visible()

    def test_click_right_border(self):
        """Clicking on the right dash border should do nothing,
        *NOT* close the dash.
        """
        if (self.dash.view.form_factor != "desktop"):
            self.skip("Not in desktop form-factor.")

        x = self.dash.view.x + self.dash.view.width + self.dash.view.right_border_width / 2;
        y = self.dash.view.y + self.dash.view.height / 2;

        self.mouse.move(x, y)
        self.mouse.click()

        self.assertThat(self.dash.visible, Eventually(Equals(True)))

    def test_click_bottom_border(self):
        """Clicking on the bottom dash border should do nothing,
        *NOT* close the dash.
        """
        if (self.dash.view.form_factor != "desktop"):
            self.skip("Not in desktop form-factor.")

        x = self.dash.view.x + self.dash.view.width / 2;
        y = self.dash.view.y + self.dash.view.height + self.dash.view.bottom_border_height / 2;

        self.mouse.move(x, y)
        self.mouse.click()

        self.assertThat(self.dash.visible, Eventually(Equals(True)))

