# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.clipboard import get_clipboard_contents
from autopilot.display import move_mouse_to_screen
from autopilot.matchers import Eventually
from testtools.matchers import Equals, NotEquals, GreaterThan
from time import sleep
from tempfile import mkstemp
from os import remove

from unity.tests import UnityTestCase

import gettext

class DashTestCase(UnityTestCase):
    def setUp(self):
        super(DashTestCase, self).setUp()
        self.set_unity_log_level("unity.shell.compiz", "DEBUG")
        self.set_unity_log_level("unity.launcher", "DEBUG")
        self.unity.dash.ensure_hidden()
        # On shutdown, ensure hidden too.  Also add a delay.  Cleanup is LIFO.
        self.addCleanup(self.unity.dash.ensure_hidden)
        self.addCleanup(sleep, 1)

    def get_current_preview(self):
        """Method to open the currently selected preview, if opened."""
        preview_fn = lambda: self.preview_container.current_preview
        self.assertThat(preview_fn, Eventually(NotEquals(None)))
        self.assertThat(self.unity.dash.preview_animation, Eventually(Equals(1.0)))
        self.assertThat(self.preview_container.animating, Eventually(Equals(False)))

        return preview_fn()

    def wait_for_category(self, scope, group):
        """Method to wait for a specific category"""
        get_scope_fn = lambda: scope.get_category_by_name(group)
        self.assertThat(get_scope_fn, Eventually(NotEquals(None), timeout=20))
        return get_scope_fn()

    def wait_for_result_settle(self):
        """wait for row count to settle"""
        old_row_count = -1
        new_row_count = self.unity.dash.get_num_rows()
        while(old_row_count != new_row_count):
            sleep(1)
            old_row_count = new_row_count
            new_row_count = self.unity.dash.get_num_rows()


class DashRevealTests(DashTestCase):
    """Test the Unity dash Reveal."""

    def test_dash_reveal(self):
        """Ensure we can show and hide the dash."""
        self.unity.dash.ensure_visible()
        self.unity.dash.ensure_hidden()

    def test_application_scope_shortcut(self):
        """Application scope must reveal when Super+a is pressed."""
        self.unity.dash.reveal_application_scope()
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals('applications.scope')))

    def test_music_scope_shortcut(self):
        """Music scope must reveal when Super+w is pressed."""
        self.unity.dash.reveal_music_scope()
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals('music.scope')))

    def test_file_scope_shortcut(self):
        """File scope must reveal when Super+f is pressed."""
        self.unity.dash.reveal_file_scope()
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals('files.scope')))

    def test_video_scope_shortcut(self):
        """Video scope must reveal when super+v is pressed."""
        self.unity.dash.reveal_video_scope()
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals('video.scope')))

    def test_command_scope_shortcut(self):
        """Run Command scope must reveat on alt+F2."""
        self.unity.dash.reveal_command_scope()
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals('commands.scope')))

    def test_can_go_from_dash_to_command_scope(self):
        """Switch to command scope without closing the dash."""
        self.unity.dash.ensure_visible()
        self.unity.dash.reveal_command_scope()
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals('commands.scope')))

    def test_command_lens_can_close_itself(self):
        """We must be able to close the Command lens with Alt+F2"""
        self.unity.dash.reveal_command_scope()
        self.assertThat(self.unity.dash.visible, Eventually(Equals(True)))

        self.keybinding("lens_reveal/command")
        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))

    def test_alt_f4_close_dash(self):
        """Dash must close on alt+F4."""
        self.unity.dash.ensure_visible()
        self.keyboard.press_and_release("Alt+F4")
        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))

    def test_alt_f4_close_dash_with_capslock_on(self):
        """Dash must close on Alt+F4 even when the capslock is turned on."""
        self.keyboard.press_and_release("Caps_Lock")
        self.addCleanup(self.keyboard.press_and_release, "Caps_Lock")

        self.unity.dash.ensure_visible()
        self.keyboard.press_and_release("Alt+F4")
        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))

    def test_closes_mouse_down_outside(self):
        """Test that a mouse down outside of the dash closes the dash."""

        self.unity.dash.ensure_visible()
        current_monitor = self.unity.dash.monitor

        (x,y,w,h) = self.unity.dash.geometry
        (screen_x,screen_y,screen_w,screen_h) = self.display.get_screen_geometry(current_monitor)

        self.mouse.move(x + w + (screen_w-((screen_x-x)+w))/2, y + h + (screen_h-((screen_y-y)+h))/2)
        self.mouse.click()

        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))

    def test_closes_then_focuses_window_on_mouse_down(self):
        """If 2 windows are open with 1 maximized and the non-maxmized
        focused. Then from the Dash clicking on the maximized window
        must focus that window and close the dash.
        """
        char_win = self.process_manager.start_app("Character Map")
        self.keybinding("window/maximize")
        self.process_manager.start_app("Calculator")

        self.unity.dash.ensure_visible()

        #Click bottom right of the screen
        w = self.display.get_screen_width()
        h = self.display.get_screen_height()
        self.mouse.move(w,h)
        self.mouse.click()

        self.assertProperty(char_win, is_active=True)


