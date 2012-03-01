# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

import logging
from time import sleep

from autopilot.keybindings import KeybindingsHelper
from autopilot.emulators.unity import get_state_by_path, make_introspection_object
from autopilot.emulators.X11 import Keyboard


logger = logging.getLogger(__name__)


class ShortcutHint(KeybindingsHelper):
    """Interact with the unity Launcher."""

    def __init__(self):
        super(ShortcutHint, self).__init__()
        self._keyboard = Keyboard()

    def show(self):
        logger.debug("Revealing shortcut hint with keyboard.")
        self.keybinding_hold("shortcuthint/reveal")

    def hide(self):
        logger.debug("Un-revealing shortcut hint with keyboard.")
        self.keybinding_release("shortcuthint/reveal")

    def cancel(self):
        logger.debug("Hide the shortcut hint with keyboard, without releasing the reveal key.")
        self.keybinding("shortcuthint/cancel")

    def get_geometry(self):
        state = self.__get_controller_state()
        x = int(state['x'])
        y = int(state['y'])
        width = int(state['width'])
        height = int(state['height'])
        return (x, y, width, height)

    def get_show_timeout(self):
        state = self.__get_controller_state()
        return float(state['timeout_duration'] / 1000.0)

    def is_enabled(self):
        state = self.__get_controller_state()
        return bool(state['enabled'])

    def is_visible(self):
        state = self.__get_controller_state()
        return bool(state['visible'])

    def __get_controller_state(self):
        return get_state_by_path('/Unity/ShortcutController')[0]
