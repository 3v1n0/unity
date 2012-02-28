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

    def test_switcher_starts_in_normal_mode(self):
        """Switcher must start in normal (i.e.- not details) mode."""
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        self.assertThat(self.switcher.get_is_in_details_mode(), Equals(False))


class SwitcherDetailsModeTests(AutopilotTestCase):
    """Tests for the details mode of the switcher.

    Tests for initiation with both grave (`) and Down arrow.

    """

    scenarios = [
        ('initiate_with_grave', {'initiate_keycode': '`'}),
        ('initiate_with_down', {'initiate_keycode': 'Down'}),
    ]

    def setUp(self):
        self.bamf = Bamf()
        self.bamf.launch_application("gucharmap.desktop")
        self.switcher = Switcher()
        super(SwitcherDetailsModeTests, self).setUp()

    def tearDown(self):
        call(["killall", "gucharmap"])
        super(SwitcherDetailsModeTests, self).tearDown()
        sleep(1)

    def test_can_start_details_mode(self):
        """Must be able to initiate details mode using selected scenario keycode."""
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        self.keyboard.press_and_release(self.initiate_keycode)
        self.assertThat(self.switcher.get_is_in_details_mode(), Equals(True))

    def test_tab_from_last_detail_works(self):
        """Pressing tab while showing last switcher item in details mode
        must select first item in the model in non-details mode.

        """
        self.switcher.initiate()
        self.addCleanup(self.switcher.terminate)
        while self.switcher.get_selection_index() < self.switcher.get_model_size() -1:
            self.switcher.next_icon()
        self.keyboard.press_and_release(self.initiate_keycode)
        sleep(0.5)
        self.switcher.next_icon()
        self.assertThat(self.switcher.get_selection_index(), Equals(0))


class SwitcherWorkspaceTests(AutopilotTestCase):
    """Test Switcher behavior with respect to multiple workspaces."""

    def setUp(self):
        super(SwitcherWorkspaceTests, self).setUp()
        self.switcher = Switcher()

    def test_switcher_shows_current_workspace_only(self):
        """Switcher must show apps from the current workspace only."""
        self.close_all_app('Calculator')
        self.close_all_app('Character Map')

        self.workspace.switch_to(1)
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

    def test_switcher_all_mode_shows_all_apps(self):
        """Test switcher 'show_all' mode shows apps from all workspaces."""
        self.close_all_app('Calculator')
        self.close_all_app('Character Map')

        self.workspace.switch_to(1)
        self.start_app("Calculator")
        sleep(1)
        self.workspace.switch_to(2)
        self.start_app("Character Map")
        sleep(1)

        self.switcher.initiate_all_mode()
        sleep(1)
        icon_names = [i.tooltip_text for i in self.switcher.get_switcher_icons()]
        self.switcher.terminate()
        self.assertThat(icon_names, Contains("Character Map"))
        self.assertThat(icon_names, Contains("Calculator"))

    def test_switcher_can_switch_to_minimised_window(self):
        """Switcher must be able to switch to a minimised window when there's
        another instance of the same application on a different workspace."""
        # disable automatic gridding of the switcher after a timeout, since it makes
        # it harder to write the tests.
        self.set_unity_option("alt_tab_timeout", False)
        self.close_all_app("Calculator")
        self.close_all_app("Mahjongg")

        self.workspace.switch_to(1)
        self.start_app("Mahjongg")

        self.workspace.switch_to(3)
        self.start_app("Mahjongg")
        sleep(1)
        # TODO: When the 'minimise window' keybinding works we can replace this hack:
        self.keyboard.press_and_release("Alt+Space")
        sleep(1)
        self.keyboard.press_and_release("n")
        self.start_app("Calculator")
        sleep(1)

        #get calculator windows - there should be only one:
        mahjongg_apps = self.bamf.get_running_applications_by_title("Mahjongg")
        self.assertThat(len(mahjongg_apps), Equals(1))
        wins = mahjongg_apps[0].get_windows()
        self.assertThat(len(wins), Equals(0))
        self.assertThat(wins[0].is_focused, Equals(True))