class DashRevealWithSpreadTests(DashTestCase):
    """Test the interaction of the Dash with the Spead/Scale

    The Spread (or Scale) in Quantal is not activated if there is no active
    apps. We use a place holder app so that it is activated as we require.

    """

    def start_placeholder_app(self):
        window_spec = {
            "Title": "Placeholder application",
        }
        self.launch_test_window(window_spec)

    def test_dash_closes_on_spread(self):
        """This test shows that when the spread is initiated, the dash closes."""
        self.start_placeholder_app()
        self.unity.dash.ensure_visible()
        self.addCleanup(self.keybinding, "spread/cancel")
        self.keybinding("spread/start")
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))
        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))

    def test_dash_opens_when_in_spread(self):
        """This test shows the dash opens when in spread mode."""
        self.start_placeholder_app()
        self.keybinding("spread/start")
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))

        self.unity.dash.ensure_visible()
        self.assertThat(self.unity.dash.visible, Eventually(Equals(True)))

    def test_command_scope_opens_when_in_spread(self):
        """This test shows the command scope opens when in spread mode."""
        self.start_placeholder_app()
        self.keybinding("spread/start")
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))

        self.unity.dash.reveal_command_scope()
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals('commands.scope')))

    def test_scope_opens_when_in_spread(self):
        """This test shows that any scope opens when in spread mode."""
        self.start_placeholder_app()
        self.keybinding("spread/start")
        self.assertThat(self.unity.window_manager.scale_active, Eventually(Equals(True)))

        self.unity.dash.reveal_application_scope()
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals('applications.scope')))


class DashSearchInputTests(DashTestCase):
    """Test features involving input to the dash search"""

    def assertSearchText(self, text):
        self.assertThat(self.unity.dash.search_string, Eventually(Equals(text)))

    def test_search_keyboard_focus(self):
        """Dash must put keyboard focus on the search bar at all times."""
        self.unity.dash.ensure_visible()
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
        self.unity.dash.reveal_application_scope()
        self.keyboard.press_and_release('Multi_key')
        self.keyboard.type("o")
        self.assertSearchText("")

    def test_multi_key_o(self):
        """Pressing the sequences 'Multi_key' + '^' + 'o' must produce 'ô'."""
        self.unity.dash.reveal_application_scope()
        self.keyboard.press_and_release('Multi_key')
        self.keyboard.type("^o")
        self.assertSearchText(u'\xf4')

    def test_multi_key_copyright(self):
        """Pressing the sequences 'Multi_key' + 'c' + 'o' must produce '©'."""
        self.unity.dash.reveal_application_scope()
        self.keyboard.press_and_release('Multi_key')
        self.keyboard.type("oc")
        self.assertSearchText(u'\xa9')

    def test_multi_key_delete(self):
        """Pressing 'Multi_key' must not get stuck looking for a sequence."""
        self.unity.dash.reveal_application_scope()
        self.keyboard.type("dd")
        self.keyboard.press_and_release('Multi_key')
        self.keyboard.press_and_release('BackSpace')
        self.keyboard.press_and_release('BackSpace')
        self.assertSearchText("d")


