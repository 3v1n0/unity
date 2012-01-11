from testtools import TestCase
from time import sleep

from autopilot.emulators.unity import Dash
from autopilot.emulators.X11 import Keyboard
from autopilot.glibrunner import GlibRunner


class DashTests(TestCase):
    """
    Test the unity Dash.
    """
    run_test_with = GlibRunner
    
    def setUp(self):
        super(DashTests, self).setUp()
        self.dash = Dash()

    def tearDown(self):
        super(DashTests, self).tearDown()
        self.dash.ensure_hidden()

    def test_dash_reveal(self):
        """
        Ensure we can show and hide the dash.
        """
        self.dash.ensure_hidden()

        self.assertFalse(self.dash.get_is_visible())
        self.dash.toggle_reveal()
        self.assertTrue(self.dash.get_is_visible())
        self.dash.toggle_reveal()
        self.assertFalse(self.dash.get_is_visible())

    def test_dash_keyboard_focus(self):
        """
        Dash must put keyboard focus on the search bar at all times.
        """
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
        """
        Application lense must reveal when Super+a is pressed.
        """
        self.dash.ensure_hidden()
        self.dash.reveal_application_lens()
        self.assertEqual(self.dash.get_current_lens(), u'applications.lens')

    def test_music_lens_shortcut(self):
        """
        Music lense must reveal when Super+w is pressed.
        """
        self.dash.ensure_hidden()
        self.dash.reveal_music_lens()
        self.assertEqual(self.dash.get_current_lens(), u'music.lens')
    
    def test_file_lens_shortcut(self):
        """
        File lense must reveal when Super+f is pressed.
        """
        self.dash.ensure_hidden()
        self.dash.reveal_file_lens()
        self.assertEqual(self.dash.get_current_lens(), u'files.lens')


        
