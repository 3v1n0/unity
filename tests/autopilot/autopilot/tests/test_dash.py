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


        
