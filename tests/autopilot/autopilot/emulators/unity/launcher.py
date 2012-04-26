# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

import dbus
import logging
from testtools.matchers import NotEquals
from time import sleep

from autopilot.emulators.dbus_handler import session_bus
from autopilot.emulators.unity import UnityIntrospectionObject
from autopilot.emulators.unity.icons import BFBLauncherIcon, BamfLauncherIcon, SimpleLauncherIcon
from autopilot.emulators.X11 import Mouse, ScreenGeometry
from autopilot.keybindings import KeybindingsHelper


logger = logging.getLogger(__name__)


class LauncherController(UnityIntrospectionObject):
    """The LauncherController class."""

    def get_launcher_for_monitor(self, monitor_num):
        """Return an instance of Launcher for the specified monitor, or None."""
        launchers = self.get_children_by_type(Launcher, monitor=monitor_num)
        return launchers[0] if launchers else None

    def get_launchers(self):
        """Return the available launchers, or None."""
        return self.get_children_by_type(Launcher)

    @property
    def model(self):
        """Return the launcher model."""
        models = LauncherModel.get_all_instances()
        assert(len(models) == 1)
        return models[0]

    def add_launcher_item_from_position(self,name,icon,icon_x,icon_y,icon_size,desktop_file,aptdaemon_task):
        """ Emulate a DBus call from Software Center to pin an icon to the launcher """
        launcher_object = session_bus.get_object('com.canonical.Unity.Launcher',
                                      '/com/canonical/Unity/Launcher')
        launcher_iface = dbus.Interface(launcher_object, 'com.canonical.Unity.Launcher')
        launcher_iface.AddLauncherItemFromPosition(name,
                                                   icon,
                                                   icon_x,
                                                   icon_y,
                                                   icon_size,
                                                   desktop_file,
                                                   aptdaemon_task)


