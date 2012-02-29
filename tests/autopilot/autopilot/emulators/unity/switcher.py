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

from autopilot.keybindings import KeybindingsHelper
from autopilot.emulators.unity import get_state_by_path, make_introspection_object
from autopilot.emulators.X11 import Keyboard, Mouse

# even though we don't use these directly, we need to make sure they've been
# imported so the classes contained are registered with the introspection API.
from autopilot.emulators.unity.icons import *

logger = logging.getLogger(__name__)


class Switcher(KeybindingsHelper):
    """Interact with the Unity switcher."""

    def __init__(self):
        super(Switcher, self).__init__()
        self._keyboard = Keyboard()
        self._mouse = Mouse()

    def initiate(self):
        """Start the switcher with alt+tab."""
        logger.debug("Initiating switcher with Alt+Tab")
        self.keybinding_hold("switcher/reveal_normal")
        self.keybinding_tap("switcher/reveal_normal")
        sleep(1)

    def initiate_detail_mode(self):
        """Start detail mode with alt+`"""
        logger.debug("Initiating switcher detail mode with Alt+`")
        self.keybinding_hold("switcher/reveal_details")
        self.keybinding_tap("switcher/reveal_details")
        sleep(1)

    def initiate_all_mode(self):
        """Start switcher in 'all workspaces' mode.

        Shows apps from all workspaces, instead of just the current workspace.
        """
        logger.debug("Initiating switcher in 'all workspaces' mode.")
        self.keybinding_hold("switcher/reveal_all")
        self.keybinding_tap("switcher/reveal_all")
        sleep(1)

    def terminate(self):
        """Stop switcher without activating the selected icon."""
        logger.debug("Terminating switcher.")
        self.keybinding("switcher/cancel")
        self.keybinding_release("switcher/reveal_normal")

    def stop(self):
        """Stop switcher and activate the selected icon."""
        logger.debug("Stopping switcher")
        self.keybinding_release("switcher/reveal_normal")

    def next_icon(self):
        """Move to the next icon."""
        logger.debug("Selecting next item in switcher.")
        self.keybinding("switcher/next")

    def previous_icon(self):
        """Move to the previous icon."""
        logger.debug("Selecting previous item in switcher.")
        self.keybinding("switcher/prev")

    def next_icon_mouse(self):
        """Move to the next icon using the mouse scroll wheel"""
        logger.debug("Selecting next item in switcher with mouse scroll wheel.")
        self._mouse.press(6)
        self._mouse.release(6)

    def previous_icon_mouse(self):
        """Move to the previous icon using the mouse scroll wheel"""
        logger.debug("Selecting previous item in switcher with mouse scroll wheel.")
        self._mouse.press(7)
        self._mouse.release(7)

    def show_details(self):
        """Show detail mode."""
        logger.debug("Showing details view.")
        self.keybinding("switcher/detail_start")

    def hide_details(self):
        """Hide detail mode."""
        logger.debug("Hiding details view.")
        self.keybinding("switcher/detail_stop")

    def next_detail(self):
        """Move to next detail in the switcher."""
        logger.debug("Selecting next item in details mode.")
        self.keybinding("switcher/detail_next")

    def previous_detail(self):
        """Move to the previous detail in the switcher."""
        logger.debug("Selecting previous item in details mode.")
        self.keybinding("switcher/detail_prev")

    def __get_icon(self, index):
        return self.__get_model()['Children'][index][1][0]

    @property
    def current_icon(self):
        """Get the currently-selected icon."""
        if not self.get_is_visible:
            return None
        model = self.__get_model()
        sel_idx = self.get_selection_index()
        try:
            return make_introspection_object(model['Children'][sel_idx])
        except KeyError:
            return None

    def get_icon_name(self, index):
        return self.__get_icon(index)['tooltip-text']

    def get_icon_desktop_file(self, index):
        try:
            return self.__get_icon(index)['desktop-file']
        except:
            return None

    def get_model_size(self):
        return len(self.__get_model()['Children'])

    def get_selection_index(self):
        return int(self.__get_model()['selection-index'])

    def get_last_selection_index(self):
        return bool(self.__get_model()['last-selection-index'])

    def get_is_visible(self):
        return bool(self.__get_controller()['visible'])

    def get_is_in_details_mode(self):
        """Return True if the SwitcherView is in details mode."""
        return bool(self.__get_model()['detail-selection'])

    def get_switcher_icons(self):
        """Get all icons in the switcher model.

        The switcher needs to be initiated in order to get the model.

        """
        icons = []
        model = get_state_by_path('//SwitcherModel')[0]
        for child in model['Children']:
            icon = make_introspection_object(child)
            if icon:
                icons.append(icon)
        return icons

    def __get_model(self):
        return get_state_by_path('/Unity/SwitcherController/SwitcherModel')[0]

    def __get_controller(self):
        return get_state_by_path('/unity/SwitcherController')[0]
