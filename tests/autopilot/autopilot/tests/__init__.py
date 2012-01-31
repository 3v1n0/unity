"""
Autopilot tests for Unity.
"""

from testtools import TestCase
from autopilot.emulators.X11 import Keyboard, Mouse

class AutopilotTestCase(TestCase):
    """Wrapper around testtools.TestCase that takes care of some cleaning."""

    def tearDown(self):
        Keyboard.cleanup()
        Mouse.cleanup()
        super(AutopilotTestCase, self).tearDown()
