# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

from autopilot.emulators.X11 import Keyboard
from autopilot.keybindings import KeybindingsHelper
from HTMLParser import HTMLParser
import re

from unity.emulators import UnityIntrospectionObject
from unity.emulators.dash import SearchBar
from unity.emulators.icons import HudEmbeddedIcon, HudLauncherIcon


class Hud(KeybindingsHelper):
    """An emulator class that makes it easier to iteract with unity hud."""

    def __init__(self):
        super (Hud, self).__init__()
        controllers = HudController.get_all_instances()
        assert(len(controllers) == 1)
        self.controller = controllers[0]
        self.keyboard = Keyboard()

    def ensure_hidden(self):
        """Hides the hud if it's not already hidden."""
        if self.visible:
            if self.search_string:
                # need this to clear the search string, and then another to
                # close the hud.
                self.keyboard.press_and_release("Escape")
                self.search_string.wait_for("")
            self.keyboard.press_and_release("Escape")
            self.visible.wait_for(False)

    def ensure_visible(self):
        """Shows the hud if it's not already showing."""
        if not self.visible:
            self.toggle_reveal()
            self.visible.wait_for(True)

    def toggle_reveal(self, tap_delay=0.1):
        """Tap the 'Alt' key to toggle the hud visibility."""
        old_state = self.visible
        self.keybinding("hud/reveal", tap_delay)
        self.visible.wait_for(not old_state)

    def get_embedded_icon(self):
        """Returns the HUD view embedded icon or None if is not shown."""
        view = self.view
        if (not view):
          return None

        icons = view.get_children_by_type(HudEmbeddedIcon)
        return icons[0] if icons else None

    def get_launcher_icon(self):
        """Returns the HUD launcher icon"""
        icons = HudLauncherIcon.get_all_instances()
        assert(len(icons) == 1)
        return icons[0]

    @property
    def icon(self):
        if self.is_locked_launcher:
            return self.get_launcher_icon()
        else:
            return self.get_embedded_icon()

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
    def is_locked_launcher(self):
        return self.controller.locked_to_launcher

    @property
    def monitor(self):
        return self.controller.hud_monitor

    @property
    def geometry(self):
        return (self.controller.x, self.controller.y, self.controller.width, self.controller.height)

    @property
    def selected_button(self):
        view = self.controller.get_hud_view()
        if view:
            return view.selected_button
        else:
            return 0

    @property
    def hud_buttons(self):
        """Returns a list of current HUD buttons."""
        return self.view.hud_buttons

    @property
    def selected_hud_button(self):
        try:
            [button] = filter(lambda x: x.focused, self.hud_buttons)
            return button
        except IndexError:
            raise RuntimeError("No HUD buttons found, is hud active?")

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

    @property
    def hud_buttons(self):
        return self.get_children_by_type(HudButton)

    @property
    def geometry(self):
        return (self.x, self.y, self.width, self.height)


class HudController(UnityIntrospectionObject):
    """Proxy object for the Unity Hud Controller."""

    def get_hud_view(self):
        views = self.get_children_by_type(HudView)
        return views[0] if views else None

class HudButton(UnityIntrospectionObject):
    """Proxy object for the hud buttons."""

    @property
    def label_no_formatting(self):
        """Returns the label text with the formatting removed."""
        htmlparser = HTMLParser()
        return htmlparser.unescape(re.sub("<[^>]*>", "", self.label))
