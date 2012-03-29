# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from autopilot.keybindings import KeybindingsHelper
from autopilot.emulators.unity import UnityIntrospectionObject
from autopilot.emulators.unity.dash import SearchBar

class Hud(KeybindingsHelper):
    """An emulator class that makes it easier to iteract with unity hud."""
    
    def __init__(self):
      assert len(HudController.get_all_instances()) > 0
      self.controller = HudController.get_all_instances()[0]

      super (Hud, self).__init__()

    def ensure_hidden(self):
        """Hides the hud if it's not already hidden."""
        if self.visible:
            self.toggle_reveal()

    def ensure_visible(self):
        """Shows the hud if it's not already showing."""
        if not self.visible:
            self.toggle_reveal()

    def toggle_reveal(self, tap_delay=0.1):
        """Tap the 'Alt' key to toggle the hud visibility."""
        self.keybinding("hud/reveal", tap_delay)

    @property
    def view(self):
        """Returns the HudView."""
        return self.controller.get_hud_view()
    
    @property
    def visible(self):
        """Is the Hud visible?"""
        return self.controller.visible

    @property
    def searchbar(self):
        """Returns the searchbar attached to the hud."""
        return self.controller.get_hud_view().searchbar

    @property
    def search_string(self):
        """Returns the searchbars' search string."""
        return self.searchbar.search_string

    @property
    def selected_button(self):
        view = self.controller.get_hud_view()
        if view:
            return view.selected_button
        else:
            return 0

    @property
    def num_buttons(self):
        view = self.controller.get_hud_view()
        if view:
            return view.num_buttons
        else:
            return 0

class HudView(UnityIntrospectionObject):
    """Proxy object for the hud view child of the controller."""
  
    @property
    def searchbar(self):
        """Get the search bar attached to this hud view."""
        return self.get_children_by_type(SearchBar)[0]

class HudController(UnityIntrospectionObject):
    """Proxy object for the Unity Hud Controller."""

    def get_hud_view(self):
        views = self.get_children_by_type(HudView)
        return views[0] if views else None
