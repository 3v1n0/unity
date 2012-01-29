from testtools import TestCase
from testtools.matchers import Equals
from testtools.matchers import LessThan

from autopilot.emulators.unity import Launcher
from autopilot.glibrunner import GlibRunner

from time import sleep

class LauncherTests(TestCase):
    run_test_with = GlibRunner

    def setUp(self):
        super(LauncherTests, self).setUp()
        self.server = Launcher()

    def test_launcher_switcher_ungrabbed(self):
        sleep(.5)
        
        self.server.start_switcher()
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(False))
        self.assertThat(self.server.key_nav_selection(), Equals(0))
        
        self.server.switcher_next()
        sleep(.5)
        self.assertThat(0, LessThan(self.server.key_nav_selection()))
        
        self.server.switcher_prev()
        sleep(.5)
        self.assertThat(self.server.key_nav_selection(), Equals(0))
        
        self.server.end_switcher(True)
        sleep(.5)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))
    
    def test_launcher_switcher_grabbed(self):
        sleep(.5)
        
        self.server.grab_switcher()
        sleep(.5)
        
        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(True))
        self.assertThat(self.server.key_nav_selection(), Equals(0))
        
        self.server.switcher_next()
        sleep(.5)
        self.assertThat(0, LessThan(self.server.key_nav_selection()))
        
        self.server.switcher_prev()
        sleep(.5)
        self.assertThat(self.server.key_nav_selection(), Equals(0))
        
        self.server.end_switcher(True)
        sleep(.5)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))

    """This test cannot be fixed until Autopilot can simulate XFixes Barrier Events"""
    #def test_reveal_on_mouse_to_edge(self):
    #    self.server.move_mouse_outside_of_boundry()
    #    self.server.move_mouse_to_reveal_pos()
    #    self.assertThat(self.server.is_showing(), Equals(True))
