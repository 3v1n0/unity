# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

import logging

from autopilot.input import Mouse

from unity.emulators import UnityIntrospectionObject

logger = logging.getLogger(__name__)


class Quicklist(UnityIntrospectionObject):
    """Represents a quicklist."""

    @property
    def items(self):
        """Individual items in the quicklist."""
        return self.get_children_by_type(QuicklistMenuItem, visible=True)

    @property
    def selectable_items(self):
        """Items that can be selected in the quicklist."""
        return self.get_children_by_type(QuicklistMenuItem, visible=True, selectable=True)

    def get_quicklist_item_by_text(self, text):
        """Returns a QuicklistMenuItemLabel object with the given text, or None."""
        if not self.active:
            raise RuntimeError("Cannot get quicklist items. Quicklist is inactive!")

        matches = self.get_children_by_type(QuicklistMenuItemLabel, text=text)

        return matches[0] if matches else None

    def get_quicklist_application_item(self, application_name):
        """Returns the QuicklistMenuItemLabel for the given application_name"""
        return self.get_quicklist_item_by_text("<b>"+application_name+"</b>")

    def click_item(self, item):
        """Click one of the quicklist items."""
        if not isinstance(item, QuicklistMenuItem):
            raise TypeError("Item must be a subclass of QuicklistMenuItem")

        item.mouse_click()

    def move_mouse_to_right(self):
        """Moves the mouse outside the quicklist"""
        logger.debug("Moving mouse outside the quicklist %r", self)
        target_x = self.x + self.width + 10
        target_y = self.y + self.height / 2
        Mouse.create().move(target_x, target_y, animate=False)

    @property
    def selected_item(self):
        items = self.get_children_by_type(QuicklistMenuItem, selected=True)
        assert(len(items) <= 1)
        return items[0] if items else None

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the quicklist."""
        return (self.x, self.y, self.width, self.height)


class QuicklistMenuItem(UnityIntrospectionObject):
    """Represents a single item in a quicklist."""

    def __init__(self, *args, **kwargs):
        super(QuicklistMenuItem, self).__init__(*args, **kwargs)
        self._mouse = Mouse.create()

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the quicklist item."""
        return (self.x, self.y, self.width, self.height)

    def mouse_move_to(self):
        assert(self.visible)
        logger.debug("Moving mouse over quicklist item %r", self)
        target_x = self.x + self.width / 2
        target_y = self.y + self.height / 2
        self._mouse.move(target_x, target_y, rate=20, time_between_events=0.005)

    def mouse_click(self, button=1):
        logger.debug("Clicking on quicklist item %r", self)
        self.mouse_move_to()
        self._mouse.click()


class QuicklistMenuItemLabel(QuicklistMenuItem):
    """Represents a text label inside a quicklist."""


class QuicklistMenuItemSeparator(QuicklistMenuItem):
    """Represents a separator in a quicklist."""
