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

from unity.tests import UnityTestCase


class ShoppingLensTests(UnityTestCase):
    """Test the shopping lens bahavior."""

    def setUp(self):
        super(ShoppingLensTests, self).setUp()
        try:
            urllib2.urlopen("http://www.google.com", timeout=2)
        except urllib2.URLError, e:
            self.skip("Skipping test, no internet connection")

    def tearDown(self):
        self.dash.ensure_hidden()
        super(ShoppingLensTests, self).tearDown()

    def test_no_results_in_home_lens_if_empty_search(self):
        """Test that the home lens contains no results if the search bar is empty."""
        self.dash.ensure_visible()
        lens = self.dash.get_current_lens()

        results_category = lens.get_category_by_name("More suggestions")
        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(Equals(0)))

    def test_home_lens_has_shopping_results(self):
        """Test that the home lens contains results."""
        self.dash.ensure_visible()
        lens = self.dash.get_current_lens()

        self.keyboard.type("playstation")
        results_category = lens.get_category_by_name("More suggestions")

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1)))

    def test_application_lens_has_shopping_results(self):
        """Test that the application lens contains results."""
        self.dash.reveal_application_lens()
        lens = self.dash.get_current_lens()

        self.keyboard.type("Text Editor")
        results_category = lens.get_category_by_name("More suggestions")

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1)))

    def test_music_lens_has_shopping_results(self):
        """Test that the music lens contains results."""
        self.dash.reveal_music_lens()
        lens = self.dash.get_current_lens()

        self.keyboard.type("megadeth")
        results_category = lens.get_category_by_name("More suggestions")

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1)))

    def test_preview_works_with_shopping_lens(self):
        """This test shows the dash preview works with shopping lens results."""
        self.dash.ensure_visible()
        lens = self.dash.get_current_lens()

        self.keyboard.type("a")
        results_category = lens.get_category_by_name("More suggestions")

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1)))

        results = results_category.get_results()
        results[0].preview()

        self.assertThat(self.dash.preview_displaying, Eventually(Equals(True)))

    def test_shopping_lens_preview_navigate_right(self):
        """This test shows that shopping lens results can open previews,
        then move to the next shopping result.
        """
        self.dash.ensure_visible()
        lens = self.dash.get_current_lens()

        self.keyboard.type("a")
        results_category = lens.get_category_by_name("More suggestions")

        refresh_results_fn = lambda: len(results_category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(1)))

        results = results_category.get_results()
        results[2].preview()

        self.assertThat(self.dash.preview_displaying, Eventually(Equals(True)))
        self.preview_container = self.dash.view.get_preview_container()
        start_index = self.preview_container.relative_nav_index
        self.preview_container.navigate_right()

        self.assertThat(self.preview_container.relative_nav_index, Eventually(Equals(start_index+1)))
