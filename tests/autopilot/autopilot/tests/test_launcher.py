from testtools.matchers import Equals
from testtools.matchers import LessThan

from autopilot.tests import AutopilotTestCase
from autopilot.emulators.unity import Launcher
from autopilot.glibrunner import GlibRunner

from time import sleep


class LauncherTests(AutopilotTestCase):
    """Test the launcher."""
    run_test_with = GlibRunner

    def setUp(self):
        super(LauncherTests, self).setUp()
        self.server = Launcher()

    def test_launcher_switcher_ungrabbed(self):
        """Tests basic key nav integration without keyboard grabs"""
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
        """Tests basic key nav integration via keyboard grab"""
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

    def test_launcher_switcher_quicklist_interaction(self):
        """Tests that the key nav opens and closes quicklists properly and regrabs afterwards"""
        self.server.move_mouse_to_right_of_launcher(0)
        sleep(.5)

        self.server.grab_switcher()
        sleep(.5)

        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(True))

        self.server.switcher_next()
        sleep(.5)

        self.server.switcher_enter_quicklist()
        sleep(.5)
        self.assertThat(self.server.quicklist_open(0), Equals(True))
        self.server.switcher_exit_quicklist()
        sleep(.5)

        self.assertThat(self.server.quicklist_open(0), Equals(False))
        self.assertThat(self.server.key_nav_is_active(), Equals(True))
        self.assertThat(self.server.key_nav_is_grabbed(), Equals(True))

        self.server.end_switcher(True)
        sleep(.5)
        self.assertThat(self.server.key_nav_is_active(), Equals(False))

    def test_reveal_on_mouse_to_edge(self):
        """Tests reeal of launchers by mouse pressure"""
        num_launchers = self.server.num_launchers()

        for x in range(num_launchers):
                self.server.move_mouse_to_right_of_launcher(x)
                self.server.reveal_launcher(x)
                self.assertThat(self.server.is_showing(x), Equals(True))