class DashKeyNavTests(DashTestCase):
    """Test the unity Dash keyboard navigation."""

    def test_scopebar_gets_keyfocus(self):
        """Test that the scopebar gets key focus after using Down keypresses."""
        self.unity.dash.ensure_visible()
        self.wait_for_result_settle()

        # Make sure that the scope bar can get the focus
        for i in range(self.unity.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        scopebar = self.unity.dash.view.get_scopebar()
        self.assertThat(scopebar.focused_scope_icon, Eventually(NotEquals('')))

    def test_scopebar_focus_changes(self):
        """Scopebar focused icon should change with Left and Right keypresses."""
        self.unity.dash.ensure_visible()
        self.wait_for_result_settle()

        for i in range(self.unity.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        scopebar = self.unity.dash.view.get_scopebar()

        current_focused_icon = scopebar.focused_scope_icon

        self.keyboard.press_and_release("Right")
        self.assertThat(scopebar.focused_scope_icon, Eventually(NotEquals(current_focused_icon)))

        self.keyboard.press_and_release("Left")
        self.assertThat(scopebar.focused_scope_icon, Eventually(Equals(current_focused_icon)))

    def test_scopebar_enter_activation(self):
        """Must be able to activate ScopeBar icons that have focus with an Enter keypress."""
        self.unity.dash.ensure_visible()
        self.wait_for_result_settle()

        for i in range(self.unity.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        self.keyboard.press_and_release("Right")
        scopebar = self.unity.dash.view.get_scopebar()
        focused_icon = scopebar.focused_scope_icon
        self.keyboard.press_and_release("Enter")

        self.assertThat(scopebar.active_scope, Eventually(Equals(focused_icon)))

        # scopebar should lose focus after activation.
        self.assertThat(scopebar.focused_scope_icon, Eventually(Equals("")))

    def test_focus_returns_to_searchbar(self):
        """This test makes sure that the focus is returned to the searchbar of the newly
        activated scope."""
        self.unity.dash.ensure_visible()
        self.wait_for_result_settle()

        for i in range(self.unity.dash.get_num_rows()):
            self.keyboard.press_and_release("Down")
        self.keyboard.press_and_release("Right")
        scopebar = self.unity.dash.view.get_scopebar()
        focused_icon = scopebar.focused_scope_icon
        self.keyboard.press_and_release("Enter")

        self.assertThat(scopebar.active_scope, Eventually(Equals(focused_icon)))
        self.assertThat(scopebar.focused_scope_icon, Eventually(Equals("")))

        # Now we make sure if the newly activated scope searchbar have the focus.
        self.keyboard.type("HasFocus")

        self.assertThat(self.unity.dash.search_string, Eventually(Equals("HasFocus")))

    def test_category_header_keynav(self):
        """ Tests that a category header gets focus when 'down' is pressed after the
        dash is opened

        OK important to note that this test only tests that A category is
        focused, not the first and from doing this it seems that it's common
        for a header other than the first to get focus.
        """
        self.unity.dash.ensure_visible()
        self.wait_for_result_settle()
        # Make sure that a category have the focus.
        self.keyboard.press_and_release("Down")
        scope = self.unity.dash.get_current_scope()
        category = scope.get_focused_category()
        self.assertIsNot(category, None)
        # Make sure that the category is highlighted.
        self.assertTrue(category.header_is_highlighted)

    def test_control_tab_scope_cycle(self):
        """This test makes sure that Ctrl+Tab cycles scopes."""
        self.unity.dash.ensure_visible()
        self.wait_for_result_settle()

        self.keyboard.press('Control')
        self.keyboard.press_and_release('Tab')
        self.keyboard.release('Control')

        scopebar = self.unity.dash.view.get_scopebar()
        self.assertEqual(scopebar.active_scope, u'applications.scope')

        self.keyboard.press('Control')
        self.keyboard.press('Shift')
        self.keyboard.press_and_release('Tab')
        self.keyboard.release('Control')
        self.keyboard.release('Shift')

        self.assertThat(scopebar.active_scope, Eventually(Equals('home.scope')))

    def test_tab_cycle_category_headers(self):
        """ Makes sure that pressing tab cycles through the category headers"""
        self.unity.dash.ensure_visible()
        self.wait_for_result_settle()
        scope = self.unity.dash.get_current_scope()

        # Test that tab cycles through the categories.
        # + 1 is the filter bar
        for i in range(scope.get_num_visible_categories()):
            self.keyboard.press_and_release('Tab')
            category = scope.get_focused_category()
            self.assertIsNot(category, None)

    def test_tab_with_filter_bar(self):
        """ This test makes sure that Tab works well with the filter bara."""
        self.unity.dash.reveal_application_scope()
        self.wait_for_result_settle()
        scope = self.unity.dash.get_current_scope()

        # Tabs to last category
        for i in range(scope.get_num_visible_categories()):
            self.keyboard.press_and_release('Tab')

        self.keyboard.press_and_release('Tab')
        self.assertThat(self.unity.dash.searchbar.expander_has_focus, Eventually(Equals(True)))

        filter_bar = scope.get_filterbar()
        if not self.unity.dash.searchbar.showing_filters:
            self.keyboard.press_and_release('Enter')
            self.assertThat(self.unity.dash.searchbar.showing_filters, Eventually(Equals(True)))
            self.addCleanup(filter_bar.ensure_collapsed)

        for i in range(filter_bar.get_num_filters()):
            self.keyboard.press_and_release('Tab')
            new_focused_filter = filter_bar.get_focused_filter()
            self.assertIsNotNone(new_focused_filter)

        # Ensure that tab cycles back to a category header
        self.keyboard.press_and_release('Tab')
        category = scope.get_focused_category()
        self.assertIsNot(category, None)

    def test_bottom_up_keynav_with_filter_bar(self):
        """This test makes sure that bottom-up key navigation works well
        in the dash filter bar.
        """
        self.unity.dash.reveal_application_scope()
        self.wait_for_result_settle()
        scope = self.unity.dash.get_current_scope()

        filter_bar = scope.get_filterbar()
        # Need to ensure the filter expander has focus, so if it's already
        # expanded, we collapse it first:
        filter_bar.ensure_collapsed()
        filter_bar.ensure_expanded()

        # Tab to fist filter expander
        self.keyboard.press_and_release('Tab')
        self.assertThat(filter_bar.get_focused_filter, Eventually(NotEquals(None)))
        old_focused_filter = filter_bar.get_focused_filter()
        old_focused_filter.ensure_expanded()

        # Tab to the next filter expander
        self.keyboard.press_and_release('Tab')
        self.assertThat(filter_bar.get_focused_filter, Eventually(NotEquals(None)))
        new_focused_filter = filter_bar.get_focused_filter()
        self.assertNotEqual(old_focused_filter, new_focused_filter)
        new_focused_filter.ensure_expanded()

        # Move the focus up.
        self.keyboard.press_and_release("Up")
        self.assertThat(filter_bar.get_focused_filter, Eventually(Equals(None)))
        self.assertThat(old_focused_filter.content_has_focus, Eventually(Equals(True)))


class DashClipboardTests(DashTestCase):
    """Test the Unity clipboard"""

    def test_ctrl_a(self):
        """ This test if ctrl+a selects all text """
        self.unity.dash.ensure_visible()

        self.keyboard.type("SelectAll")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("SelectAll")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Delete")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals('')))

    def test_ctrl_c(self):
        """ This test if ctrl+c copies text into the clipboard """
        self.unity.dash.ensure_visible()

        self.keyboard.type("Copy")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("Copy")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+c")

        self.assertThat(get_clipboard_contents, Eventually(Equals("Copy")))

    def test_ctrl_x(self):
        """ This test if ctrl+x deletes all text and copys it """
        self.unity.dash.ensure_visible()

        self.keyboard.type("Cut")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("Cut")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+x")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("")))

        self.assertThat(get_clipboard_contents, Eventually(Equals('Cut')))

    def test_ctrl_c_v(self):
        """ This test if ctrl+c and ctrl+v copies and pastes text"""
        self.unity.dash.ensure_visible()

        self.keyboard.type("CopyPaste")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("CopyPaste")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+c")
        self.keyboard.press_and_release("Ctrl+v")
        self.keyboard.press_and_release("Ctrl+v")

        self.assertThat(self.unity.dash.search_string, Eventually(Equals('CopyPasteCopyPaste')))

    def test_ctrl_x_v(self):
        """ This test if ctrl+x and ctrl+v cuts and pastes text"""
        self.unity.dash.ensure_visible()

        self.keyboard.type("CutPaste")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("CutPaste")))

        self.keyboard.press_and_release("Ctrl+a")
        self.keyboard.press_and_release("Ctrl+x")
        self.keyboard.press_and_release("Ctrl+v")
        self.keyboard.press_and_release("Ctrl+v")

        self.assertThat(self.unity.dash.search_string, Eventually(Equals('CutPasteCutPaste')))

    def test_middle_click_paste(self):
        """Tests if Middle mouse button pastes into searchbar"""

        self.process_manager.start_app_window("Calculator", locale='C')

        self.keyboard.type("ThirdButtonPaste")
        self.keyboard.press_and_release("Ctrl+a")

        self.unity.dash.ensure_visible()

        self.mouse.move(self.unity.dash.searchbar.x + self.unity.dash.searchbar.width / 2,
                       self.unity.dash.searchbar.y + self.unity.dash.searchbar.height / 2)

        self.mouse.click(button=2)

        self.assertThat(self.unity.dash.search_string, Eventually(Equals('ThirdButtonPaste')))


class DashKeyboardFocusTests(DashTestCase):
    """Tests that keyboard focus works."""

    def assertSearchText(self, text):
        self.assertThat(self.unity.dash.search_string, Eventually(Equals(text)))

    def test_filterbar_expansion_leaves_kb_focus(self):
        """Expanding or collapsing the filterbar must keave keyboard focus in the
        search bar.
        """
        self.unity.dash.reveal_application_scope()
        filter_bar = self.unity.dash.get_current_scope().get_filterbar()
        filter_bar.ensure_collapsed()

        self.keyboard.type("hello")
        filter_bar.ensure_expanded()
        self.addCleanup(filter_bar.ensure_collapsed)
        self.keyboard.type(" world")
        self.assertSearchText("hello world")

    def test_keep_focus_on_application_opens(self):
        """The Dash must keep key focus as well as stay open if an app gets opened from an external source. """

        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.process_manager.start_app_window("Calculator")
        sleep(1)

        self.keyboard.type("HasFocus")
        self.assertSearchText("HasFocus")


class DashScopeResultsTests(DashTestCase):
    """Tests results from the scope view."""

    def test_results_message_empty_search(self):
        """This tests a message is not shown when there is no text."""
        self.unity.dash.reveal_application_scope()
        scope = self.unity.dash.get_current_scope()
        self.assertThat(scope.no_results_active, Eventually(Equals(False)))

    def test_results_message(self):
        """This test no mesage will be shown when results are there."""
        self.unity.dash.reveal_application_scope()
        self.keyboard.type("Terminal")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("Terminal")))
        scope = self.unity.dash.get_current_scope()
        self.assertThat(scope.no_results_active, Eventually(Equals(False)))

    def test_no_results_message(self):
        """This test shows a message will appear in the scope."""
        self.unity.dash.reveal_application_scope()
        self.keyboard.type("qwerlkjzvxc")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("qwerlkjzvxc")))
        scope = self.unity.dash.get_current_scope()
        self.assertThat(scope.no_results_active, Eventually(Equals(True)))

    def test_results_update_on_filter_changed(self):
        """This test makes sure the results change when filters change."""
        gettext.install("unity-scope-applications")
        self.unity.dash.reveal_application_scope()
        scope = self.unity.dash.get_current_scope()
        self.keyboard.type(" ")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals(" ")))

        # wait for "Installed" category
        old_results_category = self.wait_for_category(scope, _("Installed"))

        self.assertThat(lambda: len(old_results_category.get_results()), Eventually(GreaterThan(0), timeout=20))
        old_results = old_results_category.get_results()

        # FIXME: This should be a method on the dash emulator perhaps, or
        # maybe a proper method of this class. It should NOT be an inline
        # function that is only called once!
        def activate_filter(add_cleanup = False):
            # Tabs to last category
            for i in range(scope.get_num_visible_categories()):
                self.keyboard.press_and_release('Tab')

            self.keyboard.press_and_release('Tab')
            self.assertThat(self.unity.dash.searchbar.expander_has_focus, Eventually(Equals(True)))

            filter_bar = scope.get_filterbar()
            if not self.unity.dash.searchbar.showing_filters:
                self.keyboard.press_and_release('Enter')
                self.assertThat(self.unity.dash.searchbar.showing_filters, Eventually(Equals(True)))
                if add_cleanup:
                    self.addCleanup(filter_bar.ensure_collapsed)

            # Tab to the "Type" filter in apps scope
            self.keyboard.press_and_release('Tab')
            new_focused_filter = filter_bar.get_focused_filter()
            self.assertIsNotNone(new_focused_filter)

            self.keyboard.press_and_release("Down")
            self.keyboard.press_and_release("Down")
            # We should be on the Customization category
            self.keyboard.press_and_release('Enter')
            sleep(2)

        activate_filter(True)
        self.addCleanup(activate_filter)

        results_category = self.wait_for_category(scope, _("Installed"))
        self.assertThat(lambda: len(results_category.get_results()), Eventually(GreaterThan(0), timeout=20))

        results = results_category.get_results()
        self.assertIsNot(results, old_results)

        # so we can clean up properly
        self.keyboard.press_and_release('BackSpace')


