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
from autopilot.emulators.unity.icons import BamfLauncherIcon, SimpleLauncherIcon
from autopilot.emulators.X11 import Keyboard, Mouse, ScreenGeometry


logger = logging.getLogger(__name__)


class Launcher(KeybindingsHelper):
    """Interact with the unity Launcher."""

    def __init__(self):
        super(Launcher, self).__init__()
        # set up base launcher vars
        self.x = 0
        self.y = 120
        self.width = 0
        self.height = 0
        self.show_timeout = 1
        self.hide_timeout = 1
        self.in_keynav_mode = False
        self._keyboard = Keyboard()
        self._mouse = Mouse()
        self._screen = ScreenGeometry()

        state = self.__get_state(0)
        self.icon_width = int(state['icon-size'])

    def move_mouse_to_right_of_launcher(self, monitor):
        (x, y, w, h) = self.launcher_geometry(monitor)
        target_x = x + w + 10
        target_y = y + h / 2

        logger.debug("Moving mouse away from launcher.")
        self._mouse.move(target_x, target_y, False)
        sleep(self.show_timeout)

    def move_mouse_over_launcher(self, monitor):
        (x, y, w, h) = self.launcher_geometry(monitor)
        self._screen.move_mouse_to_monitor(monitor)
        target_x = x + w / 2
        target_y = y + h / 2
        logger.debug("Moving mouse to center of launcher.")
        self._mouse.move(target_x, target_y)

    def reveal_launcher(self, monitor):
        (x, y, w, h) = self.launcher_geometry(monitor)
        logger.debug("Revealing launcher on monitor %d with mouse.", monitor)
        self._mouse.move(x - 920, y + h / 2, True, 5, .002)
        sleep(self.show_timeout)

    def keyboard_reveal_launcher(self):
        logger.debug("Revealing launcher with keyboard.")
        self.keybinding_hold("launcher/reveal")
        sleep(1)

    def keyboard_unreveal_launcher(self):
        logger.debug("Un-revealing launcher with keyboard.")
        self.keybinding_release("launcher/reveal")
        sleep(1)

    def grab_switcher(self):
        logger.debug("Initiating launcher keyboard navigation with Alt+F1.")
        self.keybinding("launcher/keynav")
        self.in_keynav_mode = True

    def switcher_enter_quicklist(self):
        if not self.in_keynav_mode:
            raise RuntimeError("Cannot open switcher quicklist while not in keynav mode.")
        logger.debug("Opening quicklist for currently selected icon.")
        self.keybinding("launcher/keynav/open-quicklist")

    def switcher_exit_quicklist(self):
        if not self.in_keynav_mode:
            raise RuntimeError("Cannot close switcher quicklist while not in keynav mode.")
        logger.debug("Closing quicklist for currently selected icon.")
        self.keybinding("launcher/keynav/close-quicklist")

    def start_switcher(self):
        logger.debug("Starting Super+Tab switcher.")
        self.keybinding_hold("launcher/switcher")
        self.keybinding_tap("launcher/switcher")
        sleep(1)

    def end_switcher(self, cancel):
        if cancel:
            logger.debug("Cancelling keyboard navigation mode.")
            self.keybinding("launcher/keynav/exit")
            if not self.in_keynav_mode:
                self.keybinding_release("launcher/switcher")
        else:
            logger.debug("Ending keyboard navigation mode.")
            if self.in_keynav_mode:
                self.keybinding("launcher/keynav/activate")
            else:
                self.keybinding_release("launcher/switcher")
        self.in_keynav_mode = False

    def switcher_next(self):
        logger.debug("Selecting next item in keyboard navigation mode.")
        if self.in_keynav_mode:
            self.keybinding("launcher/keynav/next")
        else:
            self.keybinding("launcher/switcher/next")

    def switcher_prev(self):
        logger.debug("Selecting previous item in keyboard navigation mode.")
        if self.in_keynav_mode:
            self.keybinding("launcher/keynav/prev")
        else:
            self.keybinding("launcher/switcher/prev")

    def is_quicklist_open(self, monitor):
        state = self.__get_state(monitor)
        return bool(state['quicklist-open'])

    def is_showing(self, monitor):
        state = self.__get_state(monitor)
        return not bool(state['hidden'])

    def key_nav_is_active(self):
        state = self.__get_controller_state()
        return bool(state['key_nav_is_active'])

    def key_nav_monitor(self):
        state = self.__get_controller_state()
        return int(state['key_nav_launcher_monitor'])

    def key_nav_is_grabbed(self):
        state = self.__get_controller_state()
        return bool(state['key_nav_is_grabbed'])

    def key_nav_selection(self):
        state = self.__get_controller_state()
        return int(state['key_nav_selection'])

    def launcher_geometry(self, monitor):
        state = self.__get_state(monitor)
        x = int(state['x'])
        y = int(state['y'])
        width = int(state['width'])
        height = int(state['height'])
        return (x, y, width, height)

    def num_launchers(self):
        return len(get_state_by_path('/Unity/LauncherController/Launcher'))

    def __get_controller_state(self):
        return get_state_by_path('/Unity/LauncherController')[0]

    def __get_model_state(self):
        return get_state_by_path('/Unity/LauncherController/LauncherModel')[0]

    def __get_state(self, monitor):
        return get_state_by_path('/Unity/LauncherController/Launcher[monitor=%s]' % (monitor))[0]

    def get_launcher_icons(self, visible_only=True):
        """Get a list of launcher icons in this launcher."""
        model = self.__get_model_state()
        icons = []
        for child in model['Children']:
            icon = make_introspection_object(child)
            if icon:
                if visible_only and not getattr(icon, 'quirk_visible', False):
                    continue
                icons.append(icon)
        return icons

    def get_icon_by_tooltip_text(self, tooltip_text):
        """Get a launcher icon given it's tooltip text.

        Returns None if there is no icon with the specified text.
        """
        for icon in self.get_launcher_icons():
            if icon.tooltip_text == tooltip_text:
                return icon
        return None

    def num_launcher_icons(self):
        """Get the number of icons in the launcher model."""
        return len(self.get_launcher_icons())

    def get_currently_selected_icon(self):
        """Returns the currently selected launcher icon, if keynav mode is active."""
        return self.get_launcher_icons()[self.key_nav_selection()]

    def click_launcher_icon(self, icon, monitor=0, button=1):
        """Move the mouse over the launcher icon, and click it."""
        if not isinstance(icon, SimpleLauncherIcon):
            raise TypeError("icon must be a LauncherIcon")
        logger.debug("Clicking launcher icon %r on monitor %d with mouse button %d",
            icon, monitor, button)
        self.reveal_launcher(monitor)
        self._mouse.move(icon.x, icon.y + (self.icon_width / 2))
        self._mouse.click(button)
        self.move_mouse_to_right_of_launcher(monitor)

    def lock_to_launcher(self, icon):
        """lock 'icon' to the launcher, if it's not already."""
        if not isinstance(icon, BamfLauncherIcon):
            raise TypeError("Can only lock instances of BamfLauncherIcon")
        if icon.sticky:
            # Nothing to do.
            return

        logger.debug("Locking icon %r to launcher.", icon)
        self.click_launcher_icon(icon, button=3)
        quicklist = icon.get_quicklist()
        pin_item = quicklist.get_quicklist_item_by_text('Lock to launcher')
        quicklist.click_item(pin_item)

    def unlock_from_launcher(self, icon):
        """lock 'icon' to the launcher, if it's not already."""
        if not isinstance(icon, SimpleLauncherIcon):
            raise TypeError("icon must be a LauncherIcon")
        if not icon.sticky:
            # nothing to do.
            return

        logger.debug("Unlocking icon %r from launcher.")
        self.click_launcher_icon(icon, button=3)
        quicklist = icon.get_quicklist()
        pin_item = quicklist.get_quicklist_item_by_text('Unlock from launcher')
        quicklist.click_item(pin_item)
