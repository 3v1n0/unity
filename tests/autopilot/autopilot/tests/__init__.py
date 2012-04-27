"""
Autopilot tests for Unity.
"""


from compizconfig import Setting, Plugin
from dbus import DBusException
import logging
import os
from StringIO import StringIO
from subprocess import call, check_output, Popen, PIPE, STDOUT
from tempfile import mktemp
from testscenarios import TestWithScenarios
from testtools import TestCase
from testtools.content import text_content
from testtools.matchers import Equals
import time

from autopilot.emulators.bamf import Bamf
from autopilot.emulators.unity import (
    set_log_severity,
    start_log_to_file,
    reset_logging,
    )
from autopilot.emulators.unity.dash import Dash
from autopilot.emulators.unity.hud import Hud
from autopilot.emulators.unity.launcher import LauncherController
from autopilot.emulators.unity.panel import PanelController
from autopilot.emulators.unity.switcher import Switcher
from autopilot.emulators.unity.window_manager import WindowManager
from autopilot.emulators.unity.workspace import WorkspaceManager
from autopilot.emulators.X11 import ScreenGeometry, Keyboard, Mouse, reset_display
from autopilot.glibrunner import GlibRunner
from autopilot.globals import (global_context,
    video_recording_enabled,
    video_record_directory,
    )
from autopilot.keybindings import KeybindingsHelper


logger = logging.getLogger(__name__)


try:
    from testscenarios.scenarios import multiply_scenarios
except ImportError:
    from itertools import product
    def multiply_scenarios(*scenarios):
        """Multiply two or more iterables of scenarios.

        It is safe to pass scenario generators or iterators.

        :returns: A list of compound scenarios: the cross-product of all
            scenarios, with the names concatenated and the parameters
            merged together.
        """
        result = []
        scenario_lists = map(list, scenarios)
        for combination in product(*scenario_lists):
            names, parameters = zip(*combination)
            scenario_name = ','.join(names)
            scenario_parameters = {}
            for parameter in parameters:
                scenario_parameters.update(parameter)
            result.append((scenario_name, scenario_parameters))
        return result


