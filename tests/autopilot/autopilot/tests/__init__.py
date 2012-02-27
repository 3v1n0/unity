"""
Autopilot tests for Unity.
"""

from subprocess import call
from testtools import TestCase
from testscenarios import TestWithScenarios
from testtools.content import text_content
import time
import logging
from StringIO import StringIO

from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.emulators.bamf import Bamf


class LoggedTestCase(TestWithScenarios, TestCase):
    """Initialize the logging for the test case."""

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
        # The reason that the super setup is done here is due to making sure
        # that the logging is properly set up prior to calling it.
        super(LoggedTestCase, self).setUp()

    def tearDown(self):
        logger = logging.getLogger()
        for handler in logger.handlers:
            handler.flush()
            self._log_buffer.seek(0)
            self.addDetail('test-log', text_content(self._log_buffer.getvalue()))
            logger.removeHandler(handler)
        # Calling del to remove the handler and flush the buffer.  We are
        # abusing the log handlers here a little.
        del self._log_buffer
        super(LoggedTestCase, self).tearDown()


class AutopilotTestCase(LoggedTestCase):
    """Wrapper around testtools.TestCase that takes care of some cleaning."""

    KNOWN_APPS = {
        'Character Map' : {
            'desktop-file': 'gucharmap.desktop',
            'process-name': 'gucharmap',
            },
        'Calculator' : {
            'desktop-file': 'gcalctool.desktop',
            'process-name': 'gcalctool',
            },
        'Mahjongg' : {
            'desktop-file': 'mahjongg.desktop',
            'process-name': 'mahjongg',
            },
        }

    def setUp(self):
        super(AutopilotTestCase, self).setUp()
        self.bamf = Bamf()
        self.keyboard = Keyboard()

    def tearDown(self):
        Keyboard.cleanup()
        Mouse.cleanup()
        super(AutopilotTestCase, self).tearDown()

    def start_app(self, app_name):
        """Start one of the known apps, and kill it on tear down."""
        app = self.KNOWN_APPS[app_name]
        self.bamf.launch_application(app['desktop-file'])
        self.addCleanup(call, ["killall", app['process-name']])

    def close_all_app(self, app_name):
        """Close all instances of the app_name."""
        app = self.KNOWN_APPS[app_name]
        call(["killall", app['process-name']])
