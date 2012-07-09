# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

from autopilot.emulators.dbus_handler import session_bus
from dbus import Interface

# acquire the debugging dbus object
UNITY_BUS_NAME = 'com.canonical.Unity'
DEBUG_PATH = '/com/canonical/Unity/Debug'
LOGGING_IFACE = 'com.canonical.Unity.Debug.Logging'


_debug_proxy_obj = session_bus.get_object(UNITY_BUS_NAME, DEBUG_PATH)
_logging_iface = Interface(_debug_proxy_obj, LOGGING_IFACE)

def start_log_to_file(file_path):
    """Instruct Unity to start logging to the given file."""
    _logging_iface.StartLogToFile(file_path)


def reset_logging():
    """Instruct Unity to stop logging to a file."""
    _logging_iface.ResetLogging()


def set_log_severity(component, severity):
    """Instruct Unity to set a log component's severity.

    'component' is the unity logging component name.

    'severity' is the severity name (like 'DEBUG', 'INFO' etc.)

    """
    _logging_iface.SetLogSeverity(component, severity)


def log_unity_message(severity, message):
    """Instruct unity to log a message for us.

    severity: one of ('TRACE', 'DEBUG', 'INFO', 'WARNING', 'ERROR').

    message: The message to log.

    For debugging purposes only! If you want to log a message during an autopilot
    test, use the python logging framework instead.

    """
    _logging_iface.LogMessage(severity, message)

