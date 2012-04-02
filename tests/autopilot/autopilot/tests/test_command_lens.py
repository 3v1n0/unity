# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools.matchers import Equals
from time import sleep

from autopilot.emulators.unity.dash import Dash
from autopilot.emulators.X11 import Keyboard
from autopilot.tests import AutopilotTestCase


class CommandLensSearchTests(AutopilotTestCase):
    """Test the command lense search bahavior."""

    def setUp(self):
        self.dash = Dash()
        super(CommandLensSearchTests, self).setUp()

    def tearDown(self):
        self.dash.ensure_hidden()
        super(CommandLensSearchTests, self).tearDown()

    def test_no_results(self):
        """An empty string should get no results."""
        kb = Keyboard()
        self.dash.reveal_command_lens()
        command_lens = self.dash.get_current_lens()

        if self.dash.get_searchbar().search_string != "":
            kb.press_and_release("Delete")

        sleep(1)
        results_category = command_lens.get_category_by_name("Results")
        self.assertFalse(results_category.is_visible)

    def test_results_category_appears(self):
        """Results category must appear when there are some results."""
        kb = Keyboard()
        self.dash.reveal_command_lens()
        command_lens = self.dash.get_current_lens()
        # lots of apps start with 'a'...
        kb.type("a")
        sleep(1)
        results_category = command_lens.get_category_by_name("Results")
        self.assertTrue(results_category.is_visible)

    def test_result_category_actually_contains_results(self):
        """With a search string of 'a', the results category must contain some results."""
        kb = Keyboard()
        self.dash.reveal_command_lens()
        command_lens = self.dash.get_current_lens()
        # lots of apps start with 'a'...
        kb.type("a")
        sleep(1)
        results_category = command_lens.get_category_by_name("Results")
        results = results_category.get_results()
        self.assertTrue(results)

    def test_run_before_refresh(self):
        """Hitting enter before view has updated results must run the correct command."""
        if self.app_is_running("Text Editor"):
            self.close_all_app("Text Editor")
            sleep(1)

        kb = Keyboard()
        self.dash.reveal_command_lens()
        kb.type("g")
        sleep(1)
        kb.type("edit", 0.1)
        kb.press_and_release("Enter", 0.1)
        self.addCleanup(self.close_all_app,  "Text Editor")
        app_found = self.bamf.wait_until_application_is_running("gedit.desktop", 5)
        self.assertTrue(app_found)

    def test_ctrl_tab_switching(self):
        """Pressing Ctrl+Tab after launching command lens must switch to Home lens."""
        self.dash.reveal_command_lens()
        self.keybinding("dash/lens/next")
        sleep(1)
        self.assertThat(self.dash.active_lens, Equals("home.lens"))

    def test_ctrl_shift_tab_switching(self):
        """Pressing Ctrl+Shift+Tab after launching command lens must switch to Video lens."""
        self.dash.reveal_command_lens()
        self.keybinding("dash/lens/prev")
        sleep(1)
        self.assertThat(self.dash.active_lens, Equals("video.lens"))
