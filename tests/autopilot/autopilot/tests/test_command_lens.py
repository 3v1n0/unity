# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from autopilot.emulators.unity.dash import Dash
from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.tests import AutopilotTestCase
from autopilot.glibrunner import GlibRunner


class CommandLensSearchTests(AutopilotTestCase):
    """Test the command lense search bahavior."""
    run_test_with = GlibRunner

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
        kb = Keyboard()
        self.dash.reveal_command_lens()
        command_lens = self.dash.get_current_lens()
        # lots of apps start with 'a'...
        kb.type("a")
        sleep(1)
        results_category = command_lens.get_category_by_name("Results")
        results = results_category.get_results()
        self.assertTrue(results)