class DashVisualTests(DashTestCase):
    """Tests that the dash visual is correct."""

    def test_closing_dash_hides_current_scope(self):
        """When exiting from the dash the current scope must set it self to not visible."""

        self.unity.dash.ensure_visible()
        scope = self.unity.dash.get_current_scope()
        self.unity.dash.ensure_hidden()

        self.assertThat(scope.visible, Eventually(Equals(False)))

    def test_dash_position_with_non_default_launcher_width(self):
        """There should be no empty space between launcher and dash when the launcher
        has a non-default width.

        """
        monitor = self.unity.dash.monitor
        launcher = self.unity.launcher.get_launcher_for_monitor(monitor)

        self.set_unity_option('icon_size', 60)
        self.assertThat(launcher.icon_size, Eventually(Equals(66)))

        self.unity.dash.ensure_visible()

        self.assertThat(self.unity.dash.geometry[0], Eventually(Equals(launcher.geometry[0] + launcher.geometry[2] - 1)))


    def test_see_more_result_alignment(self):
        """The see more results label should be baseline aligned
        with the category name label.
        """
        self.unity.dash.reveal_application_scope()

        scope = self.unity.dash.get_current_scope()
        self.assertThat(lambda: len(scope.get_groups()), Eventually(GreaterThan(0), timeout=20))

        groups = scope.get_groups()

        for group in groups:
            if (group.is_visible and group.expand_label_is_visible):
                expand_label_y = group.expand_label_y + group.expand_label_baseline
                name_label_y = group.name_label_y + group.name_label_baseline
                self.assertThat(expand_label_y, Equals(name_label_y))


