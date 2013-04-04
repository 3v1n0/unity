# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.keybindings import KeybindingsHelper
from testtools.matchers import GreaterThan

from unity.emulators.panel import WindowButtons

from unity.emulators import UnityIntrospectionObject
import logging
import dbus

logger = logging.getLogger(__name__)


class DashController(UnityIntrospectionObject, KeybindingsHelper):
    """The main dash controller object."""

    def get_dash_view(self):
        """Get the dash view that's attached to this controller."""
        return self.get_children_by_type(DashView)[0]

    def hide_dash_via_dbus(self):
        """ Emulate a DBus call for dash hiding  """
        dash_object = dbus.SessionBus().get_object('com.canonical.Unity',
                                             '/com/canonical/Unity/Dash')
        dash_iface = dbus.Interface(dash_object, 'com.canonical.Unity.Dash')
        dash_iface.HideDash()

    @property
    def view(self):
        return self.get_dash_view()

    def toggle_reveal(self):
        """
        Reveals the dash if it's currently hidden, hides it otherwise.
        """
        old_state = self.visible
        logger.debug("Toggling dash visibility with Super key.")
        self.keybinding("dash/reveal", 0.1)
        self.visible.wait_for(not old_state)

    def ensure_visible(self, clear_search=True):
        """
        Ensures the dash is visible.
        """
        if not self.visible:
            self.toggle_reveal()
            self.visible.wait_for(True)
            if clear_search:
                self.clear_search()

    def ensure_hidden(self):
        """
        Ensures the dash is hidden.
        """
        if self.visible:
            self.toggle_reveal()
            self.visible.wait_for(False)

    @property
    def search_string(self):
        return self.searchbar.search_string

    @property
    def searchbar(self):
        """Returns the searchbar attached to the dash."""
        return self.view.get_searchbar()

    @property
    def preview_displaying(self):
        """Returns true if the dash is currently displaying a preview"""
        return self.view.preview_displaying;

    @property
    def preview_animation(self):
        """Returns the average progress of dash slip and animating a preview.
        Between 0.0 and 1.0.
        
        """
        return self.view.preview_animation;

    def get_num_rows(self):
        """Returns the number of displayed rows in the dash."""
        return self.view.num_rows

    def clear_search(self):
        """Clear the contents of the search bar.

        Assumes dash is already visible, and search bar has keyboard focus.

        """
        self._keyboard.press_and_release("Ctrl+a")
        self._keyboard.press_and_release("Delete")
        self.search_string.wait_for("")

    def reveal_application_lens(self, clear_search=True):
        """Reveal the application lense."""
        logger.debug("Revealing application lens with Super+a.")
        self._reveal_lens("lens_reveal/apps", clear_search)
        return self.view.get_lensview_by_name("applications.lens")

    def reveal_music_lens(self, clear_search=True):
        """Reveal the music lense."""
        logger.debug("Revealing music lens with Super+m.")
        self._reveal_lens("lens_reveal/music", clear_search)
        return self.view.get_lensview_by_name("music.lens")

    def reveal_file_lens(self, clear_search=True):
        """Reveal the file lense."""
        logger.debug("Revealing file lens with Super+f.")
        self._reveal_lens("lens_reveal/files", clear_search)
        return self.view.get_lensview_by_name("files.lens")

    def reveal_video_lens(self, clear_search=True):
        """Reveal the video lens"""
        logger.debug("Revealing video lens with Super+v.")
        self._reveal_lens("lens_reveal/video", clear_search)
        return self.view.get_lensview_by_name("video.lens")

    def reveal_command_lens(self, clear_search=True):
        """Reveal the 'run command' lens."""
        logger.debug("Revealing command lens with Alt+F2.")
        self._reveal_lens("lens_reveal/command", clear_search)
        return self.view.get_lensview_by_name("commands.lens")

    def _reveal_lens(self, binding_name, clear_search):
        self.keybinding_hold(binding_name)
        self.keybinding_tap(binding_name)
        self.keybinding_release(binding_name)
        self.visible.wait_for(True)
        if clear_search:
            self.clear_search()

    @property
    def active_lens(self):
        return self.view.get_lensbar().active_lens

    def get_current_lens(self):
        """Get the currently-active LensView object."""
        active_lens_name = self.view.get_lensbar().active_lens
        return self.view.get_lensview_by_name(active_lens_name)

    @property
    def geometry(self):
        return (self.view.x, self.view.y, self.view.width, self.view.height)


class DashView(UnityIntrospectionObject):
    """The dash view."""

    def __get_window_buttons(self):
        """Return the overlay window buttons view."""
        buttons = self.get_children_by_type(OverlayWindowButtons)
        assert(len(buttons) == 1)
        return buttons[0]

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

    def get_preview_container(self):
        """Get the preview container attached to this dash view."""
        preview_containers = self.get_children_by_type(PreviewContainer)
        for preview_container in preview_containers:
            return preview_container
        return None

    @property
    def window_buttons(self):
        return self.__get_window_buttons().window_buttons()