class Launcher(UnityIntrospectionObject, KeybindingsHelper):
    """An individual launcher for a monitor."""

    def __init__(self, *args, **kwargs):
        super(Launcher, self).__init__(*args, **kwargs)

        self.show_timeout = 1
        self.hide_timeout = 1
        self.in_keynav_mode = False
        self.in_switcher_mode = False

        self._mouse = Mouse()
        self._screen = ScreenGeometry()

    def _perform_key_nav_binding(self, keybinding):
        if not self.in_keynav_mode:
                raise RuntimeError("Cannot perform key navigation when not in kaynav mode.")
        self.keybinding(keybinding)

    def _perform_key_nav_exit_binding(self, keybinding):
        self._perform_key_nav_binding(keybinding)
        self.in_keynav_mode = False

    def _perform_switcher_binding(self, keybinding):
        if not self.in_switcher_mode:
            raise RuntimeError("Cannot interact with launcher switcher when not in switcher mode.")
        self.keybinding(keybinding)

    def _perform_switcher_exit_binding(self, keybinding):
        self._perform_switcher_binding(keybinding)
        # if our exit binding was something besides just releasing, we need to release
        if keybinding != "launcher/switcher":
            self.keybinding_release("launcher/switcher")
        self.in_switcher_mode = False

    def _get_controller(self):
        """Get the launcher controller."""
        [controller] = LauncherController.get_all_instances()
        return controller

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

    def mouse_reveal_launcher(self):
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
        self.is_showing.wait_for(True)

    def keyboard_unreveal_launcher(self):
        """Un-reveal this launcher using the keyboard."""
        self._screen.move_mouse_to_monitor(self.monitor)
        logger.debug("Un-revealing launcher with keyboard.")
        self.keybinding_release("launcher/reveal")
        # only wait if the launcher is set to autohide
        if self.hidemode == 1:
            self.is_showing.wait_for(False)

    def key_nav_start(self):
        """Start keyboard navigation mode by pressing Alt+F1."""
        self._screen.move_mouse_to_monitor(self.monitor)
        logger.debug("Initiating launcher keyboard navigation with Alt+F1.")
        self.keybinding("launcher/keynav")
        self._get_controller().key_nav_is_active.wait_for(True)
        self.in_keynav_mode = True

    def key_nav_cancel(self):
        """End the key navigation."""
        logger.debug("Cancelling keyboard navigation mode.")
        self._perform_key_nav_exit_binding("launcher/keynav/exit")
        self._get_controller().key_nav_is_active.wait_for(False)

    def key_nav_activate(self):
        """Activates the selected launcher icon. In the current implementation
        this also exits key navigation"""
        logger.debug("Ending keyboard navigation mode, activating icon.")
        self._perform_key_nav_exit_binding("launcher/keynav/activate")
        self._get_controller().key_nav_is_active.wait_for(False)

    def key_nav_next(self):
        """Moves the launcher keynav focus to the next launcher icon"""
        logger.debug("Selecting next item in keyboard navigation mode.")
        old_selection = self._get_controller().key_nav_selection
        self._perform_key_nav_binding("launcher/keynav/next")
        self._get_controller().key_nav_selection.wait_for(NotEquals(old_selection))

    def key_nav_prev(self):
        """Moves the launcher keynav focus to the previous launcher icon"""
        logger.debug("Selecting previous item in keyboard navigation mode.")
        old_selection = self._get_controller().key_nav_selection
        self._perform_key_nav_binding("launcher/keynav/prev")
        self._get_controller().key_nav_selection.wait_for(NotEquals(old_selection))

    def key_nav_enter_quicklist(self):
        logger.debug("Opening quicklist for currently selected icon.")
        self._perform_key_nav_binding("launcher/keynav/open-quicklist")
        self.quicklist_open.wait_for(True)

    def key_nav_exit_quicklist(self):
        logger.debug("Closing quicklist for currently selected icon.")
        self._perform_key_nav_binding("launcher/keynav/close-quicklist")
        self.quicklist_open.wait_for(False)

    def switcher_start(self):
        """Start the super+Tab switcher on this launcher."""
        self._screen.move_mouse_to_monitor(self.monitor)
        logger.debug("Starting Super+Tab switcher.")
        self.keybinding_hold_part_then_tap("launcher/switcher")
        self._get_controller().key_nav_is_active.wait_for(True)
        self.in_switcher_mode = True

    def switcher_cancel(self):
        """End the super+tab swithcer."""
        logger.debug("Cancelling keyboard navigation mode.")
        self._perform_switcher_exit_binding("launcher/switcher/exit")
        self._get_controller().key_nav_is_active.wait_for(False)

    def switcher_activate(self):
        """Activates the selected launcher icon. In the current implementation
        this also exits the switcher"""
        logger.debug("Ending keyboard navigation mode.")
        self._perform_switcher_exit_binding("launcher/switcher")
        self._get_controller().key_nav_is_active.wait_for(False)

    def switcher_next(self):
        logger.debug("Selecting next item in keyboard navigation mode.")
        old_selection = self._get_controller().key_nav_selection
        self._perform_switcher_binding("launcher/switcher/next")
        self._get_controller().key_nav_selection.wait_for(NotEquals(old_selection))

    def switcher_prev(self):
        logger.debug("Selecting previous item in keyboard navigation mode.")
        old_selection = self._get_controller().key_nav_selection
        self._perform_switcher_binding("launcher/switcher/prev")
        self._get_controller().key_nav_selection.wait_for(NotEquals(old_selection))

    def switcher_up(self):
        logger.debug("Selecting next item in keyboard navigation mode.")
        old_selection = self._get_controller().key_nav_selection
        self._perform_switcher_binding("launcher/switcher/up")
        self._get_controller().key_nav_selection.wait_for(NotEquals(old_selection))

    def switcher_down(self):
        logger.debug("Selecting previous item in keyboard navigation mode.")
        old_selection = self._get_controller().key_nav_selection
        self._perform_switcher_binding("launcher/switcher/down")
        self._get_controller().key_nav_selection.wait_for(NotEquals(old_selection))

    def click_launcher_icon(self, icon, button=1):
        """Move the mouse over the launcher icon, and click it.
        `icon` must be an instance of SimpleLauncherIcon or it's descendants.
        """
        if not isinstance(icon, SimpleLauncherIcon):
            raise TypeError("icon must be a LauncherIcon")
        logger.debug("Clicking launcher icon %r on monitor %d with mouse button %d",
            icon, self.monitor, button)
        self.mouse_reveal_launcher()
        target_x = icon.center_x + self.x
        target_y = icon.center_y
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
        pin_item = quicklist.get_quicklist_item_by_text('Lock to Launcher')
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
        pin_item = quicklist.get_quicklist_item_by_text('Unlock from Launcher')
        quicklist.click_item(pin_item)

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the current launcher."""
        return (self.x, self.y, self.width, self.height)


class LauncherModel(UnityIntrospectionObject):
    """THe launcher model. Contains all launcher icons as children."""

    def get_bfb_icon(self):
        icons = BFBLauncherIcon.get_all_instances()
        assert(len(icons) == 1)
        return icons[0]

    def get_launcher_icons(self, visible_only=True):
        """Get a list of launcher icons in this launcher."""
        if visible_only:
            return self.get_children_by_type(SimpleLauncherIcon, visible=True)
        else:
            return self.get_children_by_type(SimpleLauncherIcon)

    def get_bamf_launcher_icons(self, visible_only=True):
        """Get a list of bamf launcher icons in this launcher."""
        if visible_only:
            return self.get_children_by_type(BamfLauncherIcon, visible=True)
        else:
            return self.get_children_by_type(BamfLauncherIcon)

    def get_launcher_icons_for_monitor(self, monitor, visible_only=True):
        """Get a list of launcher icons for provided monitor."""
        icons = []
        for icon in self.get_launcher_icons(visible_only):
            if icon.is_on_monitor(monitor):
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

    def get_icon_by_desktop_id(self, desktop_id):
        """Gets a launcher icon with the specified desktop id.

        Returns None if there is no such launcher icon.
        """
        icons = self.get_children_by_type(SimpleLauncherIcon, desktop_id=desktop_id)
        if len(icons):
            return icons[0]

        return None

    def get_icons_by_filter(self, **kwargs):
        """Get a list of icons that satisfy the given filters.

        For example:

        >>> get_icons_by_filter(tooltip_text="My Application")
        ... [...]

        Returns an empty list if no icons matched the filter.

        """
        return self.get_children_by_type(SimpleLauncherIcon, **kwargs)

    def num_launcher_icons(self):
        """Get the number of icons in the launcher model."""
        return len(self.get_launcher_icons())

    def num_bamf_launcher_icons(self, visible_only=True):
        """Get the number of bamf icons in the launcher model."""
        return len(self.get_bamf_launcher_icons(visible_only))
