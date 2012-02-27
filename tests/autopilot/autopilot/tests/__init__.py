"""
Autopilot tests for Unity.
"""

from compizconfig import Setting, Plugin
from autopilot.globals import global_context
from testtools import TestCase
from testscenarios import TestWithScenarios
from testtools.content import text_content
import time
import logging
from StringIO import StringIO

from autopilot.emulators.X11 import Keyboard, Mouse


class AutopilotTestCase(TestWithScenarios, TestCase):
    """Wrapper around testtools.TestCase that takes care of some cleaning."""

    def setUp(self):

        class MyFormatter(logging.Formatter):

            def formatTime(self, record, datefmt=None):
                ct = self.converter(record.created)
                if datefmt:
                    s = time.strftime(datefmt, ct)
                else:
                    t = time.strftime("%H:%M:%S", ct)
                    s = "%s.%03d" % (t, record.msecs)
                return s

        self._log_buffer = StringIO()
        root_logger = logging.getLogger()
        root_logger.setLevel(logging.DEBUG)
        handler = logging.StreamHandler(stream=self._log_buffer)
        log_format = "%(asctime)s %(levelname)s %(pathname)s:%(lineno)d - %(message)s"
        handler.setFormatter(MyFormatter(log_format))
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

    def set_unity_option(self, option_name, option_value):
        """Set an option in the unity compiz plugin options.

        The value will be set for the current test only.

        """
        old_value = self._set_unity_option(option_name, option_value)
        self.addCleanup(self._set_unity_option, option_name, old_value)

    def _set_unity_option(self, option_name, option_value):
        plugin = Plugin(global_context, "unityshell")
        setting = Setting(plugin, option_name)
        old_value = setting.Value
        setting.Value = option_value
        global_context.Write()
        return old_value
