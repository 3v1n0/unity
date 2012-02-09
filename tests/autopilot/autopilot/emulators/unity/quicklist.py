# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from time import sleep

from autopilot.emulators.unity import get_state_by_path
from autopilot.emulators.X11 import Mouse


class Quicklist(object):
    """Represents a quicklist."""
    def __init__(self, state_dict):
        super(Quicklist, self).__init__()
        self._set_properties(state_dict)

    def _set_properties(self, state_dict):
        self.id = state_dict['id']
        self.x = state_dict['x']
        self.y = state_dict['y']
        self.width = state_dict['width']
        self.height = state_dict['height']
        self.active = state_dict['active']
        self._children = state_dict['Children']

    def refresh_state(self):
        state = get_state_by_path('//Quicklist[id=%d]' % (self.id))
        self._set_properties(state[0])

    @property
    def items(self):
        """Individual items in the quicklist."""
        return [self.__make_quicklist_from_data(ctype, cdata) for ctype,cdata in self._children]

    def get_quicklist_item_by_text(self, text):
        """Returns a QuicklistMenuItemLabel object with the given text, or None."""
        matches = []
        for item in self.items:
            if type(item) is QuicklistMenuItemLabel and item.text == text:
                matches.append(item)
        if len(matches) > 0:
            return matches[0]

    def __make_quicklist_from_data(self, quicklist_type, quicklist_data):
        if quicklist_type == 'QuicklistMenuItemLabel':
            return QuicklistMenuItemLabel(quicklist_data)
        elif quicklist_type == 'QuicklistMenuItemSeparator':
            return QuicklistMenuItemSeparator(quicklist_data)
        else:
            raise ValueError('Unknown quicklist item type "%s"' % (quicklist_type))

    def click_item(self, item):
        """Click one of the quicklist items."""
        if not isinstance(item, QuicklistMenuItem):
            raise TypeError("Item must be a subclass of QuicklistMenuItem")
        mouse = Mouse()
        mouse.move(self.x + item.x + (item.width /2),
                        self.y + item.y + (item.height /2))
        sleep(0.25)
        mouse.click()



class QuicklistMenuItem(object):
    """Represents a single item in a quicklist."""
    def __init__(self, state_dict):
        super(QuicklistMenuItem, self).__init__()
        self._set_properties(state_dict)

    def _set_properties(self, state_dict):
        self.visible = state_dict['visible']
        self.enabled = state_dict['enabled']
        self.width = state_dict['width']
        self.height = state_dict['height']
        self.x = state_dict['x']
        self.y = state_dict['y']
        self.id = state_dict['id']
        self.lit = state_dict['lit']


class QuicklistMenuItemLabel(QuicklistMenuItem):
    """Represents a text label inside a quicklist."""

    def _set_properties(self, state_dict):
        super(QuicklistMenuItemLabel, self)._set_properties(state_dict)
        self.text = state_dict['text']


class QuicklistMenuItemSeparator(QuicklistMenuItem):
    """Represents a separator in a quicklist."""
