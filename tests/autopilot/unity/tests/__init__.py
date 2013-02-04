# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Autopilot test case class for Unity-specific tests."""

from __future__ import absolute_import


from codecs import open
from autopilot.matchers import Eventually
from autopilot.testcase import AutopilotTestCase
from dbus import DBusException
from logging import getLogger
import os
import sys
from tempfile import mktemp
from time import sleep
try:
    import testapp
    import json
    HAVE_TESTAPP=True
except ImportError:
    HAVE_TESTAPP=False
import tempfile
from testtools.content import text_content
from testtools.matchers import Equals
from unittest.case import SkipTest

from unity.emulators import ensure_unity_is_running
from unity.emulators.screen import Screen
from unity.emulators.dash import Dash
from unity.emulators.hud import Hud
from unity.emulators.launcher import LauncherController
from unity.emulators.panel import PanelController
from unity.emulators.switcher import Switcher
from unity.emulators.window_manager import WindowManager
from unity.emulators.workspace import WorkspaceManager
from unity.emulators.unity import (
    set_log_severity,
    start_log_to_file,
    reset_logging,
    )


log = getLogger(__name__)


class UnityTestCase(AutopilotTestCase):
    """Unity test case base class, with improvments specific to Unity tests."""

    def __init__(self, *args):
        super(UnityTestCase, self).__init__(*args)

    def setUp(self):
        super(UnityTestCase, self).setUp()
        try:
            ensure_unity_is_running()
        except RuntimeError:
            log.error("Unity doesn't appear to be running, exiting.")
            sys.exit(1)

        self._setUpUnityLogging()
        self._initial_workspace_num = self.workspace.current_workspace
        self.addCleanup(self.check_test_behavior)
        #
        # Setting this here since the show desktop feature seems to be a bit
        # ropey. Once it's been proven to work reliably we can remove this line:
        self.set_unity_log_level("unity.wm.compiz", "DEBUG")

    def check_test_behavior(self):
        """Fail the test if it did something naughty.

        This includes leaving the dash or the hud open, changing the current
        workspace, or leaving the system in show_desktop mode.

        """
        well_behaved = True
        reasons = []
        log.info("Checking system state for badly behaving test...")

        # Have we switched workspace?
        if not self.well_behaved(self.workspace, current_workspace=self._initial_workspace_num):
            well_behaved = False
            reasons.append("The test changed the active workspace from %d to %d." \
                % (self._initial_workspace_num, self.workspace.current_workspace))
            log.warning("Test changed the active workspace, changing it back...")
            self.workspace.switch_to(self._initial_workspace_num)
        # Have we left the dash open?
        if not self.well_behaved(self.dash, visible=False):
            well_behaved = False
            reasons.append("The test left the dash open.")
            log.warning("Test left the dash open, closing it...")
            self.unity.dash.ensure_hidden()
        # ... or the hud?
        if not self.well_behaved(self.hud, visible=False):
            well_behaved = False
            reasons.append("The test left the hud open.")
            log.warning("Test left the hud open, closing it...")
            self.hud.ensure_hidden()
        # Are we in show desktop mode?
        if not self.well_behaved(self.window_manager, showdesktop_active=False):
            well_behaved = False
            reasons.append("The test left the system in show_desktop mode.")
            log.warning("Test left the system in show desktop mode, exiting it...")
            # It is not possible to leave show desktop mode if there are no
            # app windows. So, just open a window and perform the show
            # desktop action until the desired state is acheived, then close
            # the window. The showdesktop_active state will persist.
            #
            # In the event that this doesn't work, wait_for will throw an
            # exception.
            win = self.start_app_window('Calculator', locale='C')
            count = 1
            while self.window_manager.showdesktop_active:
                self.keybinding("window/show_desktop")
                sleep(count)
                count+=1
                if count > 10:
                    break
            win.close()
            self.window_manager.showdesktop_active.wait_for(False)
        for launcher in self.launcher.get_launchers():
            if not self.well_behaved(launcher, in_keynav_mode=False):
                well_behaved = False
                reasons.append("The test left the launcher keynav mode enabled.")
                log.warning("Test left the launcher in keynav mode, exiting it...")
                launcher.key_nav_cancel()
            if not self.well_behaved(launcher, in_switcher_mode=False):
                well_behaved = False
                reasons.append("The test left the launcher in switcher mode.")
                log.warning("Test left the launcher in switcher mode, exiting it...")
                launcher.switcher_cancel()
            if not self.well_behaved(launcher, quicklist_open=False):
                well_behaved = False
                reasons.append("The test left a quicklist open.")
                log.warning("The test left a quicklist open.")
                self.keyboard.press_and_release('Escape')

        if not well_behaved:
            self.fail("/n".join(reasons))
        else:
            log.info("Test was well behaved.")

    def well_behaved(self, object, **kwargs):
        try:
            self.assertProperty(object, **kwargs)
        except AssertionError:
            return False
        return True

    @property
    def screen(self):
        if not getattr(self, '__screen', None):
            self.__screen = self._get_screen()
        return self.__screen

    @property
    def dash(self):
        if not getattr(self, '__dash', None):
            self.__dash = Dash()
        return self.__dash

    @property
    def hud(self):
        if not getattr(self, '__hud', None):
            self.__hud = Hud();
        return self.__hud

    @property
    def launcher(self):
        if not getattr(self, '__launcher', None):
            self.__launcher = self._get_launcher_controller()
        return self.__launcher

    @property
    def panels(self):
        if not getattr(self, '__panels', None):
            self.__panels = self._get_panel_controller()
        return self.__panels

    @property
    def switcher(self):
        if not getattr(self, '__switcher', None):
            self.__switcher = Switcher()
        return self.__switcher

    @property
    def window_manager(self):
        if not getattr(self, '__window_manager', None):
            self.__window_manager = self._get_window_manager()
        return self.__window_manager

    @property
    def workspace(self):
        if not getattr(self, '__workspace', None):
            self.__workspace = WorkspaceManager()
        return self.__workspace

    def _get_screen(self):
        screens = Screen.get_all_instances()
        self.assertThat(len(screens), Equals(1))
        return screens[0]

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
        with open(self._unity_log_file_name, encoding='utf-8') as unity_log:
            self.addDetail('unity-log', text_content(unity_log.read()))
        os.remove(self._unity_log_file_name)
        self._unity_log_file_name = ""

    def set_unity_log_level(self, component, level):
        """Set the unity log level for 'component' to 'level'.

        Valid levels are: TRACE, DEBUG, INFO, WARNING and ERROR.

        Components are dotted unity component names. The empty string specifies
        the root logging component.
        """
        valid_levels = ('TRACE', 'DEBUG', 'INFO', 'WARN', 'WARNING', 'ERROR')
        if level not in valid_levels:
            raise ValueError("Log level '%s' must be one of: %r" % (level, valid_levels))
        set_log_severity(component, level)

    def assertNumberWinsIsEventually(self, app, num):
        """Asserts that 'app' eventually has 'num' wins. Waits up to 10 seconds."""

        self.assertThat(lambda: len(app.get_windows()), Eventually(Equals(num)))

    def launch_test_window(self, window_spec={}):
        """Launch a test window, for the duration of this test only.

        This uses the 'testapp' application, which is not part of the
        python-autopilot or unity-autopilot packages. To use this method, you
        must have python-testapp installed. If the python-testapp packge is not
        installed, this method will raise a SkipTest exception, causing the
        calling test to be silently skipped.

        window_spec is a list or dictionary that conforms to the testapp
        specification.

        """
        if not HAVE_TESTAPP:
            raise SkipTest("The python-testapp package is required to run this test.")

        if 'Test App' not in self.KNOWN_APPS:
            self.register_known_application(
                'Test App',
                'testapp.desktop',
                'testapp'
                )
        if window_spec:
            file_path = tempfile.mktemp()
            json.dump(window_spec, open(file_path, 'w'))
            self.addCleanup(os.remove, file_path)
            return self.start_app_window('Test App', [file_path])
        else:
            return self.start_app_window('Test App')