class DashScopeBarTests(DashTestCase):
    """Tests that the scopebar works well."""

    def setUp(self):
        super(DashScopeBarTests, self).setUp()
        self.unity.dash.ensure_visible()
        self.scopebar = self.unity.dash.view.get_scopebar()

    def test_click_inside_highlight(self):
        """Scope selection should work when clicking in
        the rectangle outside of the icon.
        """
        app_icon = self.scopebar.get_icon_by_name(u'applications.scope')

        self.mouse.move(app_icon.x + (app_icon.width / 2),
                        app_icon.y + (app_icon.height / 2))
        self.mouse.click()

        self.assertThat(self.scopebar.active_scope, Eventually(Equals('applications.scope')))


class DashBorderTests(DashTestCase):
    """Tests that the dash border works well.
    """
    def setUp(self):
        super(DashBorderTests, self).setUp()
        self.unity.dash.ensure_visible()

    def test_click_right_border(self):
        """Clicking on the right dash border should do nothing,
        *NOT* close the dash.
        """
        if (self.unity.dash.view.form_factor != "desktop"):
            self.skip("Not in desktop form-factor.")

        x = self.unity.dash.view.x + self.unity.dash.view.width + self.unity.dash.view.right_border_width / 2
        y = self.unity.dash.view.y + self.unity.dash.view.height / 2

        self.mouse.move(x, y)
        self.mouse.click()

        self.assertThat(self.unity.dash.visible, Eventually(Equals(True)))

    def test_click_bottom_border(self):
        """Clicking on the bottom dash border should do nothing,
        *NOT* close the dash.
        """
        if (self.unity.dash.view.form_factor != "desktop"):
            self.skip("Not in desktop form-factor.")

        x = self.unity.dash.view.x + self.unity.dash.view.width / 2
        y = self.unity.dash.view.y + self.unity.dash.view.height + self.unity.dash.view.bottom_border_height / 2

        self.mouse.move(x, y)
        self.mouse.click()

        self.assertThat(self.unity.dash.visible, Eventually(Equals(True)))


class CategoryHeaderTests(DashTestCase):
    """Tests that category headers work.
    """
    def test_click_inside_highlight(self):
        """Clicking into a category highlight must expand/collapse
        the view.
        """
        gettext.install("unity-scope-applications", unicode=True)
        scope = self.unity.dash.reveal_application_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)

        # wait for "Installed" category
        category = self.wait_for_category(scope, _("Installed"))

        is_expanded = category.is_expanded

        self.mouse.move(self.unity.dash.view.x + self.unity.dash.view.width / 2,
                        category.header_y + category.header_height / 2)

        self.mouse.click()
        self.assertThat(category.is_expanded, Eventually(Equals(not is_expanded)))

        self.mouse.click()
        self.assertThat(category.is_expanded, Eventually(Equals(is_expanded)))


