# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

from autopilot.input import Mouse
from autopilot.display import Display, move_mouse_to_screen
from autopilot.keybindings import KeybindingsHelper
import logging
from testtools.matchers import NotEquals
from time import sleep

from unity.emulators import UnityIntrospectionObject
from unity.emulators.icons import (
    ApplicationLauncherIcon,
    BFBLauncherIcon,
    ExpoLauncherIcon,
    SimpleLauncherIcon,
    TrashLauncherIcon,
    )

from unity.emulators.compiz import get_compiz_option

logger = logging.getLogger(__name__)


class IconDragType:
    """Define possible positions to drag an icon onto another"""
    INSIDE = 0
    OUTSIDE = 1
    BEFORE = 3
    AFTER = 4


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
        models = self.get_children_by_type(LauncherModel)
        assert(len(models) == 1)
        return models[0]


class Launcher(UnityIntrospectionObject, KeybindingsHelper):
    """An individual launcher for a monitor."""

    def __init__(self, *args, **kwargs):
        super(Launcher, self).__init__(*args, **kwargs)

        self.show_timeout = 1
        self.hide_timeout = 1
        self.in_keynav_mode = False
        self.in_switcher_mode = False

        self._mouse = Mouse.create()
        self._display = Display.create()

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
        # If we're doing a normal activation, all we need to do is release the
        # keybinding. Otherwise, perform the keybinding specified *then* release
        # the switcher keybinding.
        if keybinding != "launcher/switcher":
            self._perform_switcher_binding(keybinding)
        self.keybinding_release("launcher/switcher")
        self.in_switcher_mode = False

    def _get_controller(self):
        """Get the launcher controller."""
        controller = self.get_root_instance().select_single(LauncherController)
        return controller

    def move_mouse_to_right_of_launcher(self):
        """Places the mouse to the right of this launcher."""
        move_mouse_to_screen(self.monitor)
        (x, y, w, h) = self.geometry
        target_x = x + w + 10
        target_y = y + h / 2

        logger.debug("Moving mouse away from launcher.")
        self._mouse.move(target_x, target_y, False)
        sleep(self.show_timeout)

    def move_mouse_over_launcher(self):
        """Move the mouse over this launcher."""
        move_mouse_to_screen(self.monitor)
        (x, y, w, h) = self.geometry
        target_x = x + w / 2
        target_y = y + h / 2

        logger.debug("Moving mouse to center of launcher.")
        self._mouse.move(target_x, target_y)

    def move_mouse_to_icon(self, icon, autoscroll_offset=0):
        """Move the mouse to a specific icon."""
        (x, y, w, h) = self.geometry
        found = False

        # Only try 10 times (5 secs.) before giving up.
        for i in xrange(0, 10):
            mouse_x = target_x = icon.center_x + self.x
            mouse_y = target_y = icon.center_y
            if target_y > h:
                mouse_y = h + y - autoscroll_offset
            elif target_y < 0:
                mouse_y = y + autoscroll_offset
            if self._mouse.x == target_x and self._mouse.y == target_y:
                found = True
                break
            self._mouse.move(mouse_x, mouse_y)
            sleep(0.5)

        if not found:
            raise RuntimeError("Could not move mouse to the icon")

    def mouse_reveal_launcher(self):
        """Reveal this launcher with the mouse.

        If the launcher is already visible calling this method does nothing.
        """
        if self.is_showing:
            return
        move_mouse_to_screen(self.monitor)
        (x, y, w, h) = self.geometry

        target_x = x - 920 # this is the pressure we need to reveal the launcher.
        target_y = y + h / 2
        logger.debug("Revealing launcher on monitor %d with mouse.", self.monitor)
        self._mouse.move(target_x, target_y, True, 5, .002)
        sleep(self.show_timeout)

    def keyboard_reveal_launcher(self):
        """Reveal this launcher using the keyboard."""
        move_mouse_to_screen(self.monitor)
        logger.debug("Revealing launcher with keyboard.")
        self.keybinding_hold("launcher/reveal")
        self.is_showing.wait_for(True)

    def keyboard_unreveal_launcher(self):
        """Un-reveal this launcher using the keyboard."""
        move_mouse_to_screen(self.monitor)
        logger.debug("Un-revealing launcher with keyboard.")
        self.keybinding_release("launcher/reveal")
        # only wait if the launcher is set to autohide
        if self.hidemode == 1:
            self.is_showing.wait_for(False)

    def keyboard_select_icon(self, **kwargs):
        """Using either keynav mode or the switcher, select an icon in the launcher.

        The desired mode (keynav or switcher) must be started already before
        calling this methods or a RuntimeError will be raised.

        This method won't activate the icon, it will only select it.

        Icons are selected by passing keyword argument filters to this method.
        For example:

        >>> launcher.keyboard_select_icon(tooltip_text="Calculator")

        ...will select the *first* icon that has a 'tooltip_text' attribute equal
        to 'Calculator'. If an icon is missing the attribute, it is treated as
        not matching.

        If no icon is found, this method will raise a ValueError.

        """

        if not self.in_keynav_mode and not self.in_switcher_mode:
            raise RuntimeError("Launcher must be in keynav or switcher mode")

        launcher_model = self.get_root_instance().select_single(LauncherModel)
        all_icons = launcher_model.get_launcher_icons()
        logger.debug("all_icons = %r", [i.tooltip_text for i in all_icons])
        for icon in all_icons:
            # can't iterate over the model icons directly since some are hidden
            # from the user.
            if not icon.visible:
                continue
            logger.debug("Selected icon = %s", icon.tooltip_text)
            matches = True
            for arg,val in kwargs.iteritems():
                if not hasattr(icon, arg) or getattr(icon, arg, None) != val:
                    matches = False
                    break
            if matches:
                return
            if self.in_keynav_mode:
                self.key_nav_next()
            elif self.in_switcher_mode:
                self.switcher_next()
        raise ValueError("No icon found that matches: %r", kwargs)

    def key_nav_start(self):
        """Start keyboard navigation mode by pressing Alt+F1."""
        move_mouse_to_screen(self.monitor)
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
        move_mouse_to_screen(self.monitor)
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

    def click_launcher_icon(self, icon, button=1, move_mouse_after=True):
        """Move the mouse over the launcher icon, and click it.
        `icon` must be an instance of SimpleLauncherIcon or it's descendants.
        `move_mouse_after` moves the mouse outside the launcher if true.
        """
        if not isinstance(icon, SimpleLauncherIcon):
            raise TypeError("icon must be a LauncherIcon, not %s" % type(icon))

        logger.debug("Clicking launcher icon %r on monitor %d with mouse button %d",
            icon, self.monitor, button)

        self.mouse_reveal_launcher()
        self.move_mouse_to_icon(icon)
        self._mouse.click(button)

        if (move_mouse_after):
          self.move_mouse_to_right_of_launcher()

    def drag_icon_to_position(self, icon, pos, target, drag_type=IconDragType.INSIDE):
        """Drag a launcher icon to a new position.

        'icon' is the icon to move. It must be either a ApplicationLauncherIcon or an
        ExpoLauncherIcon instance. Other values will result in a TypeError being
        raised.

        'pos' must be one of IconDragType.BEFORE or IconDragType.AFTER. If it is
        not one of these, a ValueError will be raised.

        'target' is the target icon.

        'drag_type' must be one of IconDragType.INSIDE or IconDragType.OUTSIDE.
        This specifies whether the icon is gragged inside the launcher, or to the
        right of it. The default is to drag inside the launcher. If it is
        specified, and not one of the allowed values, a ValueError will be raised.

        For example:

        >>> drag_icon_to_position(calc_icon, IconDragType.BEFORE, switcher_icon)

        This will drag the calculator icon to just before the switcher icon.

        Note: This method makes no attempt to sanity-check the requested move.
        For example, it will happily try and move an icon to before the BFB icon,
        if asked.

        """
        if not isinstance(icon, ApplicationLauncherIcon) \
            and not isinstance(icon, ExpoLauncherIcon):
            raise TypeError("Icon to move must be a ApplicationLauncherIcon or ExpoLauncherIcon, not %s"
                % type(icon).__name__)

        if pos not in (IconDragType.BEFORE, IconDragType.AFTER):
            raise ValueError("'pos' parameter must be one of IconDragType.BEFORE, IconDragType.AFTER")

        if not isinstance(target, SimpleLauncherIcon):
            raise TypeError("'target' must be a valid launcher icon, not %s"
                % type(target).__name__)

        if drag_type not in (IconDragType.INSIDE, IconDragType.OUTSIDE):
            raise ValueError("'drag_type' parameter must be one of IconDragType.INSIDE, IconDragType.OUTSIDE")

        icon_height = get_compiz_option("unityshell", "icon_size")
        target_y = target.center_y
        if target_y < icon.center_y:
            target_y += icon_height / 2
        if pos == IconDragType.BEFORE:
            target_y -= icon_height + (icon_height / 2)

        self.move_mouse_to_icon(icon)
        self._mouse.press()
        sleep(2)

        if drag_type == IconDragType.OUTSIDE:
            shift_over = self._mouse.x + (icon_height * 2)
            self._mouse.move(shift_over, self._mouse.y)
            sleep(0.5)

        self.move_mouse_to_icon(target)
        self._mouse.move(self._mouse.x, target_y)
        sleep(0.5)
        self._mouse.release()
        self.move_mouse_to_right_of_launcher()

    def lock_to_launcher(self, icon):
        """lock 'icon' to the launcher, if it's not already.
        `icon` must be an instance of ApplicationLauncherIcon.
        """
        if not isinstance(icon, ApplicationLauncherIcon):
            raise TypeError("Can only lock instances of ApplicationLauncherIcon")
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

        `icon` must be an instance of ApplicationLauncherIcon.

        """
        if not isinstance(icon, ApplicationLauncherIcon):
            raise TypeError("Can only unlock instances of ApplicationLauncherIcon, not %s" % type(icon).__name__)
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
    """The launcher model. Contains all launcher icons as children."""

    def get_bfb_icon(self):
        icons = self.get_root_instance().select_many(BFBLauncherIcon)
        assert(len(icons) == 1)
        return icons[0]

    def get_expo_icon(self):
        icons = self.get_children_by_type(ExpoLauncherIcon)
        assert(len(icons) == 1)
        return icons[0]

    def get_trash_icon(self):
        icons = self.get_children_by_type(TrashLauncherIcon)
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
            return self.get_children_by_type(ApplicationLauncherIcon, visible=True)
        else:
            return self.get_children_by_type(ApplicationLauncherIcon)

    def get_launcher_icons_for_monitor(self, monitor, visible_only=True):
        """Get a list of launcher icons for provided monitor."""
        icons = []
        for icon in self.get_launcher_icons(visible_only):
            if icon.is_on_monitor(monitor):
                icons.append(icon)

        return icons

    def get_icon(self, **kwargs):
        """Get a launcher icon from the model according to some filters.

        This method accepts keyword argument that are the filters to use when
        looking for an icon. For example, to find an icon with a particular
        desktop_id, one might do this from within a test:

        >>> self.launcher.model.get_icon(desktop_id="gcalctool.desktop")

        This method returns only one icon. It is the callers responsibility to
        ensure that the filter matches only one icon.

        This method will attempt to get the launcher icon, and will retry several
        times, so the caller can be assured that if this method doesn't find
        the icon it really does not exist.

        If no keyword arguments are specified, ValueError will be raised.

        If no icons are matched, None is returned.

        """

        if not kwargs:
            raise ValueError("You must specify at least one keyword argument to ths method.")

        for i in range(10):
            icons = self.get_children_by_type(SimpleLauncherIcon, **kwargs)
            if len(icons) > 1:
                logger.warning("Got more than one icon returned using filters=%r. Returning first one", kwargs)
            if icons:
                return icons[0]
            sleep(1)
        return None

    def num_launcher_icons(self):
        """Get the number of icons in the launcher model."""
        return len(self.get_launcher_icons())

    def num_bamf_launcher_icons(self, visible_only=True):
        """Get the number of bamf icons in the launcher model."""
        return len(self.get_bamf_launcher_icons(visible_only))
