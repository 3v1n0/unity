# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

import logging
from time import sleep

from autopilot.emulators.unity import ObjectCreatableFromStateDict
from autopilot.emulators.X11 import Mouse


logger = logging.getLogger(__name__)


class Quicklist(ObjectCreatableFromStateDict):
    """Represents a quicklist."""

    @property
    def items(self):
        """Individual items in the quicklist."""
        return [self.__make_quicklist_from_data(ctype, cdata) for ctype, cdata in self._children]

    def get_quicklist_item_by_text(self, text):
        """Returns a QuicklistMenuItemLabel object with the given text, or None."""
        if not self.active:
            # try refreshing the state, otherwise bail:
            self.refresh_state()
            if not self.active:
                raise RuntimeError("Cannot get quicklist items. Quicklist is inactive!")

        matches = filter(lambda i: i.text == text,
            self.get_children_by_type(QuicklistMenuItemLabel))

        return matches[0] if matches else None

    def click_item(self, item):
        """Click one of the quicklist items."""
        if not isinstance(item, QuicklistMenuItem):
            raise TypeError("Item must be a subclass of QuicklistMenuItem")

        logger.debug("Clicking on quicklist item %r", item)
        mouse = Mouse()
        mouse.move(self.x + item.x + (item.width / 2),
                    self.y + item.y + (item.height / 2))
        sleep(0.25)
        mouse.click()


class QuicklistMenuItem(ObjectCreatableFromStateDict):
    """Represents a single item in a quicklist."""


class QuicklistMenuItemLabel(QuicklistMenuItem):
    """Represents a text label inside a quicklist."""


class QuicklistMenuItemSeparator(QuicklistMenuItem):
    """Represents a separator in a quicklist."""
