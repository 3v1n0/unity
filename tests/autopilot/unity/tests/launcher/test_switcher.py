# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards,
#          Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually

import logging
from testtools.matchers import Equals, NotEquals, GreaterThan
from time import sleep

from unity.tests.launcher import LauncherTestCase


logger = logging.getLogger(__name__)


class LauncherSwitcherTests(LauncherTestCase):
    """ Tests the functionality of the launcher's switcher capability"""

    def start_switcher_with_cleanup_cancel(self):
        """Start switcher mode safely.

        This adds a cleanup action that cancels keynav mode at the end of the
        test if it's still running (but does nothing otherwise).

        """
        self.launcher_instance.switcher_start()
        self.addCleanup(self.safe_quit_switcher)

    def safe_quit_switcher(self):
        """Quit the keynav mode if it's engaged."""
        if self.unity.launcher.key_nav_is_active:
            self.launcher_instance.switcher_cancel()

    def test_launcher_switcher_cancel(self):
        """Test that ending the launcher switcher actually works."""
        self.launcher_instance.switcher_start()
        self.launcher_instance.switcher_cancel()
        self.assertThat(self.unity.launcher.key_nav_is_active, Eventually(Equals(False)))

    def test_launcher_switcher_cancel_resume_focus(self):
        """Test that ending the launcher switcher resume the focus."""
        self.close_all_app("Calculator")
        calc = self.process_manager.start_app("Calculator")
        self.assertTrue(calc.is_active)

        self.start_switcher_with_cleanup_cancel()
        sleep(.5)
        self.assertFalse(calc.is_active)

        self.launcher_instance.switcher_cancel()
        sleep(.5)
        self.assertTrue(calc.is_active)

    def test_launcher_switcher_starts_at_index_zero(self):
        """Test that starting the Launcher switcher puts the keyboard focus on item 0."""
        self.start_switcher_with_cleanup_cancel()

        self.assertThat(self.unity.launcher.key_nav_is_active, Eventually(Equals(True)))
        self.assertThat(self.unity.launcher.key_nav_is_grabbed, Eventually(Equals(False)))
        self.assertThat(self.unity.launcher.key_nav_selection, Eventually(Equals(0)))

    def test_launcher_switcher_next(self):
        """Moving to the next launcher item while switcher is activated must work."""
        self.start_switcher_with_cleanup_cancel()
        self.launcher_instance.switcher_next()
        # The launcher model has hidden items, so the keynav indexes do not
        # increase by 1 each time. This test was failing because the 2nd icon
        # had an index of 2, not 1 as expected. The best we can do here is to
        # make sure that the index has increased. This opens us to the
        # possibility that the launcher really is skipping forward more than one
        # icon at a time, but we can't do much about that.
        self.assertThat(self.unity.launcher.key_nav_selection, Eventually(GreaterThan(0)))

    def test_launcher_switcher_prev(self):
        """Moving to the previous launcher item while switcher is activated must work."""
        self.start_switcher_with_cleanup_cancel()
        self.launcher_instance.switcher_prev()
        self.assertThat(self.unity.launcher.key_nav_selection, Eventually(NotEquals(0)))

    def test_launcher_switcher_down(self):
        """Pressing the down arrow key while switcher is activated must work."""
        self.start_switcher_with_cleanup_cancel()
        self.launcher_instance.switcher_down()
        # The launcher model has hidden items, so the keynav indexes do not
        # increase by 1 each time. This test was failing because the 2nd icon
        # had an index of 2, not 1 as expected. The best we can do here is to
        # make sure that the index has increased. This opens us to the
        # possibility that the launcher really is skipping forward more than one
        # icon at a time, but we can't do much about that.
        self.assertThat(self.unity.launcher.key_nav_selection, Eventually(GreaterThan(0)))

    def test_launcher_switcher_up(self):
        """Pressing the up arrow key while switcher is activated must work."""
        self.start_switcher_with_cleanup_cancel()
        self.launcher_instance.switcher_up()
        self.assertThat(self.unity.launcher.key_nav_selection, Eventually(NotEquals(0)))

    def test_launcher_switcher_next_doesnt_show_shortcuts(self):
        """Moving forward in launcher switcher must not show launcher shortcuts."""
        self.start_switcher_with_cleanup_cancel()
        self.launcher_instance.switcher_next()
        # sleep so that the shortcut timeout could be triggered
        sleep(2)
        self.assertThat(self.launcher_instance.shortcuts_shown, Eventually(Equals(False)))

    def test_launcher_switcher_prev_doesnt_show_shortcuts(self):
        """Moving backward in launcher switcher must not show launcher shortcuts."""
        self.start_switcher_with_cleanup_cancel()
        self.launcher_instance.switcher_prev()
        # sleep so that the shortcut timeout could be triggered
        sleep(2)
        self.assertThat(self.launcher_instance.shortcuts_shown, Eventually(Equals(False)))

    def test_launcher_switcher_cycling_forward(self):
        """Launcher Switcher must loop through icons when cycling forwards"""
        self.start_switcher_with_cleanup_cancel()
        prev_icon = 0
        num_icons = self.unity.launcher.model.num_launcher_icons()
        logger.info("This launcher has %d icons", num_icons)
        for icon in range(1, num_icons):
            self.launcher_instance.switcher_next()
            # FIXME We can't directly check for selection/icon number equalty
            # since the launcher model also contains "hidden" icons that aren't
            # shown, so the selection index can increment by more than 1.
            self.assertThat(self.unity.launcher.key_nav_selection, Eventually(GreaterThan(prev_icon)))
            prev_icon = self.unity.launcher.key_nav_selection

        self.launcher_instance.switcher_next()
        self.assertThat(self.unity.launcher.key_nav_selection, Eventually(Equals(0)))

    def test_launcher_switcher_cycling_backward(self):
        """Launcher Switcher must loop through icons when cycling backwards"""
        self.start_switcher_with_cleanup_cancel()
        self.launcher_instance.switcher_prev()
        # FIXME We can't directly check for self.unity.launcher.num_launcher_icons - 1
        self.assertThat(self.unity.launcher.key_nav_selection, Eventually(GreaterThan(1)))

    def test_launcher_switcher_activate_keep_focus(self):
        """Activating a running launcher icon should focus the application."""
        calc = self.process_manager.start_app("Calculator")
        mahjongg = self.process_manager.start_app("Mahjongg")
        self.assertTrue(mahjongg.is_active)
        self.assertFalse(calc.is_active)

        self.start_switcher_with_cleanup_cancel()

        self.launcher_instance.keyboard_select_icon(tooltip_text=calc.name)
        self.launcher_instance.switcher_activate()

        self.assertThat(lambda: calc.is_active, Eventually(Equals(True)))
        self.assertThat(lambda: mahjongg.is_active, Eventually(Equals(False)))

    def test_launcher_switcher_using_shorcuts(self):
        """Using some other shortcut while switcher is active must cancel switcher."""
        self.start_switcher_with_cleanup_cancel()
        self.keyboard.press_and_release("s")
        sleep(.25)
        self.keyboard.press_and_release("Escape")
        sleep(.25)
        self.assertThat(self.unity.launcher.key_nav_is_active, Eventually(Equals(False)))
