# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from autopilot.emulators.dbus_handler import session_bus
from dbus import Interface


_object_registry = {}


class StateNotFoundError(RuntimeError):
    """Raised when a piece of state information from unity is not found."""

    message = "State not found for class with name '{}' and id '{}'."
    def __init__(self, class_name, class_id):
        super(StateNotFoundError, self).__init__( self.message.format(class_name, class_id))


class IntrospectableObjectMetaclass(type):
    """Metaclass to insert appropriate classes into the object registry."""

    def __new__(self, classname, bases, classdict):
        class_object = type.__new__(self, classname, bases, classdict)
        _object_registry[classname] = class_object
        return class_object


# acquire the debugging dbus object
UNITY_BUS_NAME = 'com.canonical.Unity'
DEBUG_PATH = '/com/canonical/Unity/Debug'
INTROSPECTION_IFACE = 'com.canonical.Unity.Debug.Introspection'


_debug_proxy_obj = session_bus.get_object(UNITY_BUS_NAME, DEBUG_PATH)
_introspection_iface = Interface(_debug_proxy_obj, INTROSPECTION_IFACE)

def get_state_by_path(piece='/Unity'):
    """Returns a full dump of unity's state."""
    return _introspection_iface.GetState(piece)

def get_state_by_name_and_id(class_name, unique_id):
    """Get a state dictionary from unity given a class name and id.

    raises StateNotFoundError if the state is not found.

    Returns a dictionary of information. Unlike get_state_by_path, this
    method can never return state for more than one object.
    """
    try:
        query = "//%(class_name)s[id=%(unique_id)d]" % (dict(
                                                    class_name=class_name,
                                                    unique_id=unique_id))
        return get_state_by_path(query)[0]
    except IndexError:
        raise StateNotFoundError(class_name, unique_id)


class ObjectCreatableFromStateDict(object):
    """A class that can be created using a dictionary of state from Unity."""
    __metaclass__ = IntrospectableObjectMetaclass

    def __init__(self, state_dict):
        self.set_properties(state_dict)

    def set_properties(self, state_dict):
        """Creates and set attributes of `self` based on contents of `state_dict`.

        Translates '-' to '_', so a key of 'icon-type' for example becomes 'icon_type'.

        """
        for key in state_dict.keys():
            setattr(self, key.replace('-','_'), state_dict[key])


def make_launcher_icon(dbus_tuple):
    """Make a launcher icon instance of the appropriate type given the DBus child tuple."""
    name,state = dbus_tuple
    try:
        class_type = _object_registry[name]
    except KeyError:
        print name, "is not a valid icon type!"
        return None
    return class_type(state)


# TODO: remove these - tests should specify what they want properly:

from autopilot.emulators.unity.dash import *
from autopilot.emulators.unity.switcher import *
from autopilot.emulators.unity.launcher import *
from autopilot.emulators.unity.icons import *
from autopilot.emulators.unity.quicklist import *
