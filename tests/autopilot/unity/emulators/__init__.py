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

from autopilot.introspection import (
    get_proxy_object_for_existing_process,
    ProcessSearchError
    )
from autopilot.introspection.dbus import CustomEmulatorBase
from autopilot.introspection.backends import DBusAddress

from dbus import DBusException

keys = {
        "Left/launcher/keynav/prev": "launcher/keynav/prev",
        "Left/launcher/keynav/next": "launcher/keynav/next",
        "Left/launcher/keynav/open-quicklist": "launcher/keynav/open-quicklist",
        "Bottom/launcher/keynav/prev": "launcher/keynav/close-quicklist",
        "Bottom/launcher/keynav/next": "launcher/keynav/open-quicklist",
        "Bottom/launcher/keynav/open-quicklist": "launcher/keynav/prev",
}
class UnityIntrospectionObject(CustomEmulatorBase):

    DBUS_SERVICE = "com.canonical.Unity"
    DBUS_OBJECT = "/com/canonical/Unity/Debug"

    _Backend = DBusAddress.SessionBus(DBUS_SERVICE, DBUS_OBJECT)

    def _repr_string(self, obj_details=""):
        geostr = ""
        if hasattr(self, 'globalRect'):
            geostr = " geo=[{r.x}x{r.y} {r.width}x{r.height}]".format(r=self.globalRect)

        obj_details.strip()
        obj_details = " "+obj_details if len(obj_details) else ""

        return "<{cls} {addr} id={id}{geo}{details}>".format(cls=self.__class__.__name__,
                                                             addr=hex(id(self)),
                                                             id=self.id,
                                                             geo=geostr,
                                                             details=obj_details)

    def __repr__(self):
        with self.no_automatic_refreshing():
            return self._repr_string()

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.id == other.id

    def __ne__(self, other):
        return not self.__eq__(other)


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
            get_proxy_object_for_existing_process(
                connection_name=UnityIntrospectionObject.DBUS_SERVICE,
                object_path=UnityIntrospectionObject.DBUS_OBJECT
            )
            return True
        except ProcessSearchError:
            sleep(sleep_period)
    raise RuntimeError("Unity debug interface is down after %d seconds of polling." % (timeout))
