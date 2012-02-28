# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from subprocess import call
from testtools.matchers import Equals, NotEquals, Contains, Not
from time import sleep

from autopilot.emulators.bamf import Bamf
from autopilot.emulators.unity.switcher import Switcher
from autopilot.glibrunner import GlibRunner
from autopilot.tests import AutopilotTestCase


class SwitcherTests(AutopilotTestCase):
    """Test the switcher."""
    run_test_with = GlibRunner

    def set_timeout_setting(self, value):
        self.set_unity_option("alt_tab_timeout", value)

    def setUp(self):
        super(SwitcherTests, self).setUp()

        self.start_app('Character Map')
        self.start_app('Calculator')
        self.start_app('Mahjongg')

        self.switcher = Switcher()

    def tearDown(self):
        super(SwitcherTests, self).tearDown()
        sleep(1)

    def test_switcher_move_next(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        sleep(.2)

        start = self.switcher.get_selection_index()
        self.switcher.next_icon()
        sleep(.2)

        end = self.switcher.get_selection_index()
        self.switcher.terminate()

        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start + 1))

    def test_switcher_move_prev(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.switcher.initiate()
        sleep(.2)

        start = self.switcher.get_selection_index()
        self.switcher.previous_icon()
        sleep(.2)

        end = self.switcher.get_selection_index()
        self.switcher.terminate()

        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start - 1))
        self.set_timeout_setting(True)


class SwitcherWorkspaceTests(AutopilotTestCase):
    """Test Switcher behavior with respect to multiple workspaces."""

    def setUp(self):
        super(SwitcherWorkspaceTests, self).setUp()
        self.switcher = Switcher()

    def test_switcher_shows_current_workspace_only(self):
        """Switcher must show apps from the current workspace only."""
        self.close_all_app('Calculator')
        self.close_all_app('Character Map')
        # If the default number of workspaces ever changes, this test will
        # continue to work:
        #self.set_compiz_option('core','hsize', 2)
        #self.set_compiz_option('core','vsize', 2)
        self.start_app("Calculator")
        sleep(1)
        self.workspace.switch_to(2)
        self.start_app("Character Map")
        sleep(1)

        self.switcher.initiate()
        sleep(1)
        icon_names = [i.tooltip_text for i in self.switcher.get_switcher_icons()]
        self.switcher.terminate()
        self.assertThat(icon_names, Contains("Character Map"))
        self.assertThat(icon_names, Not(Contains("Calculator")))