class PreviewInvocationTests(DashTestCase):
    """Tests that dash previews can be opened and closed in different
    scopes.
    """
    def test_app_scope_preview_open_close(self):
        """Right-clicking on an application scope result must show
        its preview.

        """
        gettext.install("unity-scope-applications", unicode=True)
        scope = self.unity.dash.reveal_application_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)

        # wait for "More suggestions" category
        category = self.wait_for_category(scope, _("Installed"))

        # wait for some results
        self.assertThat(lambda: len(category.get_results()), Eventually(GreaterThan(0), timeout=20))
        results = category.get_results()

        result = results[0]
        # result.preview handles finding xy co-ords and right mouse-click
        result.preview()
        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(True)))

        self.keyboard.press_and_release("Escape")

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_files_scope_preview_open_close(self):
        """Right-clicking on a files scope result must show its
        preview.
        """
        gettext.install("unity-scope-files", unicode=True)

        # Instead of skipping the test, here we can create a dummy file to open and
        # make sure the scope result is non-empty
        (file_handle, file_path) = mkstemp()
        self.addCleanup(remove, file_path)
        gedit_win = self.process_manager.start_app_window('Text Editor', files=[file_path], locale='C')
        self.assertProperty(gedit_win, is_focused=True)

        scope = self.unity.dash.reveal_file_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)

        # wait for "Recent" category
        category = self.wait_for_category(scope, _("Recent"))

        # wait for some results
        self.assertThat(lambda: len(category.get_results()), Eventually(GreaterThan(0), timeout=20))
        results = category.get_results()

        result = results[0]
        # result.preview handles finding xy co-ords and right mouse-click
        result.preview()
        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(True)))

        self.keyboard.press_and_release("Escape")

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_music_scope_preview_open_close(self):
        """Right-clicking on a music scope result must show its
        preview.
        """
        scope = self.unity.dash.reveal_music_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)

        category = scope.get_category_by_name("Songs")
        # Incase there was no music ever played we skip the test instead
        # of failing.
        if category is None or not category.is_visible:
            self.skipTest("This scope is probably empty")

        # wait for some results
        self.assertThat(lambda: len(category.get_results()), Eventually(GreaterThan(0), timeout=20))
        results = category.get_results()

        result = results[0]
        # result.preview handles finding xy co-ords and right mouse-click
        result.preview()
        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(True)))

        self.keyboard.press_and_release("Escape")

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_video_scope_preview_open_close(self):
        """Right-clicking on a video scope result must show its
        preview.
        """
        gettext.install("unity-scope-video", unicode=True)

        def get_category(scope):
            category = scope.get_category_by_name(_("Recently Viewed"))
            # If there was no video played on this system this category is expected
            # to be empty, if its empty we check if the 'Online' category have any
            # contents, if not then we skip the test.
            if category is None or not category.is_visible:
                category = scope.get_category_by_name("Online")
                if category is None or not category.is_visible:
                    self.skipTest("This scope is probably empty")
            return category

        scope = self.unity.dash.reveal_video_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)

        # get category. might not be any.
        category = get_category(scope)

        # wait for some results
        self.assertThat(lambda: len(category.get_results()), Eventually(GreaterThan(0), timeout=20))
        results = category.get_results()

        result = results[0]
        # result.preview handles finding xy co-ords and right mouse-click
        result.preview()
        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(True)))

        self.keyboard.press_and_release("Escape")

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_preview_key(self):
        """Pressing menu key on a selected dash result must show
        its preview.
        """
        gettext.install("unity-scope-applications", unicode=True)
        scope = self.unity.dash.reveal_application_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)

        # wait for "More suggestions" category
        category = self.wait_for_category(scope, _("More suggestions"))
        
        # wait for results
        self.assertThat(lambda: len(category.get_results()), Eventually(GreaterThan(0), timeout=20))
        results = category.get_results()

        result = results[0]
        # result.preview_key() handles finding xy co-ords and key press
        result.preview_key()
        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(True)))


