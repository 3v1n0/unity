# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from time import sleep

from autopilot.emulators.unity import Dash
from autopilot.emulators.X11 import Keyboard
from autopilot.emulators.X11 import Mouse
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

    def test_category_header_keynav(self):
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
          
          



