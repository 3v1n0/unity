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
from unity.emulators import UnityIntrospectionObject
logger = logging.getLogger(__name__)


class Screen(UnityIntrospectionObject):
    """The Screen class."""

    @property
    def windows(self):
        """Return the available windows, or None."""
        return self.get_children_by_type(Window)

    @property
    def scaled_windows(self):
        """Return the available scaled windows, or None."""
        return self.get_children_by_type(Window, scaled=True)


class Window(UnityIntrospectionObject):
    """An individual window."""

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the current window."""
        return (self.x, self.y, self.width, self.height)

    @property
    def scale_close_geometry(self):
        """Returns a tuple of (x,y,w,h) for the scale close button."""
        return (self.scaled_close_x, self.scaled_close_y, self.scaled_close_width, self.scaled_close_height)
