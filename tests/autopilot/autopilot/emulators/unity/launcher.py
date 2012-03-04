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
from autopilot.emulators.unity import UnityIntrospectionObject
from autopilot.emulators.unity.icons import BamfLauncherIcon, SimpleLauncherIcon
from autopilot.emulators.X11 import Mouse, ScreenGeometry


logger = logging.getLogger(__name__)


class LauncherController(UnityIntrospectionObject):
    """The LauncherController class."""

    def get_launcher_for_monitor(self, monitor_num):
        """Return an instnace of Launcher for the specified monitor, or None."""
        launchers = self.get_children_by_type(Launcher, monitor=monitor_num)
        return launchers[0] if launchers else None

    @property
    def model(self):
        """Return the launcher model."""
        models = LauncherModel.get_all_instances()
        assert(len(models) == 1)
        return models[0]

    def key_nav_monitor(self):
        return self.key_nav_launcher_monitor


class Launcher(UnityIntrospectionObject, KeybindingsHelper):
    """An individual launcher for a monitor."""

    def __init__(self, *args, **kwargs):
        super(Launcher, self).__init__(*args, **kwargs)

        self.show_timeout = 1
        self.hide_timeout = 1
        self.in_keynav_mode = False

        self._mouse = Mouse()
        self._screen = ScreenGeometry()

    def move_mouse_to_right_of_launcher(self):
        """Places the mouse to the right of this launcher."""
        self._screen.move_mouse_to_monitor(self.monitor)
        (x, y, w, h) = self.geometry
        target_x = x + w + 10
        target_y = y + h / 2

        logger.debug("Moving mouse away from launcher.")
        self._mouse.move(target_x, target_y, False)
        sleep(self.show_timeout)

    def move_mouse_over_launcher(self):
        """Move the mouse over this launcher."""
        self._screen.move_mouse_to_monitor(self.monitor)
        (x, y, w, h) = self.geometry
        target_x = x + w / 2
        target_y = y + h / 2

        logger.debug("Moving mouse to center of launcher.")
        self._mouse.move(target_x, target_y)

    def reveal_launcher(self):
        """Reveal this launcher with the mouse."""
        self._screen.move_mouse_to_monitor(self.monitor)
        (x, y, w, h) = self.geometry

        logger.debug("Revealing launcher on monitor %d with mouse.", self.monitor)
        self._mouse.move(x - 920, y + h / 2, True, 5, .002)
        sleep(self.show_timeout)

    def keyboard_reveal_launcher(self):
        """Reveal this launcher using the keyboard."""
        self._screen.move_mouse_to_monitor(self.monitor)
        logger.debug("Revealing launcher with keyboard.")
        self.keybinding_hold("launcher/reveal")
        sleep(1)

    def keyboard_unreveal_launcher(self):
        """Un-reveal this launcher using the keyboard."""
        self._screen.move_mouse_to_monitor(self.monitor)
        logger.debug("Un-revealing launcher with keyboard.")
        self.keybinding_release("launcher/reveal")
        sleep(1)

    def grab_switcher(self):
        """Start keyboard navigation mode by pressing Alt+F1."""
        self._screen.move_mouse_to_monitor(self.monitor)
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
        """Start the super+Tab switcher on this launcher."""
        self._screen.move_mouse_to_monitor(self.monitor)
        logger.debug("Starting Super+Tab switcher.")
        self.keybinding_hold("launcher/switcher")
        self.keybinding_tap("launcher/switcher")
        sleep(1)

    def end_switcher(self, cancel):
        """End either the keynav mode or the super+tab swithcer.

        If cancel is True, the currently selected icon will not be activated.

        """
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

    def click_launcher_icon(self, icon, button=1):
        """Move the mouse over the launcher icon, and click it.

        `icon` must be an instance of SimpleLauncherIcon or it's descendants.

        """
        if not isinstance(icon, SimpleLauncherIcon):
            raise TypeError("icon must be a LauncherIcon")
        logger.debug("Clicking launcher icon %r on monitor %d with mouse button %d",
            icon, self.monitor, button)
        self.reveal_launcher()
        target_x = icon.x + self.x
        target_y = icon.y + self.y + (self.icon_size / 2)
        self._mouse.move(target_x, target_y )
        self._mouse.click(button)
        self.move_mouse_to_right_of_launcher()

    def lock_to_launcher(self, icon):
        """lock 'icon' to the launcher, if it's not already.

        `icon` must be an instance of BamfLauncherIcon.

        """
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
        """lock 'icon' to the launcher, if it's not already.

        `icon` must be an instance of BamfLauncherIcon.

        """
        if not isinstance(icon, BamfLauncherIcon):
            raise TypeError("Can only unlock instances of BamfLauncherIcon")
        if not icon.sticky:
            # nothing to do.
            return

        logger.debug("Unlocking icon %r from launcher.")
        self.click_launcher_icon(icon, button=3)
        quicklist = icon.get_quicklist()
        pin_item = quicklist.get_quicklist_item_by_text('Unlock from launcher')
        quicklist.click_item(pin_item)

    def is_quicklist_open(self):
        return self.quicklist_open

    def is_showing(self):
        return not self.hidden

    def are_shortcuts_showing(self):
        return self.shortcuts_shown

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the current launcher."""
        return (self.x, self.y, self.width, self.height)


class LauncherModel(UnityIntrospectionObject):
    """THe launcher model. Contains all launcher icons as children."""

    def get_launcher_icons(self, visible_only=True):
        """Get a list of launcher icons in this launcher."""
        if visible_only:
            return self.get_children_by_type(SimpleLauncherIcon, quirk_visible=True)
        else:
            return self.get_children_by_type(SimpleLauncherIcon)

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

