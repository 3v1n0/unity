"""
Autopilot tests for Unity.
"""

from testtools import TestCase
from testscenarios import TestWithScenarios
from testtools.content import text_content
import logging
from StringIO import StringIO

from autopilot.emulators.X11 import Keyboard, Mouse


class AutopilotTestCase(TestWithScenarios, TestCase):
    """Wrapper around testtools.TestCase that takes care of some cleaning."""

    def setUp(self):
        self._log_buffer = StringIO()
        root_logger = logging.getLogger()
        root_logger.setLevel(logging.DEBUG)
        handler = logging.StreamHandler(stream=self._log_buffer)
        root_logger.addHandler(handler)

        super(AutopilotTestCase, self).setUp()

    def tearDown(self):
        Keyboard.cleanup()
        Mouse.cleanup()

        logger = logging.getLogger()
        for handler in logger.handlers:
            handler.flush()
            self._log_buffer.seek(0)
            self.addDetail('test-log', text_content(self._log_buffer.getvalue()))
            logger.removeHandler(handler)
        #self._log_buffer.close()
        del self._log_buffer
        super(AutopilotTestCase, self).tearDown()
