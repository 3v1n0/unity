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
        super(SimpleLauncherIcon, self).__init__(icon_dict)

    def refresh_state(self):
        """Re-get the LauncherIcon's state from unity, updating it's public properties."""
        state = self.get_state_by_path('//LauncherIcon[id=%d]' % (self.id))
        super(SimpleLauncherIcon, self).set_properties(state[0])

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


class TrashLauncherIcon(SimpleLauncherIcon):
    """Represents the trash launcher icon."""


class DeviceLauncherIcon(SimpleLauncherIcon):
    """Represents a device icon in the launcher."""


class DesktopLauncherIcon(SimpleLauncherIcon):
    """Represents an icon that may appear in the switcher."""


