# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from autopilot.emulators.unity import (get_state_by_path,
                                        ObjectCreatableFromStateDict,
                                        )
from autopilot.emulators.unity.quicklist import Quicklist

class SimpleLauncherIcon(ObjectCreatableFromStateDict):
    """Holds information about a simple launcher icon.

    Do not instantiate an instance of this class yourself. Instead, use the
    appropriate methods in the Launcher class instead.

    """

    def __init__(self, icon_dict):
        UnityIntrospectableObject.__init__()
        self._set_properties(icon_dict)

    def refresh_state(self):
        """Re-get the LauncherIcon's state from unity, updating it's public properties."""
        state = self.get_state_by_path('//LauncherIcon[id=%d]' % (self.id))
        self._set_properties(state[0])

    def _set_properties(self, state_from_unity):
        # please keep these in the same order as they are in unity:
        self.urgent = state_from_unity['quirk-urgent']
        self.presented = state_from_unity['quirk-presented']
        self.visible = state_from_unity['quirk-visible']
        self.sort_priority = state_from_unity['sort-priority']
        self.running = state_from_unity['quirk-running']
        self.active = state_from_unity['quirk-active']
        self.icon_type = state_from_unity['icon-type']
        self.related_windows = state_from_unity['related-windows']
        self.y = state_from_unity['y']
        self.x = state_from_unity['x']
        self.z = state_from_unity['z']
        self.id = state_from_unity['id']
        self.tooltip_text = unicode(state_from_unity['tooltip-text'])

    def get_quicklist(self):
        """Get the quicklist for this launcher icon.

        This may return None, if there is no quicklist associated with this
        launcher icon.

        """
        ql_state = get_state_by_path('//LauncherIcon[id=%d]/Quicklist' % (self.id))
        if len(ql_state) > 0:
            return Quicklist(ql_state[0])


class BFBLauncherIcon(SimpleLauncherIcon):
    """Represents the BFB button in the launcher."""


class BamfLauncherIcon(SimpleLauncherIcon):
    """Represents a launcher icon with BAMF integration."""

    def _set_properties(self, state_from_unity):
        super(BamfLauncherIcon, self)._set_properties(state_from_unity)
        self.desktop_file = state_from_unity['desktop-file']
        self.sticky = bool(state_from_unity['sticky'])


class TrashLauncherIcon(SimpleLauncherIcon):
    """Represents the trash launcher icon."""


class DeviceLauncherIcon(SimpleLauncherIcon):
    """Represents a device icon in the launcher."""


class DesktopLauncherIcon(SimpleLauncherIcon):
    """Represents an icon that may appear in the switcher."""


