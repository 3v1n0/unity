# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

import logging
from time import sleep

from autopilot.input import Mouse
from autopilot.keybindings import KeybindingsHelper

from unity.emulators import UnityIntrospectionObject
logger = logging.getLogger(__name__)


class PanelController(UnityIntrospectionObject):
    """The PanelController class."""

    def get_panel_for_monitor(self, monitor_num):
        """Return an instance of panel for the specified monitor, or None."""
        panels = self.get_children_by_type(UnityPanel, monitor=monitor_num)
        assert(len(panels) == 1)
        return panels[0]

    def get_active_panel(self):
        """Return the active panel, or None."""
        panels = self.get_children_by_type(UnityPanel, active=True)
        assert(len(panels) == 1)
        return panels[0]

    def get_active_indicator(self):
        for panel in self.get_panels:
            active = panel.get_active_indicator()
            if active:
                return active

        return None

    @property
    def get_panels(self):
        """Return the available panels, or None."""
        return self.get_children_by_type(UnityPanel)


class UnityPanel(UnityIntrospectionObject, KeybindingsHelper):
    """An individual panel for a monitor."""

    def __init__(self, *args, **kwargs):
        super(UnityPanel, self).__init__(*args, **kwargs)
        self._mouse = Mouse.create()

    def __get_menu_view(self):
        """Return the menu view."""
        menus = self.get_children_by_type(MenuView)
        assert(len(menus) == 1)
        return menus[0]

    def __get_window_buttons(self):
        """Return the window buttons view."""
        buttons = self.menus.get_children_by_type(WindowButtons)
        assert(len(buttons) == 1)
        return buttons[0]

    def __get_grab_area(self):
        """Return the panel grab area."""
        grab_areas = self.menus.get_children_by_type(GrabArea)
        assert(len(grab_areas) == 1)
        return grab_areas[0]

    def __get_indicators_view(self):
        """Return the menu view."""
        indicators = self.get_children_by_type(Indicators)
        assert(len(indicators) == 1)
        return indicators[0]

    def move_mouse_below_the_panel(self):
        """Places the mouse to bottom of this panel."""
        (x, y, w, h) = self.geometry
        target_x = x + w / 2
        target_y = y + h + 10

        logger.debug("Moving mouse away from panel.")
        self._mouse.move(target_x, target_y)

    def move_mouse_over_menus(self):
        """Move the mouse over the menu area for this panel."""
        (x, y, w, h) = self.menus.geometry
        target_x = x + w / 2
        target_y = y + h / 2

        # The menu view has bigger geometry than the real layout
        menu_entries = self.menus.get_entries()
        if len(menu_entries) > 0:
            first_x = menu_entries[0].x
            last_x = menu_entries[-1].x + menu_entries[-1].width / 2

            target_x = first_x + (last_x - first_x) / 2

        logger.debug("Moving mouse to center of menu area.")
        self._mouse.move(target_x, target_y)

    def move_mouse_over_grab_area(self):
        """Move the mouse over the grab area for this panel."""
        (x, y, w, h) = self.grab_area.geometry
        target_x = x + w / 2
        target_y = y + h / 2

        logger.debug("Moving mouse to center of grab area.")
        self._mouse.move(target_x, target_y)

    def move_mouse_over_window_buttons(self):
        """Move the mouse over the center of the window buttons area for this panel."""
        (x, y, w, h) = self.window_buttons.geometry
        target_x = x + w / 2
        target_y = y + h / 2

        logger.debug("Moving mouse to center of the window buttons.")
        self._mouse.move(target_x, target_y)

    def move_mouse_over_indicators(self):
        """Move the mouse over the center of the indicators area for this panel."""
        (x, y, w, h) = self.indicators.geometry
        target_x = x + w / 2
        target_y = y + h / 2

        logger.debug("Moving mouse to center of the indicators area.")
        self._mouse.move(target_x, target_y)

    def get_indicator_entries(self, visible_only=True, include_hidden_menus=False):
        """Returns a list of entries for this panel including both menus and indicators"""
        entries = []
        if include_hidden_menus or self.menus_shown:
            entries = self.menus.get_entries()
        entries += self.indicators.get_ordered_entries(visible_only)
        return entries

    def get_active_indicator(self):
        """Returns the indicator entry that is currently active"""
        entries = self.get_indicator_entries(False, True)
        entries = filter(lambda e: e.active == True, entries)
        assert(len(entries) <= 1)
        return entries[0] if entries else None

    def get_indicator_entry(self, entry_id):
        """Returns the indicator entry for the given ID or None"""
        entries = self.get_indicator_entries(False, True)
        entries = filter(lambda e: e.entry_id == entry_id, entries)
        assert(len(entries) <= 1)
        return entries[0] if entries else None

    @property
    def title(self):
        return self.menus.panel_title

    @property
    def desktop_is_active(self):
        return self.menus.desktop_active

    @property
    def menus_shown(self):
        return self.active and self.menus.draw_menus

    @property
    def window_buttons_shown(self):
        return self.menus.draw_window_buttons

    @property
    def window_buttons(self):
        return self.__get_window_buttons()

    @property
    def menus(self):
        return self.__get_menu_view()

    @property
    def grab_area(self):
        return self.__get_grab_area()

    @property
    def indicators(self):
        return self.__get_indicators_view()

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the current panel."""
        return (self.x, self.y, self.width, self.height)


class MenuView(UnityIntrospectionObject):
    """The Menu View class."""

    def get_entries(self):
        """Return a list of menu entries"""
        entries = self.get_children_by_type(IndicatorEntry)
        # We need to filter out empty entries, which are seperators - those
        # are not valid, visible and working entries
        # For instance, gedit adds some of those, breaking our tests
        entries = [e for e in entries if (e.label != "")]
        return entries

    def get_menu_by_label(self, entry_label):
        """Return the first indicator entry found with the given label"""
        indicators = self.get_children_by_type(IndicatorEntry, label=entry_label)
        return indicators[0] if indicators else None

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the current menu view."""
        return (self.x, self.y, self.width, self.height)


