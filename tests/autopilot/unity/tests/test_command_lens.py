# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import Equals, NotEquals
from time import sleep

from unity.tests import UnityTestCase

import gettext

class CommandScopeSearchTests(UnityTestCase):
    """Test the command scope search bahavior."""

    def setUp(self):
        super(CommandScopeSearchTests, self).setUp()
        gettext.install("unity-scope-applications")

    def tearDown(self):
        self.unity.dash.ensure_hidden()
        super(CommandScopeSearchTests, self).tearDown()

    def wait_for_category(self, scope, group):
        """Method to wait for a specific category"""
        get_scope_fn = lambda: scope.get_category_by_name(group)
        self.assertThat(get_scope_fn, Eventually(NotEquals(None)))
        return get_scope_fn()

    def test_no_results(self):
        """An empty string should get no results."""
        self.unity.dash.reveal_command_scope()
        command_scope = self.unity.dash.get_current_scope()

        if self.unity.dash.search_string != "":
            self.keyboard.press_and_release("Delete")

        self.assertThat(self.unity.dash.search_string, Eventually(Equals("")))

        results_category = self.wait_for_category(command_scope, _("Results"))
        self.assertThat(results_category.is_visible, Eventually(Equals(False)))

    def test_results_category_appears(self):
        """Results category must appear when there are some results."""
        self.unity.dash.reveal_command_scope()
        command_scope = self.unity.dash.get_current_scope()
        # lots of apps start with 'a'...
        self.keyboard.type("a")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("a")))

        results_category = self.wait_for_category(command_scope, _("Results"))
        self.assertThat(results_category.is_visible, Eventually(Equals(True)))

    def test_result_category_actually_contains_results(self):
        """With a search string of 'a', the results category must contain some results."""
        self.unity.dash.reveal_command_scope()
        command_scope = self.unity.dash.get_current_scope()
        # lots of apps start with 'a'...
        self.keyboard.type("a")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("a")))

        results_category = self.wait_for_category(command_scope, _("Results"))
        results = results_category.get_results()
        self.assertTrue(results)

    def test_run_before_refresh(self):
        """Hitting enter before view has updated results must run the correct command."""
        if self.app_is_running("Text Editor"):
            self.close_all_app("Text Editor")
            sleep(1)

        self.unity.dash.reveal_command_scope()
        self.keyboard.type("g")
        sleep(1)
        self.keyboard.type("edit", 0.1)
        self.keyboard.press_and_release("Enter", 0.1)
        self.addCleanup(self.close_all_app,  "Text Editor")
        app_found = self.bamf.wait_until_application_is_running("gedit.desktop", 5)
        self.assertTrue(app_found)

    def test_ctrl_tab_switching(self):
        """Pressing Ctrl+Tab after launching command scope must switch to Home scope."""
        self.unity.dash.reveal_command_scope()
        self.keybinding("dash/lens/next")
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals("home.scope")))

    def test_ctrl_shift_tab_switching(self):
        """Pressing Ctrl+Shift+Tab after launching command scope must switch to Video scope."""
        self.unity.dash.reveal_command_scope()
        self.keybinding("dash/lens/prev")
        self.assertThat(self.unity.dash.active_scope, Eventually(Equals("photos.scope")))
