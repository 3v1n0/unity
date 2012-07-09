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
from testtools.matchers import Equals, GreaterThan

from unity.tests.launcher import LauncherTestCase

logger = logging.getLogger(__name__)


class LauncherKeyNavTests(LauncherTestCase):
    """Test the launcher key navigation"""

    def start_keynav_with_cleanup_cancel(self):
        """Start keynav mode safely.

        This adds a cleanup action that cancels keynav mode at the end of the
        test if it's still running (but does nothing otherwise).

        """
        self.launcher_instance.key_nav_start()
        self.addCleanup(self.safe_quit_keynav)

    def safe_quit_keynav(self):
        """Quit the keynav mode if it's engaged."""
        if self.launcher.key_nav_is_active:
            self.launcher_instance.key_nav_cancel()

    def test_launcher_keynav_initiate(self):
        """Tests we can initiate keyboard navigation on the launcher."""
        self.start_keynav_with_cleanup_cancel()
        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(True)))
        self.assertThat(self.launcher.key_nav_is_grabbed, Eventually(Equals(True)))

    def test_launcher_keynav_cancel(self):
        """Test that we can exit keynav mode."""
        self.launcher_instance.key_nav_start()
        self.launcher_instance.key_nav_cancel()
        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(False)))
        self.assertThat(self.launcher.key_nav_is_grabbed, Eventually(Equals(False)))

    def test_launcher_keynav_cancel_resume_focus(self):
        """Test that ending the launcher keynav resume the focus."""
        calc = self.start_app("Calculator")
        self.assertTrue(calc.is_active)

        self.start_keynav_with_cleanup_cancel()
        self.assertFalse(calc.is_active)

        self.launcher_instance.key_nav_cancel()
        self.assertTrue(calc.is_active)

    def test_launcher_keynav_starts_at_index_zero(self):
        """Test keynav mode starts at index 0."""
        self.start_keynav_with_cleanup_cancel()
        self.assertThat(self.launcher.key_nav_selection, Eventually(Equals(0)))

    def test_launcher_keynav_forward(self):
        """Must be able to move forwards while in keynav mode."""
        self.start_keynav_with_cleanup_cancel()
        self.launcher_instance.key_nav_next()
        # The launcher model has hidden items, so the keynav indexes do not
        # increase by 1 each time. This test was failing because the 2nd icon
        # had an index of 2, not 1 as expected. The best we can do here is to
        # make sure that the index has increased. This opens us to the
        # possibility that the launcher really is skipping forward more than one
        # icon at a time, but we can't do much about that.
        self.assertThat(self.launcher.key_nav_selection, Eventually(GreaterThan(0)))

    def test_launcher_keynav_prev_works(self):
        """Must be able to move backwards while in keynav mode."""
        self.start_keynav_with_cleanup_cancel()
        self.launcher_instance.key_nav_next()
        self.assertThat(self.launcher.key_nav_selection, Eventually(GreaterThan(0)))
        self.launcher_instance.key_nav_prev()
        self.assertThat(self.launcher.key_nav_selection, Eventually(Equals(0)))

    def test_launcher_keynav_cycling_forward(self):
        """Launcher keynav must loop through icons when cycling forwards"""
        self.start_keynav_with_cleanup_cancel()
        prev_icon = 0
        for icon in range(1, self.launcher.model.num_launcher_icons()):
            self.launcher_instance.key_nav_next()
            # FIXME We can't directly check for selection/icon number equalty
            # since the launcher model also contains "hidden" icons that aren't
            # shown, so the selection index can increment by more than 1.
            self.assertThat(self.launcher.key_nav_selection, Eventually(GreaterThan(prev_icon)))
            prev_icon = self.launcher.key_nav_selection

        self.launcher_instance.key_nav_next()
        self.assertThat(self.launcher.key_nav_selection, Eventually(Equals(0)))

    def test_launcher_keynav_cycling_backward(self):
        """Launcher keynav must loop through icons when cycling backwards"""
        self.start_keynav_with_cleanup_cancel()
        self.launcher_instance.key_nav_prev()
        # FIXME We can't directly check for self.launcher.num_launcher_icons - 1
        self.assertThat(self.launcher.key_nav_selection, Eventually(GreaterThan(1)))

    def test_launcher_keynav_can_open_and_close_quicklist(self):
        """Tests that we can open and close a quicklist from keynav mode."""
        self.start_keynav_with_cleanup_cancel()
        self.launcher_instance.key_nav_next()
        self.launcher_instance.key_nav_enter_quicklist()
        self.assertThat(self.launcher_instance.quicklist_open, Eventually(Equals(True)))
        self.launcher_instance.key_nav_exit_quicklist()
        self.assertThat(self.launcher_instance.quicklist_open, Eventually(Equals(False)))
        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(True)))
        self.assertThat(self.launcher.key_nav_is_grabbed, Eventually(Equals(True)))

    def test_launcher_keynav_mode_toggles(self):
        """Tests that keynav mode toggles with Alt+F1."""
        # was initiated in setup.
        self.start_keynav_with_cleanup_cancel()
        self.keybinding("launcher/keynav")
        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(False)))

    def test_launcher_keynav_activate_keep_focus(self):
        """Activating a running launcher icon must focus it."""
        calc = self.start_app("Calculator")
        mahjongg = self.start_app("Mahjongg")
        self.assertTrue(mahjongg.is_active)
        self.assertFalse(calc.is_active)

        self.start_keynav_with_cleanup_cancel()

        self.launcher_instance.keyboard_select_icon(tooltip_text=calc.name)
        self.launcher_instance.key_nav_activate()

        self.assertTrue(calc.is_active)
        self.assertFalse(mahjongg.is_active)

    def test_launcher_keynav_expo_focus(self):
        """When entering expo mode from KeyNav the Desktop must get focus."""
        self.start_keynav_with_cleanup_cancel()

        self.launcher_instance.keyboard_select_icon(tooltip_text="Workspace Switcher")
        self.launcher_instance.key_nav_activate()
        self.addCleanup(self.keybinding, "expo/cancel")

        self.assertThat(self.panels.get_active_panel().title, Eventually(Equals("Ubuntu Desktop")))

    def test_launcher_keynav_expo_exit_on_esc(self):
        """Esc should quit expo when entering it from KeyNav."""
        self.start_keynav_with_cleanup_cancel()

        self.launcher_instance.keyboard_select_icon(tooltip_text="Workspace Switcher")
        self.launcher_instance.key_nav_activate()

        self.keyboard.press_and_release("Escape")
        self.assertThat(self.window_manager.expo_active, Eventually(Equals(False)))

    def test_launcher_keynav_alt_tab_quits(self):
        """Tests that alt+tab exits keynav mode."""
        self.start_keynav_with_cleanup_cancel()

        self.keybinding("switcher/reveal_normal")
        self.addCleanup(self.switcher.terminate)
        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(False)))

    def test_launcher_keynav_alt_grave_quits(self):
        """Tests that alt+` exits keynav mode."""
        self.start_keynav_with_cleanup_cancel()
        # Can't use switcher emulat here since the switcher won't appear.
        self.keybinding("switcher/reveal_details")
        self.assertThat(self.launcher.key_nav_is_active, Eventually(Equals(False)))

    def test_launcher_keynav_cancel_doesnt_activate_icon(self):
        """This tests when canceling keynav the current icon doesnt activate."""
        self.start_keynav_with_cleanup_cancel()
        self.keyboard.press_and_release("Escape")
        self.assertThat(self.dash.visible, Eventually(Equals(False)))

    def test_alt_f1_closes_dash(self):
        """Pressing Alt+F1 when the Dash is open must close the Dash and start keynav."""
        self.dash.ensure_visible()

        self.start_keynav_with_cleanup_cancel()

        self.assertThat(self.dash.visible, Equals(False))
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))

    def test_alt_f1_closes_hud(self):
        """Pressing Alt+F1 when the HUD is open must close the HUD and start keynav."""
        self.hud.ensure_visible()

        self.start_keynav_with_cleanup_cancel()

        self.assertThat(self.hud.visible, Equals(False))
        self.assertThat(self.launcher.key_nav_is_active, Equals(True))
