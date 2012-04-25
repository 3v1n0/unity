# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards, Martin Mrazik
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Tests to ensure unity is compatible with ibus input method."""

from testtools.matchers import Equals, NotEquals

from autopilot.emulators.ibus import (
    set_active_engines,
    get_available_input_engines,
    )
from autopilot.matchers import Eventually
from autopilot.tests import AutopilotTestCase, multiply_scenarios


class IBusTests(AutopilotTestCase):
    """Base class for IBus tests."""

    def setUp(self):
        super(IBusTests, self).setUp()

    def tearDown(self):
        super(IBusTests, self).tearDown()

    @classmethod
    def setUpClass(cls):
        cls._old_engines = None
        cls.activate_input_engine_or_skip(cls.engine_name)

    @classmethod
    def tearDownClass(cls):
        if cls._old_engines is not None:
            set_active_engines(cls._old_engines)

    @classmethod
    def activate_input_engine_or_skip(cls, engine_name):
        available_engines = get_available_input_engines()
        if engine_name in available_engines:
            cls._old_engines = set_active_engines([engine_name])
        else:
            raise AutopilotTestCase.skipException("This test requires the '%s' engine to be installed." % (engine_name))

    def activate_ibus(self, widget):
        """Activate IBus, and wait till it's actived on 'widget'"""
        self.assertThat(widget.im_active, Equals(False))
        self.keyboard.press_and_release('Ctrl+Space', 0.05)
        self.assertThat(widget.im_active, Eventually(Equals(True)))

    def deactivate_ibus(self, widget):
        """Deactivate ibus, and wait till it's inactive on 'widget'"""
        self.assertThat(widget.im_active, Equals(True))
        self.keyboard.press_and_release('Ctrl+Space', 0.05)
        self.assertThat(widget.im_active, Eventually(Equals(False)))

    def do_dash_test_with_engine(self):
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        self.activate_ibus(self.dash.searchbar)
        self.keyboard.type(self.input)
        commit_key = getattr(self, 'commit_key', None)
        if commit_key:
            self.keyboard.press_and_release(commit_key)
        self.deactivate_ibus(self.dash.searchbar)
        self.assertThat(self.dash.search_string, Eventually(Equals(self.result)))

    def do_hud_test_with_engine(self):
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        self.activate_ibus(self.hud.searchbar)
        self.keyboard.type(self.input)
        commit_key = getattr(self, 'commit_key', None)
        if commit_key:
            self.keyboard.press_and_release(commit_key)
        self.deactivate_ibus(self.hud.searchbar)
        self.assertThat(self.hud.search_string, Eventually(Equals(self.result)))


class IBusTestsPinyin(IBusTests):
    """Tests for the Pinyin(Chinese) input engine."""

    engine_name = "pinyin"

    scenarios = [
        ('basic', {'input': 'abc1', 'result': u'\u963f\u5e03\u4ece'}),
        ('photo', {'input': 'zhaopian ', 'result': u'\u7167\u7247'}),
        ('internet', {'input': 'hulianwang ', 'result': u'\u4e92\u8054\u7f51'}),
        ('disk', {'input': 'cipan ', 'result': u'\u78c1\u76d8'}),
        ('disk_management', {'input': 'cipan guanli ', 'result': u'\u78c1\u76d8\u7ba1\u7406'}),
    ]

    def test_simple_input_dash(self):
        self.do_dash_test_with_engine()

    def test_simple_input_hud(self):
        self.do_hud_test_with_engine()


class IBusTestsHangul(IBusTests):
    """Tests for the Hangul(Korean) input engine."""

    engine_name = "hangul"

    scenarios = [
        ('transmission', {'input': 'xmfostmaltus ', 'result': u'\ud2b8\ub79c\uc2a4\ubbf8\uc158 '}),
        ('social', {'input': 'httuf ', 'result': u'\uc18c\uc15c '}),
        ('document', {'input': 'anstj ', 'result': u'\ubb38\uc11c '}),
        ]

    def test_simple_input_dash(self):
        self.do_dash_test_with_engine()

    def test_simple_input_hud(self):
        self.do_hud_test_with_engine()


class IBusTestsAnthy(IBusTests):
    """Tests for the Anthy(Japanese) input engine."""

    engine_name = "anthy"

    scenarios = multiply_scenarios(
        [
            ('system', {'input': 'shisutemu ', 'result': u'\u30b7\u30b9\u30c6\u30e0'}),
            ('game', {'input': 'ge-mu ', 'result': u'\u30b2\u30fc\u30e0'}),
            ('user', {'input': 'yu-za- ', 'result': u'\u30e6\u30fc\u30b6\u30fc'}),
        ],
        [
            ('commit_j', {'commit_key': 'Ctrl+j'}),
            ('commit_enter', {'commit_key': 'Enter'}),
        ]
        )

    def test_simple_input_dash(self):
        self.do_dash_test_with_engine()

    def test_simple_input_hud(self):
        self.do_hud_test_with_engine()


class IBusTestsPinyinIgnore(IBusTests):
    """Tests for ignoring key events while the Pinyin input engine is active."""

    engine_name = "pinyin"

    def test_ignore_key_events_on_dash(self):
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        self.activate_ibus(self.dash.searchbar)
        self.keyboard.type("cipan")
        self.keyboard.press_and_release("Tab")
        self.keyboard.type("  ")
        self.deactivate_ibus(self.dash.searchbar)
        self.assertThat(self.dash.search_string, Eventually(NotEquals("  ")))

    def test_ignore_key_events_on_hud(self):
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)

        self.keyboard.type("a")
        self.activate_ibus(self.hud.searchbar)
        self.keyboard.type("riqi")
        old_selected = self.hud.selected_button
        self.keyboard.press_and_release("Down")
        new_selected = self.hud.selected_button
        self.deactivate_ibus(self.hud.searchbar)

        self.assertEqual(old_selected, new_selected)


class IBusTestsAnthyIgnore(IBusTests):
    """Tests for ignoring key events while the Anthy input engine is active."""

    engine_name = "anthy"

    def test_ignore_key_events_on_dash(self):
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        self.activate_ibus(self.dash.searchbar)
        self.keyboard.type("shisutemu ")
        self.keyboard.press_and_release("Tab")
        self.keyboard.press_and_release("Ctrl+j")
        self.deactivate_ibus(self.dash.searchbar)
        dash_search_string = self.dash.search_string

        self.assertNotEqual("", dash_search_string)

    def test_ignore_key_events_on_hud(self):
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        self.keyboard.type("a")
        self.activate_ibus(self.hud.searchbar)
        self.keyboard.type("hiduke")
        old_selected = self.hud.selected_button
        self.keyboard.press_and_release("Down")
        new_selected = self.hud.selected_button
        self.deactivate_ibus(self.hud.searchbar)

        self.assertEqual(old_selected, new_selected)