class PreviewNavigateTests(DashTestCase):
    """Tests that mouse navigation works with previews."""

    def setUp(self):
        super(PreviewNavigateTests, self).setUp()
        gettext.install("unity-scope-applications", unicode=True)

        scope = self.unity.dash.reveal_application_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)

        # wait for "More suggestions" category
        category = self.wait_for_category(scope, _("More suggestions"))

        # wait for results (we need 4 results to perorm the multi-navigation tests)
        self.assertThat(lambda: len(category.get_results()), Eventually(GreaterThan(4), timeout=20))

        results = category.get_results()
        result = results[2] # 2 so we can navigate left
        result.preview()
        self.assertThat(self.unity.dash.view.preview_displaying, Eventually(Equals(True)))
        self.assertThat(self.unity.dash.view.get_preview_container, Eventually(NotEquals(None)))

        self.preview_container = self.unity.dash.view.get_preview_container()

    def test_navigate_left(self):
        """Tests that left navigation works with previews."""

        # wait until preview has finished animating
        self.assertThat(self.preview_container.animating, Eventually(Equals(False)))
        self.assertThat(self.preview_container.navigate_left_enabled, Eventually(Equals(True)))

        old_navigation_complete_count = self.preview_container.navigation_complete_count
        old_relative_nav_index = self.preview_container.relative_nav_index

        self.preview_container.navigate_left(1)

        self.assertThat(self.preview_container.navigation_complete_count, Eventually(Equals(old_navigation_complete_count+1)))
        self.assertThat(self.preview_container.relative_nav_index, Eventually(Equals(old_relative_nav_index-1)))

        # should be one more on the left
        self.assertThat(self.preview_container.navigate_left_enabled, Eventually(Equals(True)))
        # if we've navigated left, there should be at least one preview available on right.
        self.assertThat(self.preview_container.navigate_right_enabled, Eventually(Equals(True)))

        # Test close preview after navigate
        self.keyboard.press_and_release("Escape")
        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_navigate_left_multi(self):
        """Tests that multiple left navigation works with previews."""

        # wait until preview has finished animating
        self.assertThat(self.preview_container.animating, Eventually(Equals(False)))
        self.assertThat(self.preview_container.navigate_left_enabled, Eventually(Equals(True)))

        old_navigation_complete_count = self.preview_container.navigation_complete_count
        old_relative_nav_index = self.preview_container.relative_nav_index

        self.preview_container.navigate_left(2)

        self.assertThat(self.preview_container.navigation_complete_count, Eventually(Equals(old_navigation_complete_count+2)))
        self.assertThat(self.preview_container.relative_nav_index, Eventually(Equals(old_relative_nav_index-2)))

        # shouldnt be any previews on left.
        self.assertThat(self.preview_container.navigate_left_enabled, Eventually(Equals(False)))
        # if we've navigated left, there should be at least one preview available on right.
        self.assertThat(self.preview_container.navigate_right_enabled, Eventually(Equals(True)))


    def test_navigate_right(self):
        """Tests that right navigation works with previews."""

        # wait until preview has finished animating
        self.assertThat(self.preview_container.animating, Eventually(Equals(False)))
        self.assertThat(self.preview_container.navigate_left_enabled, Eventually(Equals(True)))

        old_navigation_complete_count = self.preview_container.navigation_complete_count
        old_relative_nav_index = self.preview_container.relative_nav_index

        self.preview_container.navigate_right(1)

        self.assertThat(self.preview_container.navigation_complete_count, Eventually(Equals(old_navigation_complete_count+1)))
        self.assertThat(self.preview_container.relative_nav_index, Eventually(Equals(old_relative_nav_index+1)))

        # should be at least one more on the left
        self.assertThat(self.preview_container.navigate_left_enabled, Eventually(Equals(True)))
        # if we've navigated right, there should be at least one preview available on left.
        self.assertThat(self.preview_container.navigate_right_enabled, Eventually(Equals(True)))

        # Test close preview after navigate
        self.keyboard.press_and_release("Escape")
        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_navigate_right_multi(self):
        """Tests that multiple right navigation works with previews."""

        # wait until preview has finished animating
        self.assertThat(self.preview_container.animating, Eventually(Equals(False)))
        self.assertThat(self.preview_container.navigate_left_enabled, Eventually(Equals(True)))

        old_navigation_complete_count = self.preview_container.navigation_complete_count
        old_relative_nav_index = self.preview_container.relative_nav_index

        self.preview_container.navigate_right(2)

        self.assertThat(self.preview_container.navigation_complete_count, Eventually(Equals(old_navigation_complete_count+2)))
        self.assertThat(self.preview_container.relative_nav_index, Eventually(Equals(old_relative_nav_index+2)))

        # if we've navigated right, there should be at least one preview available on left.
        self.assertThat(self.preview_container.navigate_left_enabled, Eventually(Equals(True)))

    def test_preview_refocus_close(self):
        """Clicking on a preview element must not lose keyboard focus."""
        cover_art = self.get_current_preview().cover_art[0]

        # click the cover-art (this will set focus)
        tx = cover_art.x + (cover_art.width / 2)
        ty = cover_art.y + (cover_art.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click()

        self.keyboard.press_and_release("Escape")

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_overlay_text(self):
        """Fallback overlay text is internationalized, should always be valid."""
        cover_art = self.get_current_preview().cover_art[0]
        self.assertThat(cover_art.overlay_text,
                        Eventually(Equals("No Image Available")))


class PreviewClickCancelTests(DashTestCase):
    """Tests that the preview closes when left, middle, and right clicking in the preview"""

    def setUp(self):
        super(PreviewClickCancelTests, self).setUp()
        gettext.install("unity-scope-applications")
        scope = self.unity.dash.reveal_application_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)
        # Only testing an application preview for this test.
        self.keyboard.type("Software Updater")

        # wait for "Installed" category
        category = self.wait_for_category(scope, _("Installed"))
        
        # wait for results
        self.assertThat(lambda: len(category.get_results()), Eventually(GreaterThan(0), timeout=20))
        results = category.get_results()

        result = results[0]
        result.preview()
        self.assertThat(self.unity.dash.view.preview_displaying, Eventually(Equals(True)))

        self.preview_container = self.unity.dash.view.get_preview_container()

    def test_left_click_on_preview_icon_cancel_preview(self):
        """Left click on preview icon must close preview."""
        icon = self.get_current_preview().icon[0]
        self.assertThat(icon, NotEquals(None))

        tx = icon.x + icon.width
        ty = icon.y + (icon.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=1)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_middle_click_on_preview_icon_cancel_preview(self):
        """Middle click on preview icon must close preview."""
        icon = self.get_current_preview().icon[0]
        self.assertThat(icon, NotEquals(None))

        tx = icon.x + icon.width
        ty = icon.y + (icon.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=2)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_right_click_on_preview_icon_cancel_preview(self):
        """Right click on preview icon must close preview."""
        icon = self.get_current_preview().icon[0]
        self.assertThat(icon, NotEquals(None))

        tx = icon.x + icon.width
        ty = icon.y + (icon.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=3)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_left_click_on_preview_image_cancel_preview(self):
        """Left click on preview image must cancel the preview."""
        cover_art = self.get_current_preview().cover_art[0]
        self.assertThat(cover_art, NotEquals(None))

        tx = cover_art.x + (cover_art.width / 2)
        ty = cover_art.y + (cover_art.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=1)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_middle_click_on_preview_image_cancel_preview(self):
        """Middle click on preview image must cancel the preview."""
        cover_art = self.get_current_preview().cover_art[0]
        self.assertThat(cover_art, NotEquals(None))

        tx = cover_art.x + (cover_art.width / 2)
        ty = cover_art.y + (cover_art.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=2)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_right_click_on_preview_image_cancel_preview(self):
        """Right click on preview image must cancel the preview."""
        cover_art = self.get_current_preview().cover_art[0]
        self.assertThat(cover_art, NotEquals(None))

        tx = cover_art.x + (cover_art.width / 2)
        ty = cover_art.y + (cover_art.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=3)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_left_click_on_preview_text_cancel_preview(self):
        """Left click on some preview text must cancel the preview."""
        text = self.get_current_preview().text_boxes[0]
        self.assertThat(text, NotEquals(None))

        tx = text.x + (text.width / 2)
        ty = text.y + (text.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=1)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_middle_click_on_preview_text_cancel_preview(self):
        """Middle click on some preview text must cancel the preview."""
        text = self.get_current_preview().text_boxes[0]
        self.assertThat(text, NotEquals(None))

        tx = text.x + (text.width / 2)
        ty = text.y + (text.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=2)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_right_click_on_preview_text_cancel_preview(self):
        """Right click on some preview text must cancel the preview."""
        text = self.get_current_preview().text_boxes[0]
        self.assertThat(text, NotEquals(None))

        tx = text.x + (text.width / 2)
        ty = text.y + (text.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=3)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_left_click_on_preview_ratings_widget_cancel_preview(self):
        """Left click on the ratings widget must cancel the preview."""
        ratings_widget = self.get_current_preview().ratings_widget[0]
        self.assertThat(ratings_widget, NotEquals(None))

        tx = ratings_widget.x + (ratings_widget.width / 2)
        ty = ratings_widget.y + (ratings_widget.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=1)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_middle_click_on_preview_ratings_widget_cancel_preview(self):
        """Middle click on the ratings widget must cancel the preview."""
        ratings_widget = self.get_current_preview().ratings_widget[0]
        self.assertThat(ratings_widget, NotEquals(None))

        tx = ratings_widget.x + (ratings_widget.width / 2)
        ty = ratings_widget.y + (ratings_widget.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=2)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_right_click_on_preview_ratings_widget_cancel_preview(self):
        """Right click on the ratings widget must cancel the preview."""
        ratings_widget = self.get_current_preview().ratings_widget[0]
        self.assertThat(ratings_widget, NotEquals(None))

        tx = ratings_widget.x + (ratings_widget.width / 2)
        ty = ratings_widget.y + (ratings_widget.height / 2)
        self.mouse.move(tx, ty)
        self.mouse.click(button=3)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_left_click_on_preview_info_hint_cancel_preview(self):
        """Left click on the info hint must cancel the preview."""
        info_hint = self.get_current_preview().info_hint_widget[0]
        self.assertThat(info_hint, NotEquals(None))

        tx = info_hint.x + (info_hint.width / 2)
        ty = info_hint.y + (info_hint.height / 8)
        self.mouse.move(tx, ty)
        self.mouse.click(button=1)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_middle_click_on_preview_info_hint_cancel_preview(self):
        """Middle click on the info hint must cancel the preview."""
        info_hint = self.get_current_preview().info_hint_widget[0]
        self.assertThat(info_hint, NotEquals(None))

        tx = info_hint.x + (info_hint.width / 2)
        ty = info_hint.y + (info_hint.height / 8)
        self.mouse.move(tx, ty)
        self.mouse.click(button=2)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))

    def test_right_click_on_preview_info_hint_cancel_preview(self):
        """Right click on the info hint must cancel the preview."""
        info_hint = self.get_current_preview().info_hint_widget[0]
        self.assertThat(info_hint, NotEquals(None))

        tx = info_hint.x + (info_hint.width / 2)
        ty = info_hint.y + (info_hint.height / 8)
        self.mouse.move(tx, ty)
        self.mouse.click(button=3)

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(False)))


