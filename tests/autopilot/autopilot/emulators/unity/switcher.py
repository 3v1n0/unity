# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from time import sleep

from autopilot.emulators.unity import get_state_by_path, make_launcher_icon
from autopilot.emulators.X11 import Keyboard

class Switcher(object):
    """Interact with the Unity switcher."""

    def __init__(self):
        super(Switcher, self).__init__()
        self._keyboard = Keyboard()

    def initiate(self):
        """Start the switcher with alt+tab."""
        self._keyboard.press('Alt')
        self._keyboard.press_and_release('Tab')
        sleep(1)

    def initiate_detail_mode(self):
        """Start detail mode with alt+`"""
        self._keyboard.press('Alt')
        self._keyboard.press_and_release('`')

    def terminate(self):
        """Stop switcher."""
        self._keyboard.release('Alt')

    def next_icon(self):
        """Move to the next application."""
        self._keyboard.press_and_release('Tab')

    def previous_icon(self):
        """Move to the previous application."""
        self._keyboard.press_and_release('Shift+Tab')

    def show_details(self):
        """Show detail mode."""
        self._keyboard.press_and_release('`')

    def hide_details(self):
        """Hide detail mode."""
        self._keyboard.press_and_release('Up')

    def next_detail(self):
        """Move to next detail in the switcher."""
        self._keyboard.press_and_release('`')

    def previous_detail(self):
        """Move to the previous detail in the switcher."""
        self._keyboard.press_and_release('Shift+`')

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
            return make_launcher_icon(model['Children'][sel_idx])
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

    def get_switcher_icons(self):
        """Get all icons in the switcher model.

        The switcher needs to be initiated in order to get the model.

        """
        icons = []
        model = self.get_state_by_path('//SwitcherModel')[0]
        for child in model['Children']:
            icon = make_launcher_icon(child)
            if icon:
                icons.append(icon)
        return icons

    def __get_model(self):
        return get_state_by_path('/Unity/SwitcherController/SwitcherModel')[0]

    def __get_controller(self):
        return get_state_by_path('/unity/SwitcherController')[0]
