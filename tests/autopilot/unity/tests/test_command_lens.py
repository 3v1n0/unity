# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from time import sleep

from unity.tests import UnityTestCase

import gettext

class CommandScopeSearchTests(UnityTestCase):
    """Test the command scopee search bahavior."""

    def setUp(self):
        super(CommandScopeSearchTests, self).setUp()
        gettext.install("unity-lens-applications")

    def tearDown(self):
        self.dash.ensure_hidden()
        super(CommandScopeSearchTests, self).tearDown()

    def test_no_results(self):
        """An empty string should get no results."""
        self.dash.reveal_command_scope()
        command_scope = self.dash.get_current_scope()

        if self.dash.search_string != "":
            self.keyboard.press_and_release("Delete")

        self.assertThat(self.dash.search_string, Eventually(Equals("")))
        results_category = command_scope.get_category_by_name(_("Results"))
        self.assertThat(results_category.is_visible, Eventually(Equals(False)))

    def test_results_category_appears(self):
        """Results category must appear when there are some results."""
        self.dash.reveal_command_scope()
        command_scope = self.dash.get_current_scope()
        # lots of apps start with 'a'...
        self.keyboard.type("a")
        self.assertThat(self.dash.search_string, Eventually(Equals("a")))
        results_category = command_scope.get_category_by_name(_("Results"))
        self.assertThat(results_category.is_visible, Eventually(Equals(True)))

    def test_result_category_actually_contains_results(self):
        """With a search string of 'a', the results category must contain some results."""
        self.dash.reveal_command_scope()
        command_scope = self.dash.get_current_scope()
        # lots of apps start with 'a'...
        self.keyboard.type("a")
        self.assertThat(self.dash.search_string, Eventually(Equals("a")))
        results_category = command_scope.get_category_by_name(_("Results"))
        results = results_category.get_results()
        self.assertTrue(results)

    def test_run_before_refresh(self):
        """Hitting enter before view has updated results must run the correct command."""
        if self.app_is_running("Text Editor"):
            self.close_all_app("Text Editor")
            sleep(1)

        self.dash.reveal_command_scope()
        self.keyboard.type("g")
        sleep(1)
        self.keyboard.type("edit", 0.1)
        self.keyboard.press_and_release("Enter", 0.1)
        self.addCleanup(self.close_all_app,  "Text Editor")
        app_found = self.bamf.wait_until_application_is_running("gedit.desktop", 5)
        self.assertTrue(app_found)

    def test_ctrl_tab_switching(self):
        """Pressing Ctrl+Tab after launching command scope must switch to Home scope."""
        self.dash.reveal_command_scope()
        self.keybinding("dash/scope/next")
        self.assertThat(self.dash.active_scope, Eventually(Equals("home.scope")))

    def test_ctrl_shift_tab_switching(self):
        """Pressing Ctrl+Shift+Tab after launching command scope must switch to Video scope."""
        self.dash.reveal_command_scope()
        self.keybinding("dash/scope/prev")
        self.assertThat(self.dash.active_scope, Eventually(Equals("video.scope")))
