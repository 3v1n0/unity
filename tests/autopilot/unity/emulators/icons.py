# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

from unity.emulators import UnityIntrospectionObject
from unity.emulators.quicklist import Quicklist
from unity.emulators.tooltip import ToolTip

class SimpleLauncherIcon(UnityIntrospectionObject):
    """Holds information about a simple launcher icon.

    Do not instantiate an instance of this class yourself. Instead, use the
    appropriate methods in the Launcher class instead.

    """

    @property
    def center_position(self):
        """Get the center point of an icon, returns a tuple with (x, y, z)."""
        return self.center

    def get_quicklist(self):
        """Get the quicklist for this launcher icon.

        This may return None, if there is no quicklist associated with this
        launcher icon.

        """
        matches = self.get_children_by_type(Quicklist)
        return matches[0] if matches else None

    def get_tooltip(self):
        """Get the tooltip for this launcher icon.

        This may return None, if there is no tooltip associated with this
        launcher icon.

        """
        matches = self.get_children_by_type(ToolTip)
        return matches[0] if matches else None

    def is_on_monitor(self, monitor):
        """Returns True if the icon is available in the defined monitor."""
        if monitor >= 0 and monitor < len(self.monitors_visibility):
            return self.monitors_visibility[monitor]

        return False

    def controls_window(self, xid):
        """Returns true if the icon controls the specified xid."""

        return self.xids.contains(xid)

    def __repr__(self):
        with self.no_automatic_refreshing():
            return "<%s id=%d>" % (self.__class__.__name__, self.id)


class BFBLauncherIcon(SimpleLauncherIcon):
    """Represents the BFB button in the launcher."""


class ExpoLauncherIcon(SimpleLauncherIcon):
    """Represents the Expo button in the launcher."""


class HudLauncherIcon(SimpleLauncherIcon):
    """Represents the HUD button in the launcher."""


class ApplicationLauncherIcon(SimpleLauncherIcon):
    """Represents a launcher icon with BAMF integration."""

    def __repr__(self):
        with self.no_automatic_refreshing():
            return "<%s %s id=%d>" % (
                self.__class__.__name__,
                self.desktop_id,
                self.id)

class TrashLauncherIcon(SimpleLauncherIcon):
    """Represents the trash launcher icon."""


class DeviceLauncherIcon(SimpleLauncherIcon):
    """Represents a device icon in the launcher."""


class DesktopLauncherIcon(SimpleLauncherIcon):
    """Represents an icon that may appear in the switcher."""


class VolumeLauncherIcon(SimpleLauncherIcon):
    """Represents a mounted disk icon in the launcher."""


class SoftwareCenterLauncherIcon(ApplicationLauncherIcon):
    """Represents a launcher icon of a Software Center app."""


class HudEmbeddedIcon(UnityIntrospectionObject):
    """Proxy object for the hud embedded icon child of the view."""

    @property
    def geometry(self):
        return (self.x, self.y, self.width, self.height)


class LauncherEntry(UnityIntrospectionObject):
    """Proxy for the LauncherEntryRemote instances in Unity."""
