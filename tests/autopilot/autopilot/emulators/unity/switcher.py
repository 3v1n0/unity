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

from autopilot import keybindings
from autopilot.emulators.unity import get_state_by_path, make_introspection_object
from autopilot.emulators.X11 import Keyboard


logger = logging.getLogger(__name__)


class Switcher(object):
    """Interact with the Unity switcher."""

    def __init__(self):
        super(Switcher, self).__init__()
        self._keyboard = Keyboard()

    def initiate(self):
        """Start the switcher with alt+tab."""
        logger.debug("Initiating switcher with Alt+Tab")
        hold = keybindings.get_hold_part("switcher/reveal_normal")
        tap = keybindings.get_tap_part("switcher/reveal_normal")
        self._keyboard.press(hold)
        self._keyboard.press_and_release(tap)
        sleep(1)

    def initiate_detail_mode(self):
        """Start detail mode with alt+`"""
        logger.debug("Initiating switcher detail mode with Alt+`")
        hold = keybindings.get_hold_part("switcher/reveal_details")
        tap = keybindings.get_tap_part("switcher/reveal_details")
        self._keyboard.press(hold)
        self._keyboard.press_and_release(tap)
        sleep(1)

    def terminate(self):
        """Stop switcher without activating the selected icon."""
        logger.debug("Terminating switcher.")
        self._keyboard.press_and_release(keybindings.get("switcher/cancel"))
        self._keyboard.release(keybindings.get_hold_part("switcher/reveal_normal"))

    def stop(self):
        """Stop switcher and activate the selected icon."""
        logger.debug("Stopping switcher")
        self._keyboard.release(keybindings.get_hold_part("switcher/reveal_normal"))

    def next_icon(self):
        """Move to the next application."""
        logger.debug("Selecting next item in switcher.")
        self._keyboard.press_and_release(keybindings.get("switcher/next"))

    def previous_icon(self):
        """Move to the previous application."""
        logger.debug("Selecting previous item in switcher.")
        self._keyboard.press_and_release(keybindings.get("switcher/prev"))

    def show_details(self):
        """Show detail mode."""
        logger.debug("Showing details view.")
        self._keyboard.press_and_release(keybindings.get("switcher/detail_start"))

    def hide_details(self):
        """Hide detail mode."""
        logger.debug("Hiding details view.")
        self._keyboard.press_and_release(keybindings.get("switcher/detail_stop"))

    def next_detail(self):
        """Move to next detail in the switcher."""
        logger.debug("Selecting next item in details mode.")
        self._keyboard.press_and_release(keybindings.get("switcher/detail_next"))

    def previous_detail(self):
        """Move to the previous detail in the switcher."""
        logger.debug("Selecting previous item in details mode.")
        self._keyboard.press_and_release(keybindings.get("switcher/detail_prev"))

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
