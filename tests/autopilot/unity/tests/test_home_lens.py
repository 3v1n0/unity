# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Michal Hruby
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from time import sleep

from unity.tests import UnityTestCase


class HomeLensSearchTests(UnityTestCase):
    """Test the command lense search bahavior."""

    def setUp(self):
        super(HomeLensSearchTests, self).setUp()

    def tearDown(self):
        self.unity.dash.ensure_hidden()
        super(HomeLensSearchTests, self).tearDown()

    def test_quick_run_app(self):
        """Hitting enter runs an application even though a search might not have fully
        finished yet.

        """
        if self.process_manager.app_is_running("Text Editor"):
            self.process_manager.close_all_app("Text Editor")
            sleep(1)

        kb = self.keyboard
        self.unity.dash.ensure_visible()
        kb.type("g")
        self.assertThat(self.unity.dash.search_string, Eventually(Equals("g")))
        kb.type("edit", 0.1)
        kb.press_and_release("Enter", 0.1)
        self.addCleanup(self.process_manager.close_all_app,  "Text Editor")
        app_found = self.process_manager.wait_until_application_is_running("gedit.desktop", 5)
        self.assertTrue(app_found)
