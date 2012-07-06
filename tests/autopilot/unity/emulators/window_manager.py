# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

import logging
from autopilot.keybindings import KeybindingsHelper

from unity.emulators import UnityIntrospectionObject

logger = logging.getLogger(__name__)


class WindowManager(UnityIntrospectionObject, KeybindingsHelper):
    """The WindowManager class."""

    @property
    def screen_geometry(self):
        """Returns a tuple of (x,y,w,h) for the screen."""
        return (self.x, self.y, self.width, self.height)

    def enter_show_desktop(self):
        if not self.showdesktop_active:
            logger.info("Entering show desktop mode.")
            self.keybinding("window/show_desktop")
            self.showdesktop_active.wait_for(True)
        else:
            logger.warning("Test tried to enter show desktop mode while already \
                in show desktop mode.")

    def leave_show_desktop(self):
        if self.showdesktop_active:
            logger.info("Leaving show desktop mode.")
            self.keybinding("window/show_desktop")
            self.showdesktop_active.wait_for(False)
        else:
            logger.warning("Test tried to leave show desktop mode while not in \
                show desktop mode.")
