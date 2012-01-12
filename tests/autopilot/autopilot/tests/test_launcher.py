from testtools import TestCase

from autopilot.emulators.unity import Launcher
from autopilot.glibrunner import GlibRunner

class LauncherTests(TestCase):
    run_test_with = GlibRunner

    def setUp(self):
        super(LauncherTests, self).setUp()
        self.server = Launcher()

    def test_reveal_on_mouse_to_edge(self):
        self.server.move_mouse_outside_of_boundry()
        self.server.move_mouse_to_reveal_pos()
        self.assertTrue(self.server.is_showing())
        self.server.move_mouse_outside_of_boundry()
        self.assertFalse(self.server.is_showing())
