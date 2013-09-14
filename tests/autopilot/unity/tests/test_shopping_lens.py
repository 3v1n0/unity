# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Brandon Schaefer
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import Equals, GreaterThan
from time import sleep
import urllib2
import gettext

from unity.tests import UnityTestCase


class ShoppingScopeTests(UnityTestCase):
    """Test the shopping scope bahavior."""

    def setUp(self):
        super(ShoppingScopeTests, self).setUp()
        try:
            urllib2.urlopen("http://www.google.com", timeout=2)
        except urllib2.URLError, e:
            self.skip("Skipping test, no internet connection")
        gettext.install("unity-scope-shopping")

    def tearDown(self):
        self.unity.dash.ensure_hidden()
        super(ShoppingScopeTests, self).tearDown()

    def test_no_results_in_home_scope_if_empty_search(self):
        """Test that the home scope contains no results if the search bar is empty."""
        self.unity.dash.ensure_visible()
        scope = self.unity.dash.get_current_scope()

        results_category = scope.get_category_by_name(_("More suggestions"))
        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(Equals(0)))

    def test_home_scope_has_shopping_results(self):
        """Test that the home scope contains results."""
        self.unity.dash.ensure_visible()
        scope = self.unity.dash.get_current_scope()

        self.keyboard.type("playstation")
        results_category = scope.get_category_by_name(_("More suggestions"))

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1), timeout=25))

    def test_application_scope_has_shopping_results(self):
        """Test that the application scope contains results."""
        self.unity.dash.reveal_application_scope()
        scope = self.unity.dash.get_current_scope()

        self.keyboard.type("Text Editor")
        results_category = scope.get_category_by_name(_("More suggestions"))

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1), timeout=25))

    def test_music_scope_has_shopping_results(self):
        """Test that the music scope contains results."""
        self.unity.dash.reveal_music_scope()
        scope = self.unity.dash.get_current_scope()

        self.keyboard.type("megadeth")
        results_category = scope.get_category_by_name(_("More suggestions"))

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1), timeout=25))

    def test_preview_works_with_shopping_scope(self):
        """This test shows the dash preview works with shopping scope results."""
        self.unity.dash.ensure_visible()
        scope = self.unity.dash.get_current_scope()

        self.keyboard.type("playstation")
        results_category = scope.get_category_by_name(_("More suggestions"))

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1), timeout=25))

        results = results_category.get_results()
        results[0].preview()

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(True)))

    def test_shopping_scope_preview_navigate_right(self):
        """This test shows that shopping scope results can open previews,
        then move to the next shopping result.
        """
        self.unity.dash.ensure_visible()
        scope = self.unity.dash.get_current_scope()

        self.keyboard.type("playstation")
        results_category = scope.get_category_by_name(_("More suggestions"))

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(2), timeout=25))

        results = results_category.get_results()
        results[0].preview()

        self.assertThat(self.unity.dash.preview_displaying, Eventually(Equals(True)))
        self.preview_container = self.unity.dash.view.get_preview_container()
        start_index = self.preview_container.relative_nav_index
        self.preview_container.navigate_right()

        self.assertThat(self.preview_container.relative_nav_index, Eventually(Equals(start_index+1)))