class OverlayWindowButtons(UnityIntrospectionObject):
    """The Overlay window buttons class"""

    def window_buttons(self):
        buttons = self.get_children_by_type(WindowButtons)
        assert(len(buttons) == 1)
        return buttons[0]

class SearchBar(UnityIntrospectionObject):
    """The search bar for the dash view."""


class LensBar(UnityIntrospectionObject):
    """The bar of lens icons at the bottom of the dash."""
    def get_icon_by_name(self, name):
        """Get a LensBarIcon child object by it's name. For example, 'home.lens'."""
        icons = self.get_children_by_type(LensBarIcon)
        for icon in icons:
            if icon.name == name:
                return icon

class LensBarIcon(UnityIntrospectionObject):
    """A lens icon at the bottom of the dash."""


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

    def activate(self):
        tx = self.x + (self.width / 2)
        ty = self.y + (self.height / 2)
        m = Mouse()
        m.move(tx, ty)
        m.click(1)

    def preview(self):
        tx = self.x + (self.width / 2)
        ty = self.y + (self.height / 2)
        m = Mouse()
        m.move(tx, ty)
        m.click(3)

    def preview_key(self):
        tx = self.x + (self.width / 2)
        ty = self.y + (self.height / 2)
        m = Mouse()
        m.move(tx, ty)

        k = Keyboard()
        k.press_and_release('Menu')

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

    @property
    def expanded(self):
        """Return True if the filterbar on this lens is expanded, False otherwise.
        """
        searchbar = self._get_searchbar()
        return searchbar.showing_filters

    def ensure_expanded(self):
        """Expand the filter bar, if it's not already."""
        if not self.expanded:
            searchbar = self._get_searchbar()
            tx = searchbar.filter_label_x + (searchbar.filter_label_width / 2)
            ty = searchbar.filter_label_y + (searchbar.filter_label_height / 2)
            m = Mouse()
            m.move(tx, ty)
            m.click()
            self.expanded.wait_for(True)

    def ensure_collapsed(self):
        """Collapse the filter bar, if it's not already."""
        if self.expanded:
            searchbar = self._get_searchbar()
            tx = searchbar.filter_label_x + (searchbar.filter_label_width / 2)
            ty = searchbar.filter_label_y + (searchbar.filter_label_height / 2)
            m = Mouse()
            m.move(tx, ty)
            m.click()
            self.expanded.wait_for(False)

    def _get_searchbar(self):
        """Get the searchbar.

        This hack exists because there's now more than one SearchBar in Unity,
        and for some reason the FilterBar stuff is bundled in the SearchBar.

        """
        searchbar_state = self.get_state_by_path("//DashView/SearchBar")
        assert(len(searchbar_state) == 1)
        return self.make_introspection_object(searchbar_state[0])


class FilterExpanderLabel(UnityIntrospectionObject):
    """A label that expands into a filter within a filter bar."""

    def ensure_expanded(self):
        """Expand the filter expander label, if it's not already"""
        if not self.expanded:
            tx = self.x + self.width / 2
            ty = self.y + self.height / 2
            m = Mouse()
            m.move(tx, ty)
            m.click()
            self.expanded.wait_for(True)

    def ensure_collapsed(self):
        """Collapse the filter expander label, if it's not already"""
        if self.expanded:
            tx = self.x + self.width / 2
            ty = self.y + self.height / 2
            m = Mouse()
            m.move(tx, ty)
            m.click()
            self.expanded.wait_for(False)


class CoverArt(UnityIntrospectionObject):
    """A view which can be used to show a texture, or generate one using a thumbnailer."""


class RatingsButton(UnityIntrospectionObject):
    """A button which shows user rating as a function of stars."""


class Preview(UnityIntrospectionObject):
    """A preview of a dash lens result."""

    def get_num_actions(self):
        """Get the number of actions for the preview."""
        actions = self.get_children_by_type(ActionButton)
        return len(actions)

    def get_action_by_id(self, action_id):
        """Returns the action given it's action hint."""
        actions = self.get_children_by_type(ActionButton)
        for action in actions:
            if action.action == action_id:
                return action
        return None

    def execute_action_by_id(action_id, action):
        """Executes an action given by the id."""
        action = self.get_action_by_id(action_id)
        if action:
            tx = action.x + (searchbar.width / 2)
            ty = action.y + (searchbar.height / 2)
            m = Mouse()
            m.move(tx, ty)
            m.click()

    @property
    def cover_art(self):
        return self.get_children_by_type(CoverArt)

    @property
    def ratings_widget(self):
        return self.get_children_by_type(PreviewRatingsWidget)

    @property
    def info_hint_widget(self):
        return self.get_children_by_type(PreviewInfoHintWidget)

    @property
    def icon(self):
        return self.get_children_by_type(IconTexture)

    @property
    def text_boxes(self):
        return self.get_children_by_type(StaticCairoText)