class WindowButtons(UnityIntrospectionObject):
    """The window buttons class"""

    def get_buttons(self, visible_only=True):
        """Return a list of window buttons"""
        if visible_only:
            return self.get_children_by_type(WindowButton, visible=True)
        else:
            return self.get_children_by_type(WindowButton)

    def get_button(self, type):
        buttons = self.get_children_by_type(WindowButton, type=type)
        assert(len(buttons) == 1)
        return buttons[0]

    @property
    def visible(self):
        return len(self.get_buttons()) != 0

    @property
    def close(self):
        return self.get_button("Close")

    @property
    def minimize(self):
        return self.get_button("Minimize")

    @property
    def unmaximize(self):
        return self.get_button("Unmaximize")

    @property
    def maximize(self):
        return self.get_button("Maximize")

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the current panel."""
        return (self.x, self.y, self.width, self.height)


class WindowButton(UnityIntrospectionObject):
    """The Window WindowButton class."""

    def __init__(self, *args, **kwargs):
        super(WindowButton, self).__init__(*args, **kwargs)
        self._mouse = Mouse.create()

    def mouse_move_to(self):
        target_x = self.x + self.width / 2
        target_y = self.y + self.height / 2
        self._mouse.move(target_x, target_y, rate=20, time_between_events=0.005)

    def mouse_click(self):
        self.mouse_move_to()
        sleep(.2)
        self._mouse.click(press_duration=.1)
        sleep(.01)

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the window button."""
        return (self.x, self.y, self.width, self.height)


class GrabArea(UnityIntrospectionObject):
    """The grab area class"""

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the grab area."""
        return (self.x, self.y, self.width, self.height)


class Indicators(UnityIntrospectionObject):
    """The Indicators View class."""

    def get_ordered_entries(self, visible_only=True):
        """Return a list of indicators, ordered by their priority"""

        if visible_only:
            entries = self.get_children_by_type(IndicatorEntry, visible=True)
        else:
            entries = self.get_children_by_type(IndicatorEntry)

        return sorted(entries, key=lambda entry: entry.priority)

    def get_indicator_by_name_hint(self, name_hint):
        """Return the IndicatorEntry with the name_hint"""
        indicators = self.get_children_by_type(IndicatorEntry, name_hint=name_hint)
        assert(len(indicators) == 1)
        return indicators[0]

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the indicators area."""
        return (self.x, self.y, self.width, self.height)


class IndicatorEntry(UnityIntrospectionObject):
    """The IndicatorEntry View class."""

    def __init__(self, *args, **kwargs):
        super(IndicatorEntry, self).__init__(*args, **kwargs)
        self._mouse = Mouse.create()

    def mouse_move_to(self):
        target_x = self.x + self.width / 2
        target_y = self.y + self.height / 2
        self._mouse.move(target_x, target_y, rate=20, time_between_events=0.005)

    def mouse_click(self, button=1):
        self.mouse_move_to()
        sleep(.2)
        assert(self.visible)
        self._mouse.click(press_duration=.1)
        sleep(.01)

    @property
    def geometry(self):
        """Returns a tuple of (x,y,w,h) for the indicator entry."""
        return (self.x, self.y, self.width, self.height)

    @property
    def menu_geometry(self):
        """Returns a tuple of (x,y,w,h) for the opened menu geometry."""
        return (self.menu_x, self.menu_y, self.menu_width, self.menu_height)

    def __repr__(self):
        with self.no_automatic_refreshing():
            return "<IndicatorEntry 0x%x (%s)>" % (id(self), self.label)


class Tray(UnityIntrospectionObject):
    """A panel tray object."""
