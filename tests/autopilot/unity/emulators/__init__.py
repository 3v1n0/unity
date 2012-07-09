# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

"""A collection of Unity-specific emulators."""


from autopilot.introspection.dbus import DBusIntrospectionObject


class UnityIntrospectionObject(DBusIntrospectionObject):

    DBUS_SERVICE = "com.canonical.Unity"
    DBUS_OBJECT = "/com/canonical/Unity/Debug"