class ApplicationPreview(Preview):
    """A application preview of a dash lens result."""

class GenericPreview(Preview):
    """A generic preview of a dash lens result."""

class MusicPreview(Preview):
    """A music preview of a dash lens result."""

class MoviePreview(Preview):
    """A movie preview of a dash lens result."""

class PreviewContent(UnityIntrospectionObject):
    """A preview content layout for the dash previews."""

    def get_current_preview(self):
        previews = self.get_children_by_type(Preview)
        if len(previews) > 0:
            return previews[0]
        return None


class PreviewContainer(UnityIntrospectionObject):
    """A container view for the main dash preview widget."""

    @property
    def content(self):
        return self.get_content()

    def get_num_previews(self):
        """Get the number of previews queued and current in the container."""
        previews = self.content.get_children_by_type(Preview)
        return len(previews)

    def get_content(self):
        """Get the preview content layout for the container."""
        return self.get_children_by_type(PreviewContent)[0]

    def get_left_navigator(self):
        """Return the left navigator object"""
        navigators = self.get_children_by_type(PreviewNavigator)
        for nav in navigators:
            if nav.direction == 2:
                return nav
        return None

    def get_right_navigator(self):
        """Return the right navigator object"""
        navigators = self.get_children_by_type(PreviewNavigator)
        for nav in navigators:
            if nav.direction == 3:
                return nav
        return None

    def navigate_left(self, count=1):
        """Navigate preview left"""
        navigator = self.get_left_navigator()

        tx = navigator.button_x + (navigator.button_width / 2)
        ty = navigator.button_y + (navigator.button_height / 2)
        m = Mouse()
        m.move(tx, ty)

        old_preview_initiate_count = self.preview_initiate_count

        for i in range(count):
            self.navigate_left_enabled.wait_for(True)
            m.click()
            self.preview_initiate_count.wait_for(GreaterThan(old_preview_initiate_count))
            old_preview_initiate_count = self.preview_initiate_count

    def navigate_right(self, count=1):
        """Navigate preview right"""
        navigator = self.get_right_navigator()

        tx = navigator.button_x + (navigator.button_width / 2)
        ty = navigator.button_y + (navigator.button_height / 2)
        m = Mouse()
        m.move(tx, ty)

        old_preview_initiate_count = self.preview_initiate_count

        for i in range(count):
            self.navigate_right_enabled.wait_for(True)
            m.click()
            self.preview_initiate_count.wait_for(GreaterThan(old_preview_initiate_count))
            old_preview_initiate_count = self.preview_initiate_count

    @property
    def animating(self):
        """Return True if the preview is animating, False otherwise."""
        return self.content.animating

    @property
    def waiting_preview(self):
        """Return True if waiting for a preview, False otherwise."""
        return self.content.waiting_preview

    @property
    def animation_progress(self):
        """Return the progress of the current preview animation."""
        return self.content.animation_progress

    @property
    def current_preview(self):
        """Return the current preview object."""
        return self.content.get_current_preview()
        preview_initiate_count_

    @property
    def preview_initiate_count(self):
        """Return the number of initiated previews since opened."""
        return self.content.preview_initiate_count

    @property
    def navigation_complete_count(self):
        """Return the number of completed previews since opened."""
        return self.content.navigation_complete_count

    @property
    def relative_nav_index(self):
        """Return the navigation position relative to the direction of movement."""
        return self.content.relative_nav_index

    @property
    def navigate_right_enabled(self):
        """Return True if right preview navigation is enabled, False otherwise."""
        return self.content.navigate_right_enabled

    @property
    def navigate_left_enabled(self):
        """Return True if left preview navigation is enabled, False otherwise."""
        return self.content.navigate_left_enabled


class PreviewNavigator(UnityIntrospectionObject):
    """A view containing a button to nagivate between previews."""

    def icon(self):
        return self.get_children_by_type(IconTexture);


class PreviewInfoHintWidget(UnityIntrospectionObject):
    """A view containing additional info for a preview."""


class PreviewRatingsWidget(UnityIntrospectionObject):
    """A view containing a rating button and user rating count."""


class Tracks(UnityIntrospectionObject):
    """A view containing music tracks."""


class Track(UnityIntrospectionObject):
    """A singular music track for dash prevews."""


class ActionButton(UnityIntrospectionObject):
    """A preview action button."""


class IconTexture(UnityIntrospectionObject):
    """An icon for the preview."""


class StaticCairoText(UnityIntrospectionObject):
    """Text boxes in the preview"""
