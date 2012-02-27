# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from autopilot import keybindings
from autopilot.emulators.unity import UnityIntrospectionObject
from autopilot.emulators.X11 import Keyboard


class HudController(UnityIntrospectionObject):
    """Proxy object for the Unity Hud Controller."""

    def ensure_hidden(self):
        """Hides the hud if it's not already hidden."""
        if self.is_visible():
            self.toggle_reveal()

    def ensure_visible(self):
        """Shows the hud if it's not already showing."""
        if not self.is_visible():
            self.toggle_reveal()

    def is_visible(self):
        self.refresh_state()
        return self.visible

    def toggle_reveal(self):
        """Tap the 'Alt' key to toggle the hud visibility."""
        kb = Keyboard()
        kb.press_and_release(keybindings.get("hud/reveal"), 0.1)
