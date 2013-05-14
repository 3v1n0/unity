# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

"""A collection of Unity-specific emulators."""

from time import sleep

from autopilot.introspection.dbus import DBusIntrospectionObject
from autopilot.introspection.backends import DBusAddress

from dbus import DBusException


class UnityIntrospectionObject(DBusIntrospectionObject):

    DBUS_SERVICE = "com.canonical.Unity"
    DBUS_OBJECT = "/com/canonical/Unity/Debug"

    _Backend = DBusAddress.SessionBus(DBUS_SERVICE, DBUS_OBJECT)


def ensure_unity_is_running(timeout=300):
    """Poll the unity debug interface, and return when it's ready for use.

    The default timeout is 300 seconds (5 minutes) to account for the case where
    Unity has crashed and is taking a while to get restarted (on a slow VM for
    example).

    If, after the timeout period, unity is still not up, this function raises a
    RuntimeError exception.

    """
    sleep_period=10
    for i in range(0, timeout, sleep_period):
        try:
            UnityIntrospectionObject.get_state_by_path("/")
            return True
        except DBusException:
            sleep(sleep_period)
    raise RuntimeError("Unity debug interface is down after %d seconds of polling." % (timeout))
