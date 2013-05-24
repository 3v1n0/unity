# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2013 Canonical
# Author: Łukasz 'sil2100' Zemczak
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from autopilot.matchers import Eventually
from testtools.matchers import (
    Equals,
    GreaterThan,
    )

from unity.tests import UnityTestCase

import gettext

# XXX: Ugly workaround for a really annoying bug (LP: #1152517) which breaks
#  this test suite. So, to workaround, we toggle the dash once before doing any
#  app scope search tests
workaround_scopes_load_done = False

class SearchTestsBase(UnityTestCase):
    """Base class for testing searching in search fields.
      
    Each deriving class should define the self.input_and_check_result()
    method that takes 2 arguments: the input string and the expected
    string. This method will be used during self.do_search_test(), which
    should be called for every defined scenario of input and result.
    """

    def setUp(self):
        super(SearchTestsBase, self).setUp()

    def start_test_app(self):
        """Start the window mocker for our search testing.

        This method creates a windowmocker application with a custom name and
        custom menu. We want it to have a locale-independent menu with a
        more-or-less unique menu entry for HUD testing. Also, the name of
        the application is rather unique too.
        """
        window_spec = {
            "Title": "Test menu application",
            "Menu": ["Search entry", "Quit"],
        }
        self.launch_test_window(window_spec)

    def do_search_test(self):
        """Use the input_and_check_result method for a given scenario.

        This method uses the self.input_and_check_result() method which
        needs to be defined for the given test sub-class. It uses the
        self.input and self.result strings as defined by a scenario.
        """
        self.input_and_check_result(self.input, self.result) 


# Scope tests

class ApplicationScopeSearchTestBase(SearchTestsBase):
    """Common class for all tests for searching in the application scope."""

    def setUp(self):
        super(ApplicationScopeSearchTestBase, self).setUp()
        # XXX: Temporary workaround for LP: #1152517
        global workaround_scopes_load_done
        if not workaround_scopes_load_done:
            self.unity.dash.ensure_visible()
            self.unity.dash.ensure_hidden()
            workaround_scopes_load_done = True

        self.app_scope = self.unity.dash.reveal_application_scope()
        self.addCleanup(self.unity.dash.ensure_hidden)
        gettext.install("unity-lens-applications", unicode=True)

    def input_and_check_result(self, string, expected):
        self.keyboard.type(string)
        self.assertThat(self.unity.dash.search_string, Eventually(Equals(string)))
        category = self.app_scope.get_category_by_name(_("Installed"))
        refresh_results_fn = lambda: len(category.get_results())
        self.assertThat(refresh_results_fn, Eventually(GreaterThan(0)))
        results = category.get_results()
        found = False
        for r in results:
            if r.name == expected:
                found = True
                break
        self.assertTrue(found)
 

class ApplicationScopeSearchTests(ApplicationScopeSearchTestBase):
    """Simple search tests for the application scope."""

    scenarios = [
        ('basic', {'input': 'Window Mocker', 'result': 'Window Mocker'}),
        ('lowercase', {'input': 'window mocker', 'result': 'Window Mocker'}),
        ('uppercase', {'input': 'WINDOW MOCKER', 'result': 'Window Mocker'}),
        ('partial', {'input': 'Window Mock', 'result': 'Window Mocker'}),
        ('keyword', {'input': 'arithmetic', 'result': 'Calculator'}),
    ]

    def setUp(self):
        super(ApplicationScopeSearchTests, self).setUp()

    def test_application_scope_search(self):
        self.do_search_test()


class ApplicationScopeFuzzySearchTests(ApplicationScopeSearchTestBase):
    """Fuzzy, erroneous search tests for the application scope.
    This checks if the application scope will find the searched application
    (windowmocker here, since we want some app that has the name 
    locale-independent) when small spelling errors are made.
    """

    scenarios = [
        ('transposition', {'input': 'Wnidow Mocker', 'result': 'Window Mocker'}),
        ('duplication', {'input': 'Wiindow Mocker', 'result': 'Window Mocker'}),
        ('insertion', {'input': 'Wiondow Mocker', 'result': 'Window Mocker'}),
        ('deletion', {'input': 'Wndow Mocker', 'result': 'Window Mocker'}),
    ]       

    def setUp(self):
        super(ApplicationScopeFuzzySearchTests, self).setUp()

    def test_application_scope_fuzzy_search(self):
        self.do_search_test()



# HUD tests

class HudSearchTestBase(SearchTestsBase):
    """Common class for all tests for searching in the HUD."""

    def setUp(self):
        super(HudSearchTestBase, self).setUp()
        self.start_test_app()
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden);

    def input_and_check_result(self, string, expected):
        self.keyboard.type(string)
        self.assertThat(self.unity.hud.search_string, Eventually(Equals(string), timeout=30))
        def hud_query_check():
            try:
                button = self.unity.hud.selected_hud_button
                if not button:
                   return
                return button.label_no_formatting
            except StateNotFoundError:
                return

        self.assertThat(hud_query_check, Eventually(Equals(expected), timeout=30))


class HudSearchTests(HudSearchTestBase):
    """Simple search tests for the HUD."""

    scenarios = [
        ('basic', {'input': 'Search entry', 'result': 'Search entry'}),
        ('lowercase', {'input': 'search entry', 'result': 'Search entry'}),
        ('uppercase', {'input': 'SEARCH ENTRY', 'result': 'Search entry'}),
        ('partial', {'input': 'Search ', 'result': 'Search entry'}),
    ]

    def setUp(self):
        super(HudSearchTests, self).setUp()

    def test_hud_search(self):
        self.do_search_test()


class HudFuzzySearchTests(HudSearchTestBase):
    """Fuzzy, erroneous search tests for the HUD.
    This checks if the HUD will find the searched menu entry from our application
    (windowmocker here, since we want to have unique, locale-independent menu 
    entries) when small spelling errors are made.
    """

    scenarios = [
        ('transposition', {'input': 'Saerch entry', 'result': 'Search entry'}),
        ('duplication', {'input': 'Seearch entry', 'result': 'Search entry'}),
        ('insertion', {'input': 'Seasrch entry ', 'result': 'Search entry'}),
        ('deletion', {'input': 'Serch entry', 'result': 'Search entry'}),
    ]

    def setUp(self):
        super(HudFuzzySearchTests, self).setUp()

    def test_hud_fuzzy_search(self):
        self.do_search_test()
