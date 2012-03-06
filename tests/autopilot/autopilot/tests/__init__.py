"""
Autopilot tests for Unity.
"""


from compizconfig import Setting, Plugin
import logging
import os
from StringIO import StringIO
from subprocess import call, Popen, PIPE, STDOUT
from testscenarios import TestWithScenarios
from testtools import TestCase
from testtools.content import text_content
from testtools.matchers import Equals
import time

from autopilot.emulators.bamf import Bamf
from autopilot.emulators.unity.launcher import LauncherController
from autopilot.emulators.unity.switcher import Switcher
from autopilot.emulators.unity.workspace import WorkspaceManager
from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.glibrunner import GlibRunner
from autopilot.globals import global_context, video_recording_enabled
from autopilot.keybindings import KeybindingsHelper


logger = logging.getLogger(__name__)


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
        #Tear down logging in a cleanUp handler, so it's done after all other
        # tearDown() calls and cleanup handlers.
        self.addCleanup(self.tearDownLogging)
        # The reason that the super setup is done here is due to making sure
        # that the logging is properly set up prior to calling it.
        super(LoggedTestCase, self).setUp()

    def tearDownLogging(self):
        logger = logging.getLogger()
        for handler in logger.handlers:
            handler.flush()
            self._log_buffer.seek(0)
            self.addDetail('test-log', text_content(self._log_buffer.getvalue()))
            logger.removeHandler(handler)
        # Calling del to remove the handler and flush the buffer.  We are
        # abusing the log handlers here a little.
        del self._log_buffer


class VideoCapturedTestCase(LoggedTestCase):
    """Video capture autopilot tests, saving the results if the test failed."""

    _recording_app = '/usr/bin/recordmydesktop'
    _recording_opts = ['--no-sound', '--no-frame', '-o',]

    def setUp(self):
        super(VideoCapturedTestCase, self).setUp()
        global video_recording_enabled
        if video_recording_enabled and not self._have_recording_app():
            video_recording_enabled = False
            logger.warning("Disabling video capture since '%s' is not present", self._recording_app)

        if video_recording_enabled:
            self._test_passed = True
            self.addOnException(self._on_test_failed)
            self.addCleanup(self._stop_video_capture)
            self._start_video_capture()

    def _have_recording_app(self):
        return os.path.exists(self._recording_app)

    def _start_video_capture(self):
        """Start capturing video."""
        args = self._get_capture_command_line()
        self._capture_file = self._get_capture_output_file()
        self._ensure_directory_exists_but_not_file(self._capture_file)
        args.append(self._capture_file)
        logger.debug("Starting: %r", args)
        self._capture_process = Popen(args, stdout=PIPE, stderr=STDOUT)

    def _stop_video_capture(self):
        """Stop the video capture. If the test failed, save the resulting file."""
        self._capture_process.terminate()
        self._capture_process.wait()

        if self._test_passed:
            os.remove(self._capture_file)
        else:
            self.addDetail('video capture log', text_content(self._capture_process.stdout.read()))
        self._capture_process = None


    def _get_capture_command_line(self):
        return [self._recording_app] + self._recording_opts

    def _get_capture_output_file(self):
        return '/tmp/autopilot/%s.ogv' % (self.shortDescription())

    def _ensure_directory_exists_but_not_file(self, file_path):
        dirpath = os.path.dirname(file_path)
        if not os.path.exists(dirpath):
            os.makedirs(dirpath)
        elif os.path.exists(file_path):
            logger.warning("Video capture file '%s' already exists, deleting.", file_path)
            os.remove(file_path)


    def _on_test_failed(self, ex_info):
        """Called when a test fails."""
        self._test_passed = False


class AutopilotTestCase(VideoCapturedTestCase, KeybindingsHelper):
    """Wrapper around testtools.TestCase that takes care of some cleaning."""

    run_test_with = GlibRunner

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
        self.mouse = Mouse()
        self.switcher = Switcher()
        self.workspace = WorkspaceManager()
        self.launcher = self._get_launcher_controller()
        self.addCleanup(self.workspace.switch_to, self.workspace.current_workspace)
        self.addCleanup(Keyboard.cleanup)
        self.addCleanup(Mouse.cleanup)

    def start_app(self, app_name):
        """Start one of the known apps, and kill it on tear down."""
        logger.info("Starting application '%s'", app_name)
        app = self.KNOWN_APPS[app_name]
        self.bamf.launch_application(app['desktop-file'])
        self.addCleanup(call, ["killall", app['process-name']])

    def close_all_app(self, app_name):
        """Close all instances of the app_name."""
        app = self.KNOWN_APPS[app_name]
        call(["killall", app['process-name']])
        super(LoggedTestCase, self).tearDown()

    def set_unity_option(self, option_name, option_value):
        """Set an option in the unity compiz plugin options.

        The value will be set for the current test only.

        """
        old_value = self._set_compiz_option("unityshell", option_name, option_value)
        self.addCleanup(self._set_compiz_option, "unityshell", option_name, old_value)

    def set_compiz_option(self, plugin_name, setting_name, setting_value):
        """Set setting `setting_name` in compiz plugin `plugin_name` to value `setting_value`
        for one test only.
        """
        old_value = self._set_compiz_option(plugin_name, setting_name, setting_value)
        self.addCleanup(self._set_compiz_option, plugin_name, setting_name, old_value)

    def _set_compiz_option(self, plugin_name, option_name, option_value):
        plugin = Plugin(global_context, plugin_name)
        setting = Setting(plugin, option_name)
        old_value = setting.Value
        setting.Value = option_value
        global_context.Write()
        return old_value

    def _get_launcher_controller(self):
        controllers = LauncherController.get_all_instances()
        self.assertThat(len(controllers), Equals(1))
        return controllers[0]
