# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Autopilot test case class for Unity-specific tests."""

from __future__ import absolute_import

from autopilot.testcase import AutopilotTestCase
from dbus import DBusException
import os
from tempfile import mktemp
from testtools.content import text_content
from testtools.matchers import Equals

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



class UnityTestCase(AutopilotTestCase):
    """Unity test case base class, with improvments specific to Unity tests."""

    def __init__(self, *args):
        super(UnityTestCase, self).__init__(*args)

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

