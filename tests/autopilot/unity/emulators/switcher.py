# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

import logging

from autopilot.input import Mouse
from autopilot.keybindings import KeybindingsHelper

from unity.emulators import UnityIntrospectionObject
# even though we don't use these directly, we need to make sure they've been
# imported so the classes contained are registered with the introspection API.
from unity.emulators.icons import *


logger = logging.getLogger(__name__)

class SwitcherMode():
    """Define the possible modes the switcher can be in"""
    NORMAL = 0
    ALL = 1
    DETAIL = 2


class SwitcherDirection():
    """Directions the switcher can switch in."""
    FORWARDS = 0
    BACKWARDS = 1


class SwitcherController(UnityIntrospectionObject, KeybindingsHelper):
    """A class for interacting with the switcher.

    Abstracts out switcher implementation, and makes the necessary functionality available
    to consumer code.

    """

    def __init__(self, *args, **kwargs):
        super(SwitcherController, self).__init__(*args, **kwargs)
        self._mouse = Mouse.create()

    def get_switcher_view(self):
        views = self.get_children_by_type(SwitcherView)
        return views[0] if views else None

    @property
    def view(self):
        """Returns the SwitcherView."""
        return self.get_switcher_view()

    @property
    def model(self):
        models = self.get_children_by_type(SwitcherModel)
        return models[0] if models else None

    @property
    def icons(self):
        """The set of icons in the switcher model

        """
        return self.model.icons

    @property
    def current_icon(self):
        """The currently selected switcher icon"""
        return self.icons[self.selection_index]

    @property
    def selection_index(self):
        """The index of the currently selected icon"""
        return self.model.selection_index

    @property
    def label(self):
        """The current switcher label"""
        return self.view.label

    @property
    def label_visible(self):
        """The switcher label visibility"""
        return self.view.label_visible

    @property
    def mode(self):
        """Returns the SwitcherMode that the switcher is currently in."""
        if not self.visible:
            return None
        if self.model.detail_selection and not self.model.only_detail_on_viewport:
            return SwitcherMode.DETAIL, SwitcherMode.ALL
        elif self.model.detail_selection:
            return SwitcherMode.DETAIL
        elif not self.model.only_detail_on_viewport:
            return SwitcherMode.ALL
        else:
            return SwitcherMode.NORMAL

    def initiate(self, mode=SwitcherMode.NORMAL):
        """Initiates the switcher in designated mode. Defaults to NORMAL"""
        if mode == SwitcherMode.NORMAL:
            logger.debug("Initiating switcher with Alt+Tab")
            self.keybinding_hold_part_then_tap("switcher/reveal_normal")
            self.visible.wait_for(True)
        elif mode == SwitcherMode.DETAIL:
            logger.debug("Initiating switcher detail mode with Alt+`")
            self.keybinding_hold_part_then_tap("switcher/reveal_details")
            self.visible.wait_for(True)
        elif mode == SwitcherMode.ALL:
            logger.debug("Initiating switcher in 'all workspaces' mode. Ctrl+Alt+Tab")
            self.keybinding_hold_part_then_tap("switcher/reveal_all")
            self.model.only_detail_on_viewport.wait_for(False)

    def next_icon(self):
        """Move to the next icon."""
        logger.debug("Selecting next item in switcher.")
        self.keybinding("switcher/next")

    def previous_icon(self):
        """Move to the previous icon."""
        logger.debug("Selecting previous item in switcher.")
        self.keybinding("switcher/prev")

    def select_icon(self, direction, **kwargs):
        """Select an icon in the switcher.

        direction must be one of SwitcherDirection.FORWARDS or SwitcherDirection.BACKWARDS.

        The keyword arguments are used to select an icon. For example, you might
        do this to select the 'Show Desktop' icon:

        >>> self.switcher.select_icon(SwitcherDirection.BACKWARDS, tooltip_text="Show Desktop")

        The switcher must be initiated already, and must be in normal mode when
        this method is called, or a RuntimeError will be raised.

        If no icon matches, a ValueError will be raised.

        """
        if self.mode == SwitcherMode.DETAIL:
            raise RuntimeError("Switcher must be initiated in normal mode before calling this method.")

        if direction not in (SwitcherDirection.BACKWARDS, SwitcherDirection.FORWARDS):
            raise ValueError("direction must be one of SwitcherDirection.BACKWARDS, SwitcherDirection.FORWARDS")

        for i in self.model.icons:
            current_icon = self.current_icon
            passed=True
            for key,val in kwargs.iteritems():
                if not hasattr(current_icon, key) or getattr(current_icon, key) != val:
                    passed=False
            if passed:
                return
            if direction == SwitcherDirection.FORWARDS:
                self.next_icon()
            elif direction == SwitcherDirection.BACKWARDS:
                self.previous_icon()
        raise ValueError("No icon found in switcher model that matches: %r" % kwargs)

    def cancel(self):
        """Stop switcher without activating the selected icon and releasing the keys.

        NOTE: Does not release Alt.

        """
        logger.debug("Cancelling switcher.")
        self.keybinding("switcher/cancel")
        self.visible.wait_for(False)

    def terminate(self):
        """Stop switcher without activating the selected icon."""
        logger.debug("Terminating switcher.")
        self.keybinding("switcher/cancel")
        self.keybinding_release("switcher/reveal_normal")
        self.visible.wait_for(False)

    def select(self):
        """Stop switcher and activate the selected icon."""
        logger.debug("Stopping switcher")
        self.keybinding_release("switcher/reveal_normal")
        self.visible.wait_for(False)

    def next_via_mouse(self):
        """Move to the next icon using the mouse scroll wheel."""
        logger.debug("Selecting next item in switcher with mouse scroll wheel.")
        self._mouse.press(6)
        self._mouse.release(6)

    def previous_via_mouse(self):
        """Move to the previous icon using the mouse scroll wheel."""
        logger.debug("Selecting previous item in switcher with mouse scroll wheel.")
        self._mouse.press(7)
        self._mouse.release(7)

    @property
    def detail_selection_index(self):
        """The index of the currently selected detail"""
        return self.model.detail_selection_index

    @property
    def detail_current_count(self):
        """The number of shown details"""
        return self.model.detail_current_count

    def show_details(self):
        """Show detail mode."""
        logger.debug("Showing details view.")
        self.keybinding("switcher/detail_start")
        self.model.detail_selection.wait_for(True)

    def hide_details(self):
        """Hide detail mode."""
        logger.debug("Hiding details view.")
        self.keybinding("switcher/detail_stop")
        self.model.detail_selection.wait_for(False)

    def next_detail(self):
        """Move to next detail in the switcher."""
        logger.debug("Selecting next item in details mode.")
        self.keybinding("switcher/detail_next")

    def previous_detail(self):
        """Move to the previous detail in the switcher."""
        logger.debug("Selecting previous item in details mode.")
        self.keybinding("switcher/detail_prev")


