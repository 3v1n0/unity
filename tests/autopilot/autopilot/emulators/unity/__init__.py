# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from dbus import Interface
import logging

from autopilot.emulators.dbus_handler import session_bus

_object_registry = {}
logger = logging.getLogger(__name__)


class StateNotFoundError(RuntimeError):
    """Raised when a piece of state information from unity is not found."""

    message = "State not found for class with name '{}' and id '{}'."

    def __init__(self, class_name, class_id):
        super(StateNotFoundError, self).__init__(self.message.format(class_name, class_id))


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


def make_introspection_object(dbus_tuple):
    """Make an introspection object given a DBus tuple of (name, state_dict).

    This only works for classes that derive from UnityIntrospectionObject.
    """
    name, state = dbus_tuple
    try:
        class_type = _object_registry[name]
    except KeyError:
        print name, "is not a valid introspection type!"
        return None
    return class_type(state)


class UnityIntrospectionObject(object):
    """A class that can be created using a dictionary of state from Unity."""
    __metaclass__ = IntrospectableObjectMetaclass

    def __init__(self, state_dict):
        self.__state = {}
        self.set_properties(state_dict)

    def set_properties(self, state_dict):
        """Creates and set attributes of `self` based on contents of `state_dict`.

        Translates '-' to '_', so a key of 'icon-type' for example becomes 'icon_type'.

        """
        self.__state = {}
        for key, value in state_dict.iteritems():
            # don't store id in state dictionary -make it a proper instance attribute
            if key == 'id':
                self.id = value
            self.__state[key.replace('-', '_')] = value

    def _get_child_tuples_by_type(self, desired_type):
        """Get a list of (name,dict) pairs from children of the specified type.

        desired_type must be a subclass of UnityIntrospectionObject.

        """
        if not issubclass(desired_type, UnityIntrospectionObject):
            raise TypeError("%r must be a subclass of %r" % (desired_type,
                UnityIntrospectionObject))

        children = getattr(self, 'Children', [])
        results = []
        # loop through all children, and try find one that matches the type the
        # user wants.
        for child_type, child_state in children:
            try:
                if issubclass(_object_registry[child_type], desired_type):
                    results.append((child_type, child_state))
            except KeyError:
                pass
        return results

    def get_children_by_type(self, desired_type, **kwargs):
        """Get a list of children of the specified type.

        desired_type must be a subclass of UnityIntrospectionObject.

        Keyword arguments can be used to restrict returned instances. For example:

        >>> get_children_by_type(Launcher, monitor=1)

        ... will return only LauncherInstances that have an attribute 'monitor'
            that is equal to 1.

        """
        self.refresh_state()
        result = []
        for child in self._get_child_tuples_by_type(desired_type):
            instance = make_introspection_object(child)
            for attr, val in kwargs.iteritems():
                if not hasattr(instance, attr) or getattr(instance, attr) != val:
                    # Either attribute is not present, or is present but with
                    # the wrong value - don't add this instance to the results list.
                    continue
            result.append(instance)
        return result

    def refresh_state(self):
        """Refreshes the object's state from unity.

        raises StateNotFound if the object in unity has been destroyed.

        """
        # need to get name from class object.
        new_state = get_state_by_name_and_id(self.__class__.__name__, self.id)
        self.set_properties(new_state)

    @classmethod
    def get_all_instances(cls):
        """Get all instances of this class that exist within the Unity state tree.

        For example, to get all the BamfLauncherIcons:

        icons = BamfLauncherIcons.get_all_instances()

        The return value is a list (possibly empty) of class instances.

        """
        cls_name = cls.__name__
        instances = get_state_by_path("//%s" % (cls_name))
        return [make_introspection_object((cls_name,i)) for i in instances]

    def __getattr__(self, name):
        # avoid recursion if for some reason we have no state set (should never)
        # happen.
        if name == '__state':
            raise AttributeError()

        if name in self.__state:
            self.refresh_state()
            return self.__state[name]
        # attribute not found.
        raise AttributeError("Attribute '%s' not found." % (name))
