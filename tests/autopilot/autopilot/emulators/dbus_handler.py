# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Initialise dbus once using glib mainloop."""

#
# DBus has an annoying bug where we need to initialise it with the gobject main
# loop *before* it's initialised anywhere else. This module exists so we can
# initialise the dbus module once, and once only.

import dbus
import dbus.glib
from dbus.mainloop.glib import DBusGMainLoop
import gobject

DBusGMainLoop(set_as_default=True)
session_bus = dbus.SessionBus()

gobject.threads_init()
dbus.glib.init_threads()
