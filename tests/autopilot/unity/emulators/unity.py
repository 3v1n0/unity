# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

from dbus import Interface, SessionBus

from unity.emulators import UnityIntrospectionObject
from unity.emulators.dash import DashController
from unity.emulators.hud import HudController
from unity.emulators.launcher import LauncherController
from unity.emulators.panel import PanelController
from unity.emulators.screen import Screen
from unity.emulators.shortcut_hint import ShortcutController
from unity.emulators.switcher import SwitcherController
from unity.emulators.window_manager import WindowManager


# acquire the debugging dbus object
UNITY_BUS_NAME = 'com.canonical.Unity'
DEBUG_PATH = '/com/canonical/Unity/Debug'
LOGGING_IFACE = 'com.canonical.Unity.Debug.Logging'


def get_dbus_proxy_object():
    return SessionBus().get_object(UNITY_BUS_NAME, DEBUG_PATH)


def get_dbus_logging_interface():
    return Interface(get_dbus_proxy_object(), LOGGING_IFACE)


def start_log_to_file(file_path):
    """Instruct Unity to start logging to the given file."""
    get_dbus_logging_interface().StartLogToFile(file_path)


def reset_logging():
    """Instruct Unity to stop logging to a file."""
    get_dbus_logging_interface().ResetLogging()


def set_log_severity(component, severity):
    """Instruct Unity to set a log component's severity.

    'component' is the unity logging component name.

    'severity' is the severity name (like 'DEBUG', 'INFO' etc.)

    """
    get_dbus_logging_interface().SetLogSeverity(component, severity)


def log_unity_message(severity, message):
    """Instruct unity to log a message for us.

    severity: one of ('TRACE', 'DEBUG', 'INFO', 'WARNING', 'ERROR').

    message: The message to log.

    For debugging purposes only! If you want to log a message during an autopilot
    test, use the python logging framework instead.

    """
    get_dbus_logging_interface().LogMessage(severity, message)


class Unity(UnityIntrospectionObject):

    @property
    def screen(self):
        return self.get_children_by_type(Screen)[0]

    @property
    def dash(self):
        return self.get_children_by_type(DashController)[0]

    @property
    def hud(self):
        return self.get_children_by_type(HudController)[0]

    @property
    def launcher(self):
        return self.get_children_by_type(LauncherController)[0]

    @property
    def panels(self):
        return self.get_children_by_type(PanelController)[0]

    @property
    def switcher(self):
        return self.get_children_by_type(SwitcherController)[0]

    @property
    def shortcut_hint(self):
        return self.get_children_by_type(ShortcutController)[0]

    @property
    def window_manager(self):
        return self.get_children_by_type(WindowManager)[0]
