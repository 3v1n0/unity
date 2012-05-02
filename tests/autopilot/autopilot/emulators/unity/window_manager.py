# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

import logging
from autopilot.emulators.unity import UnityIntrospectionObject

logger = logging.getLogger(__name__)


class WindowManager(UnityIntrospectionObject):
    """The WindowManager class."""

    @property
    def screen_geometry(self):
        """Returns a tuple of (x,y,w,h) for the screen."""
        return (self.x, self.y, self.width, self.height)