class DashDBusIfaceTests(DashTestCase):
    """Test the Unity dash DBus interface."""

    def test_dash_hide(self):
        """Ensure we can hide the dash via HideDash() dbus method."""
        self.unity.dash.ensure_visible()
        self.unity.dash.hide_dash_via_dbus()
        self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))
        self.unity.dash.ensure_hidden()


class DashCrossMonitorsTests(DashTestCase):
    """Multi-monitor dash tests."""

    def setUp(self):
        super(DashCrossMonitorsTests, self).setUp()
        if self.display.get_num_screens() < 2:
            self.skipTest("This test requires more than 1 monitor.")

    def test_dash_stays_on_same_monitor(self):
        """If the dash is opened, then the mouse is moved to another monitor and
        the keyboard is used. The Dash must not move to that monitor.
        """
        current_monitor = self.unity.dash.ideal_monitor

        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)

        move_mouse_to_screen((current_monitor + 1) % self.display.get_num_screens())
        self.keyboard.type("abc")

        self.assertThat(self.unity.dash.ideal_monitor, Eventually(Equals(current_monitor)))

    def test_dash_close_on_cross_monitor_click(self):
        """Dash must close when clicking on a window in a different screen."""

        self.addCleanup(self.unity.dash.ensure_hidden)

        for monitor in range(self.display.get_num_screens()-1):
            move_mouse_to_screen(monitor)
            self.unity.dash.ensure_visible()

            move_mouse_to_screen(monitor+1)
            sleep(.5)
            self.mouse.click()

            self.assertThat(self.unity.dash.visible, Eventually(Equals(False)))