class SwitcherView(UnityIntrospectionObject):
    """An emulator class for interacting with with SwitcherView."""

    def __init__(self, *args, **kwargs):
        super(SwitcherView, self).__init__(*args, **kwargs)
        self._mouse = Mouse.create()

    @property
    def icon_args(self):
        return self.get_children_by_type(RenderArgs);

    @property
    def detail_icons(self):
        return self.get_children_by_type(LayoutWindow);

    def move_over_icon(self, index):
        offset = self.spread_offset
        icon_arg = self.icon_args[index]

        x = icon_arg.logical_center_x + offset
        y = icon_arg.logical_center_y + offset

        self._mouse.move(x,y)

    def move_over_detail_icon(self, index):
        offset = self.spread_offset
        x = self.detail_icons[index].x + offset
        y = self.detail_icons[index].y + offset

        self._mouse.move(x,y)

    def break_mouse_bump_detection(self):
        """ Only break mouse detection if the switcher is open.
            Move the mouse back to the orginal position
        """

        old_x = self._mouse.x
        old_y = self._mouse.y

        self.move_over_icon(0)

        x = self._mouse.x
        y = self._mouse.y
        self._mouse.move(x + 5, y + 5)

        self._mouse.move(old_x, old_y)


class RenderArgs(UnityIntrospectionObject):
  """An emulator class for interacting with the RenderArgs class."""

class LayoutWindow(UnityIntrospectionObject):
  """An emulator class for interacting with the LayoutWindows class."""

class SwitcherModel(UnityIntrospectionObject):
    """An emulator class for interacting with the SwitcherModel."""

    @property
    def icons(self):
        return self.get_children_by_type(SimpleLauncherIcon)
