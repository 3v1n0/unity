# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from time import sleep

from autopilot.keybindings import KeybindingsHelper
from autopilot.emulators.unity import UnityIntrospectionObject
from autopilot.emulators.X11 import Keyboard, Mouse


import logging
logger = logging.getLogger(__name__)


class Dash(KeybindingsHelper):
    """
    An emulator class that makes it easier to interact with the unity dash.
    """

    def __init__(self):
        self.controller = DashController.get_all_instances()[0]
        self.view = self.controller.get_dash_view()

        self._keyboard = Keyboard()
        super(Dash, self).__init__()

    def toggle_reveal(self):
        """
        Reveals the dash if it's currently hidden, hides it otherwise.
        """
        logger.debug("Toggling dash visibility with Super key.")
        self.keybinding("dash/reveal")
        sleep(1)

    def ensure_visible(self):
        """
        Ensures the dash is visible.
        """
        if not self.get_is_visible():
            self.toggle_reveal()

    def ensure_hidden(self):
        """
        Ensures the dash is hidden.
        """
        if self.get_is_visible():
            self.toggle_reveal()

    def get_is_visible(self):
        """
        Is the dash visible?
        """
        self.controller.refresh_state()
        return self.controller.visible

    def get_searchbar(self):
        """Returns the searchbar attached to the dash."""
        return self.view.get_searchbar()

    def get_num_rows(self):
        """Returns the number of displayed rows in the dash."""
        self.view.refresh_state()
        return self.view.num_rows

    def reveal_application_lens(self):
        """Reveal the application lense."""
        logger.debug("Revealing application lens with Super+a.")
        self._reveal_lens("lens_reveal/apps")

    def reveal_music_lens(self):
        """Reveal the music lense."""
        logger.debug("Revealing music lens with Super+m.")
        self._reveal_lens("lens_reveal/music")

    def reveal_file_lens(self):
        """Reveal the file lense."""
        logger.debug("Revealing file lens with Super+f.")
        self._reveal_lens("lens_reveal/files")

    def reveal_command_lens(self):
        """Reveal the 'run command' lens."""
        logger.debug("Revealing command lens with Alt+F2.")
        self._reveal_lens("lens_reveal/command")

    def _reveal_lens(self, binding_name):
        self.keybinding_hold(binding_name)
        self.keybinding_tap(binding_name)
        self.keybinding_release(binding_name)

    def get_current_lens(self):
        """Get the currently-active LensView object."""
        active_lens_name = self.view.get_lensbar().active_lens
        return self.view.get_lensview_by_name(active_lens_name)


class DashController(UnityIntrospectionObject):
    """The main dash controller object."""

    def get_dash_view(self):
        """Get the dash view that's attached to this controller."""
        return self.get_children_by_type(DashView)[0]


class DashView(UnityIntrospectionObject):
    """The dash view."""

    def get_searchbar(self):
        """Get the search bar attached to this dash view."""
        return self.get_children_by_type(SearchBar)[0]

    def get_lensbar(self):
        """Get the lensbar attached to this dash view."""
        return self.get_children_by_type(LensBar)[0]

    def get_lensview_by_name(self, lens_name):
        """Get a LensView child object by it's name. For example, "home.lens"."""
        lenses = self.get_children_by_type(LensView)
        for lens in lenses:
            if lens.name == lens_name:
                return lens


class SearchBar(UnityIntrospectionObject):
    """The search bar for the dash view."""


class LensBar(UnityIntrospectionObject):
    """The bar of lens icons at the bottom of the dash."""


class LensView(UnityIntrospectionObject):
    """A Lens View."""

    def get_groups(self):
        """Get a list of all groups within this lensview. May return an empty list."""
        groups = self.get_children_by_type(PlacesGroup)
        return groups

    def get_focused_category(self):
        """Return a PlacesGroup instance for the category whose header has keyboard focus.

        Returns None if no category headers have keyboard focus.

        """
        categories = self.get_children_by_type(PlacesGroup)
        matches = [m for m in categories if m.header_has_keyfocus]
        if matches:
            return matches[0]
        return None

    def get_category_by_name(self, category_name):
        """Return a PlacesGroup instance with the given name, or None."""
        categories = self.get_children_by_type(PlacesGroup)
        matches = [m for m in categories if m.name == category_name]
        if matches:
            return matches[0]
        return None

    def get_num_visible_categories(self):
        """Get the number of visible categories in this lens."""
        return len([c for c in self.get_children_by_type(PlacesGroup) if c.is_visible])

    def get_filterbar(self):
        """Get the filter bar for the current lense, or None if it doesn't have one."""
        bars = self.get_children_by_type(FilterBar)
        if bars:
            return bars[0]
        return None


class PlacesGroup(UnityIntrospectionObject):
    """A category in the lense view."""

    def get_results(self):
        """Get a list of all results within this category. May return an empty list."""
        result_view = self.get_children_by_type(ResultView)[0]
        return result_view.get_children_by_type(Result)


class ResultView(UnityIntrospectionObject):
    """Contains a list of Result objects."""


class Result(UnityIntrospectionObject):
    """A single result in the dash."""


class FilterBar(UnityIntrospectionObject):
    """A filterbar, as shown inside a lens."""

    def get_num_filters(self):
        """Get the number of filters in this filter bar."""
        filters = self.get_children_by_type(FilterExpanderLabel)
        return len(filters)

    def get_focused_filter(self):
        """Returns the id of the focused filter widget."""
        filters = self.get_children_by_type(FilterExpanderLabel)
        for filter_label in filters:
            if filter_label.expander_has_focus:
                return filter_label
        return None

    def is_expanded(self):
        """Return True if the filterbar on this lens is expanded, False otherwise.
        """
        searchbar = SearchBar.get_all_instances()[0]
        return searchbar.showing_filters

    def ensure_expanded(self):
        """Expand the filter bar, if it's not already."""
        if not self.is_expanded():
            searchbar = SearchBar.get_all_instances()[0]
            tx = searchbar.filter_label_x + (searchbar.filter_label_width / 2)
            ty = searchbar.filter_label_y + (searchbar.filter_label_height / 2)
            m = Mouse()
            m.move(tx, ty)
            m.click()

    def ensure_collapsed(self):
        """Collapse the filter bar, if it's not already."""
        if self.is_expanded():
            searchbar = SearchBar.get_all_instances()[0]
            tx = searchbar.filter_label_x + (searchbar.filter_label_width / 2)
            ty = searchbar.filter_label_y + (searchbar.filter_label_height / 2)
            m = Mouse()
            m.move(tx, ty)
            m.click()


class FilterExpanderLabel(UnityIntrospectionObject):
    """A label that expands into a filter within a filter bar."""
