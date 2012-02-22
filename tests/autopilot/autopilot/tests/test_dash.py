# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from autopilot.emulators.unity.dash import Dash
from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.tests import AutopilotTestCase
from autopilot.glibrunner import GlibRunner


class DashTests(AutopilotTestCase):
    """Test the unity Dash."""
    run_test_with = GlibRunner

    def setUp(self):
        super(DashTests, self).setUp()
        self.dash = Dash()

    def tearDown(self):
        super(DashTests, self).tearDown()
        self.dash.ensure_hidden()

    def test_dash_reveal(self):
        """Ensure we can show and hide the dash."""
        self.dash.ensure_hidden()

        self.assertFalse(self.dash.get_is_visible())
        self.dash.toggle_reveal()
        self.assertTrue(self.dash.get_is_visible())
        self.dash.toggle_reveal()
        self.assertFalse(self.dash.get_is_visible())

    def test_dash_keyboard_focus(self):
        """Dash must put keyboard focus on the search bar at all times."""
        self.dash.ensure_hidden()

        self.assertFalse(self.dash.get_is_visible())
        kb = Keyboard()
        self.dash.toggle_reveal()
        kb.type("Hello")
        sleep(1)
        self.assertEqual(self.dash.get_search_string(), u'Hello')
        self.dash.toggle_reveal()
        self.assertFalse(self.dash.get_is_visible())

    def test_application_lens_shortcut(self):
        """Application lense must reveal when Super+a is pressed."""
        self.dash.ensure_hidden()
        self.dash.reveal_application_lens()
        self.assertEqual(self.dash.get_current_lens(), u'applications.lens')

    def test_music_lens_shortcut(self):
        """Music lense must reveal when Super+w is pressed."""
        self.dash.ensure_hidden()
        self.dash.reveal_music_lens()
        self.assertEqual(self.dash.get_current_lens(), u'music.lens')

    def test_file_lens_shortcut(self):
        """File lense must reveal when Super+f is pressed."""
        self.dash.ensure_hidden()
        self.dash.reveal_file_lens()
        self.assertEqual(self.dash.get_current_lens(), u'files.lens')

    def test_command_lens_shortcut(self):
        """Run Command lens must reveat on alt+F2."""
        self.dash.ensure_hidden()
        self.dash.reveal_command_lens()
        self.assertEqual(self.dash.get_current_lens(), u'commands.lens')

    def test_lensbar_keyfocus(self):
        """Test that the lensbar keynavigation works well."""
        self.dash.ensure_hidden()
        self.dash.toggle_reveal()
        kb = Keyboard()

        # Make sure that the lens bar can get the focus
        for i in range(self.dash.get_num_rows()):
          kb.press_and_release("Down")
        self.assertIsNot(self.dash.get_focused_lens_icon(), '')

        # Make sure that left - right work well
        temp = self.dash.get_focused_lens_icon()
        kb.press_and_release("Right");
        self.assertIsNot(self.dash.get_focused_lens_icon(), temp)
        kb.press_and_release("Left")
        self.assertEqual(self.dash.get_focused_lens_icon(), temp)

        # Make sure that pressing 'Enter' we can change the lens...
        kb.press_and_release("Right");
        temp = self.dash.get_focused_lens_icon();
        kb.press_and_release("Enter");
        self.assertEqual(self.dash.get_current_lens(), temp)

        # ... the lens bar should lose the key focus
        self.assertEqual(self.dash.get_focused_lens_icon(), "")

    def test_category_header_keynav_autoscroll(self):
          """Test that the dash autoscroll when a category header gets
          the focus.
          """
          self.dash.ensure_hidden()
          self.dash.reveal_application_lens()

          kb = Keyboard()
          mouse = Mouse()

          # Expand the first category
          kb.press_and_release("Down")
          kb.press_and_release("Enter")
          category = self.dash.get_focused_category()

          # Get the geometry of that category header.
          x = category['header-x']
          y = category['header-y']
          w = category['header-width']
          h = category['header-height']

          # Manually scroll the dash.
          mouse.move(x+w+10, y+w+10, True)
          mouse.click(5)
          mouse.click(5)
          mouse.click(5)

          cached_y = y

          # Focus the search bar with the mouse
          x, y, w, h = self.dash.get_searchbar_geometry()
          mouse.move(x+100, y+h/2, True)
          mouse.click()
          sleep(2)

          # Then focus again the first category header
          kb.press_and_release("Down")
          kb.press_and_release("Enter")
          category = self.dash.get_focused_category()
          y = category['header-y']

          # Make sure the dash autoscroll
          self.assertTrue(abs(y-cached_y) < 30)

    def test_category_header_keynav(self):
          """ This test makes sure that:
          1. A category header can get the focus.
          2. A category header stays highlight when it loses the focus
             and mouse is close to it (but not inside).
          """
          self.dash.ensure_hidden()
          self.dash.reveal_application_lens()

          kb = Keyboard()
          mouse = Mouse()

          # Make sure that a category have the focus.
          kb.press_and_release("Down")
          category = self.dash.get_focused_category()
          self.assertIsNot(category, None)

          # Make sure that the category is highlighted.
          self.assertTrue(category['header-is-highlighted'], True)

          # Get the geometry of that category header.
          x = category['header-x']
          y = category['header-y']
          w = category['header-width']
          h = category['header-height']

          # Move the mouse close the view, and press down.
          mouse.move(x+w+10, y+h/2, True)
          sleep(1)
          kb.press_and_release("Down")

          category = self.dash.get_focused_category()
          self.assertEqual(category, None)

    def test_cltr_tab(self):
          """ This test makes sure that Ctlr + Tab works well."""
          self.dash.ensure_hidden()
          self.dash.toggle_reveal()

          kb = Keyboard()

          kb.press('Control')
          kb.press_and_release('Tab')
          kb.release('Control')

          self.assertEqual(self.dash.get_current_lens(), u'applications.lens')

          kb.press('Control')
          kb.press('Shift')
          kb.press_and_release('Tab')
          kb.release('Control')
          kb.release('Shift')

          self.assertEqual(self.dash.get_current_lens(), u'home.lens')

    def test_tab(self):
          """ This test makes sure that Tab works well."""
          self.dash.ensure_hidden()
          self.dash.reveal_application_lens()

          kb = Keyboard()

          for i in range(self.dash.get_num_categories('applications.lens')):
              kb.press_and_release('Tab')
              category = self.dash.get_focused_category()
              self.assertIsNot(category, None)

          kb.press_and_release('Tab')
          self.assertTrue(self.dash.filter_expander_has_focus())

          if not self.dash.get_showing_filters():
              kb.press_and_release('Enter')
              self.assertTrue(self.dash.get_showing_filters())

          last_focused_filter = None
          for i in range(self.dash.get_num_filters('applications.lens')):
              kb.press_and_release('Tab')
              new_focused_filter = self.dash.get_focused_filter()
              self.assertIsNot(new_focused_filter, last_focused_filter)

          kb.press_and_release('Tab')
          category = self.dash.get_focused_category()
          self.assertIsNot(category, None)
              