class LoggedTestCase(TestWithScenarios, TestCase):
    """Initialize the logging for the test case."""

    def setUp(self):
        self._setUpTestLogging()
        self._setUpUnityLogging()
        # The reason that the super setup is done here is due to making sure
        # that the logging is properly set up prior to calling it.
        super(LoggedTestCase, self).setUp()

    def _setUpTestLogging(self):
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
        log_format = "%(asctime)s %(levelname)s %(module)s:%(lineno)d - %(message)s"
        handler.setFormatter(MyFormatter(log_format))
        root_logger.addHandler(handler)
        #Tear down logging in a cleanUp handler, so it's done after all other
        # tearDown() calls and cleanup handlers.
        self.addCleanup(self._tearDownLogging)

    def _tearDownLogging(self):
        logger = logging.getLogger()
        for handler in logger.handlers:
            handler.flush()
            self._log_buffer.seek(0)
            self.addDetail('test-log', text_content(self._log_buffer.getvalue()))
            logger.removeHandler(handler)
        # Calling del to remove the handler and flush the buffer.  We are
        # abusing the log handlers here a little.
        del self._log_buffer

    def _setUpUnityLogging(self):
        self._unity_log_file_name = mktemp(prefix=self.shortDescription())
        start_log_to_file(self._unity_log_file_name)
        self.addCleanup(self._tearDownUnityLogging)

    def _tearDownUnityLogging(self):
        # If unity dies, our dbus interface has gone, and reset_logging will fail
        # but we still want our log, so we ignore any errors.
        try:
            reset_logging()
        except DBusException:
            pass
        with open(self._unity_log_file_name) as unity_log:
            self.addDetail('unity-log', text_content(unity_log.read()))
        os.remove(self._unity_log_file_name)
        self._unity_log_file_name = ""

    def set_unity_log_level(self, component, level):
        """Set the unity log level for 'component' to 'level'.

        Valid levels are: TRACE, DEBUG, INFO, WARNING and ERROR.

        Components are dotted unity component names. The empty string specifies
        the root logging component.
        """
        set_log_severity(component, level)


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
        args = self._get_capture_command_line()
        self._capture_file = self._get_capture_output_file()
        self._ensure_directory_exists_but_not_file(self._capture_file)
        args.append(self._capture_file)
        logger.debug("Starting: %r", args)
        self._capture_process = Popen(args, stdout=PIPE, stderr=STDOUT)

    def _stop_video_capture(self):
        """Stop the video capture. If the test failed, save the resulting file."""

        if self._test_passed:
            # We use kill here because we don't want the recording app to start
            # encoding the video file (since we're removing it anyway.)
            self._capture_process.kill()
            self._capture_process.wait()
        else:
            self._capture_process.terminate()
            self._capture_process.wait()
            if self._capture_process.returncode != 0:
                self.addDetail('video capture log', text_content(self._capture_process.stdout.read()))
        self._capture_process = None

    def _get_capture_command_line(self):
        return [self._recording_app] + self._recording_opts

    def _get_capture_output_file(self):
        return os.path.join(video_record_directory, '%s.ogv' % (self.shortDescription()))

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
        'Remmina' : {
            'desktop-file': 'remmina.desktop',
            'process-name': 'remmina',
            },
        'System Settings' : {
            'desktop-file': 'gnome-control-center.desktop',
            'process-name': 'gnome-control-center',
            },
        'Text Editor' : {
            'desktop-file': 'gedit.desktop',
            'process-name': 'gedit',
            },
        }

    def setUp(self):
        super(AutopilotTestCase, self).setUp()
        self.bamf = Bamf()
        self.keyboard = Keyboard()
        self.mouse = Mouse()
        self.dash = Dash()
        self.hud = Hud()
        self.launcher = self._get_launcher_controller()
        self.panels = self._get_panel_controller()
        self.switcher = Switcher()
        self.window_manager = self._get_window_manager()
        self.workspace = WorkspaceManager()
        self.screen_geo = ScreenGeometry()
        self.addCleanup(self.workspace.switch_to, self.workspace.current_workspace)
        self.addCleanup(Keyboard.cleanup)
        self.addCleanup(Mouse.cleanup)

    def start_app(self, app_name, files=[], locale=None):
        """Start one of the known apps, and kill it on tear down.

        If files is specified, start the application with the specified files.
        If locale is specified, the locale will be set when the application is launched.

        The method returns the BamfApplication instance.

        """
        if locale:
            os.putenv("LC_ALL", locale)
            self.addCleanup(os.unsetenv, "LC_ALL")
            logger.info("Starting application '%s' with files %r in locale %s", app_name, files, locale)
        else:
            logger.info("Starting application '%s' with files %r", app_name, files)

        app = self.KNOWN_APPS[app_name]
        self.bamf.launch_application(app['desktop-file'], files)
        apps = self.bamf.get_running_applications_by_desktop_file(app['desktop-file'])
        self.addCleanup(call, "kill `pidof %s`" % (app['process-name']), shell=True)
        self.assertThat(len(apps), Equals(1))
        return apps[0]

    def close_all_app(self, app_name):
        """Close all instances of the app_name."""
        app = self.KNOWN_APPS[app_name]
        self.addCleanup(call, "kill `pidof %s`" % (app['process-name']), shell=True)
        super(LoggedTestCase, self).tearDown()

    def get_app_instances(self, app_name):
        """Get BamfApplication instances for app_name."""
        desktop_file = self.KNOWN_APPS[app_name]['desktop-file']
        return self.bamf.get_running_applications_by_desktop_file(desktop_file)

    def app_is_running(self, app_name):
        """Returns true if an instance of the application is running."""
        apps = self.get_app_instances(app_name)
        return len(apps) > 0

    def call_gsettings_cmd(self, command, schema, *args):
        """Set a desktop wide gsettings option

        Using the gsettings command because there's a bug with importing
        from gobject introspection and pygtk2 simultaneously, and the Xlib
        keyboard layout bits are very unweildy. This seems like the best
        solution, even a little bit brutish.
        """
        cmd = ['gsettings', command, schema] + list(args)
        # strip to remove the trailing \n.
        ret = check_output(cmd).strip()
        time.sleep(5)
        reset_display()
        return ret

    def set_unity_option(self, option_name, option_value):
        """Set an option in the unity compiz plugin options.

        The value will be set for the current test only.

        """
        self.set_compiz_option("unityshell", option_name, option_value)

    def set_compiz_option(self, plugin_name, setting_name, setting_value):
        """Set setting `setting_name` in compiz plugin `plugin_name` to value `setting_value`
        for one test only.
        """
        old_value = self._set_compiz_option(plugin_name, setting_name, setting_value)
        self.addCleanup(self._set_compiz_option, plugin_name, setting_name, old_value)
        # Allow unity time to respond to the new setting.
        time.sleep(0.5)

    def _set_compiz_option(self, plugin_name, option_name, option_value):
        logger.info("Setting compiz option '%s' in plugin '%s' to %r",
            option_name, plugin_name, option_value)
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

    def _get_panel_controller(self):
        controllers = PanelController.get_all_instances()
        self.assertThat(len(controllers), Equals(1))
        return controllers[0]

    def _get_window_manager(self):
        managers = WindowManager.get_all_instances()
        self.assertThat(len(managers), Equals(1))
        return managers[0]

    def assertVisibleWindowStack(self, stack_start):
        """Check that the visible window stack starts with the windows passed in.

        The start_stack is an iterable of BamfWindow objects.
        Minimised windows are skipped.

        """
        stack = [win for win in self.bamf.get_open_windows() if not win.is_hidden]
        for pos, win in enumerate(stack_start):
            self.assertThat(stack[pos].x_id, Equals(win.x_id),
                            "%r at %d does not equal %r" % (stack[pos], pos, win))
