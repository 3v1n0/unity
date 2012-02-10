"""
Autopilot tests for Unity.
"""

from testtools import TestCase
from testtools.content import content_from_stream
import logging
from StringIO import StringIO

from autopilot.emulators.X11 import Keyboard, Mouse

class AutopilotTestCase(TestCase):
    """Wrapper around testtools.TestCase that takes care of some cleaning."""

    def setUp(self):
        self._log_buffer = StringIO()
        logging.basicConfig(level=logging.DEBUG, stream=self._log_buffer)
        super(AutopilotTestCase, self).setUp()

    def tearDown(self):
        super(AutopilotTestCase, self).tearDown()
        Keyboard.cleanup()
        Mouse.cleanup()
        logger = logging.getLogger()
        for handler in logger.handlers:
            handler.flush()
        self._log_buffer.seek(0)
        self.addDetail('test-log', content_from_stream(self._log_buffer))

