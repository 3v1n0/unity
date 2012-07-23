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

from autopilot.emulators.X11 import Mouse
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


class Switcher(KeybindingsHelper):
    """A class for interacting with the switcher.

    Abstracts out switcher implementation, and makes the necessary functionality available
    to consumer code.

    """

    DIRECTION_FORWARDS = 0
    DIRECTION_BACKWARDS = 1

    def __init__(self):
        super(Switcher, self).__init__()
        self._mouse = Mouse()
        controllers = SwitcherController.get_all_instances()
        assert(len(controllers) == 1)
        self.controller = controllers[0]

    @property
    def visible(self):
        """Is the switcher currently visible

        """
        return self.controller.visible

    @property
    def icons(self):
        """The set of icons in the switcher model

        """
        return self.controller.model.icons

    @property
    def current_icon(self):
        """The currently selected switcher icon"""
        return self.icons[self.selection_index]

    @property
    def selection_index(self):
        """The index of the currently selected icon"""
        return self.controller.model.selection_index

    @property
    def mode(self):
        """Returns the SwitcherMode that the switcher is currently in."""
        if not self.visible:
            return None
        if self.controller.model.detail_selection and not self.controller.model.only_detail_on_viewport:
            return SwitcherMode.DETAIL, SwitcherMode.ALL
        elif self.controller.model.detail_selection:
            return SwitcherMode.DETAIL
        elif not self.controller.model.only_detail_on_viewport:
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
            self.controller.model.only_detail_on_viewport.wait_for(False)

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

        direction must be one of Switcher.DIRECTION_FORWARDS or Switcher.DIRECTION_BACKWARDS.

        The keyword arguments are used to select an icon. For example, you might
        do this to select the 'Show Desktop' icon:

        >>> self.switcher.select_icon(Switcher.DIRECTION_BACKWARDS, tooltip_text="Show Desktop")

        The switcher must be initiated already, and must be in normal mode when
        this method is called, or a RuntimeError will be raised.

        If no icon matches, a ValueError will be raised.

        """
        if self.mode != SwitcherMode.NORMAL:
            raise RuntimeError("Switcher must be initiated in normal mode before calling this method.")

        if direction not in (self.DIRECTION_BACKWARDS, self.DIRECTION_FORWARDS):
            raise ValueError("direction must be one of Switcher.DIRECTION_BACKWARDS, Switcher.DIRECTION_FORWARDS")

        for i in self.controller.model.icons:
            current_icon = self.current_icon
            passed=True
            for key,val in kwargs.iteritems():
                if not hasattr(current_icon, key) or getattr(current_icon, key) != val:
                    passed=False
            if passed:
                return
            if direction == self.DIRECTION_FORWARDS:
                self.next_icon()
            elif direction == self.DIRECTION_BACKWARDS:
                self.previous_icon()
        raise ValueError("No icon found in switcher model that matches: %r" % kwargs)

    def cancel(self):
        """Stop switcher without activating the selected icon and releasing the keys.

        NOTE: Does not release Alt.

        """
        logger.debug("Cancelling switcher.")
        self.keybinding("switcher/cancel")
        self.controller.visible.wait_for(False)

    def terminate(self):
        """Stop switcher without activating the selected icon."""
        logger.debug("Terminating switcher.")
        self.keybinding("switcher/cancel")
        self.keybinding_release("switcher/reveal_normal")
        self.controller.visible.wait_for(False)

    def select(self):
        """Stop switcher and activate the selected icon."""
        logger.debug("Stopping switcher")
        self.keybinding_release("switcher/reveal_normal")
        self.controller.visible.wait_for(False)

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
        return self.controller.model.detail_selection_index

    @property
    def detail_current_count(self):
        """The number of shown details"""
        return self.controller.model.detail_current_count

    def show_details(self):
        """Show detail mode."""
        logger.debug("Showing details view.")
        self.keybinding("switcher/detail_start")
        self.controller.model.detail_selection.wait_for(True)

    def hide_details(self):
        """Hide detail mode."""
        logger.debug("Hiding details view.")
        self.keybinding("switcher/detail_stop")
        self.controller.model.detail_selection.wait_for(False)

    def next_detail(self):
        """Move to next detail in the switcher."""
        logger.debug("Selecting next item in details mode.")
        self.keybinding("switcher/detail_next")

    def previous_detail(self):
        """Move to the previous detail in the switcher."""
        logger.debug("Selecting previous item in details mode.")
        self.keybinding("switcher/detail_prev")


class SwitcherController(UnityIntrospectionObject):
    """An emulator class for interacting with the switcher controller."""

    @property
    def view(self):
        return self.get_children_by_type(SwitcherView)[0]

    @property
    def model(self):
        return self.get_children_by_type(SwitcherModel)[0]


class SwitcherView(UnityIntrospectionObject):
    """An emulator class for interacting with with SwitcherView."""


class SwitcherModel(UnityIntrospectionObject):
    """An emulator class for interacting with the SwitcherModel."""

    @property
    def icons(self):
        return self.get_children_by_type(SimpleLauncherIcon)

