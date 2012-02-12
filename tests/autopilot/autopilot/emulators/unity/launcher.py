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

from autopilot.emulators.unity import get_state_by_path, make_introspection_object
from autopilot.emulators.unity.icons import BamfLauncherIcon, SimpleLauncherIcon
from autopilot.emulators.X11 import Keyboard, Mouse


logger = logging.getLogger(__name__)


class Launcher(object):
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
        self.grabbed = False
        self._keyboard = Keyboard()
        self._mouse = Mouse()

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
        self._keyboard.press('Super')
        sleep(1)

    def keyboard_unreveal_launcher(self):
        logger.debug("Un-revealing launcher with keyboard.")
        self._keyboard.release('Super')
        sleep(1)

    def grab_switcher(self):
        logger.debug("Initiating launcher keyboard navigation with Alt+F1.")
        self._keyboard.press_and_release('Alt+F1')
        self.grabbed = True

    def switcher_enter_quicklist(self):
        if self.grabbed:
            logger.debug("Opening quicklist for currently selected icon.")
            self._keyboard.press_and_release('Right')

    def switcher_exit_quicklist(self):
        if self.grabbed:
            logger.debug("Closing quicklist for currently selected icon.")
            self._keyboard.press_and_release('Left')

    def start_switcher(self):
        logger.debug("Starting Super+Tab switcher.")
        self._keyboard.press('Super+Tab')
        self._keyboard.release('Tab')
        sleep(1)

    def end_switcher(self, cancel):
        if cancel:
            logger.debug("Cancelling keyboard navigation mode.")
            self._keyboard.press_and_release('Escape')
            if self.grabbed != True:
                self._keyboard.release('Super')
        else:
            logger.debug("Ending keyboard navigation mode.")
            if self.grabbed:
                self._keyboard.press_and_release('\n')
            else:
                self._keyboard.release('Super')
        self.grabbed = False

    def switcher_next(self):
        logger.debug("Selecting next item in keyboard navigation mode.")
        if self.grabbed:
            self._keyboard.press_and_release('Down')
        else:
            self._keyboard.press_and_release('Tab')

    def switcher_prev(self):
        logger.debug("Selecting previous item in keyboard navigation mode.")
        if self.grabbed:
            self._keyboard.press_and_release('Up')
        else:
            self._keyboard.press_and_release('Shift+Tab')

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
