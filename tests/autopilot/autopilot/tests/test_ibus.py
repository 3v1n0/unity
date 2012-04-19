# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards, Martin Mrazik
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Tests to ensure unity is compatible with ibus input method."""

from testtools.matchers import Equals, NotEquals
from time import sleep

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
        self._old_engines = None

    def tearDown(self):
        if self._old_engines is not None:
            set_active_engines(self._old_engines)
        super(IBusTests, self).tearDown()

    def activate_input_engine_or_skip(self, engine_name):
        available_engines = get_available_input_engines()
        if engine_name in available_engines:
            self._old_engines = set_active_engines([engine_name])
        else:
            self.skipTest("This test requires the '%s' engine to be installed." % (engine_name))

    def activate_ibus(self, widget):
        """Activate IBus, and wait till it's actived on 'widget'"""
        self.assertThat(widget.im_active, Equals(False))
        self.keyboard.press_and_release('Ctrl+Space')
        self.assertThat(widget.im_active, Eventually(Equals(True)))

    def deactivate_ibus(self, widget):
        self.assertThat(widget.im_active, Equals(True))
        self.keyboard.press_and_release('Ctrl+Space')
        self.assertThat(widget.im_active, Eventually(Equals(False)))

    def do_dash_test_with_engine(self, engine_name):
        self.activate_input_engine_or_skip(engine_name)
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(0.5)
        self.activate_ibus(self.dash.searchbar)
        self.addCleanup(self.deactivate_ibus, self.dash.searchbar)
        sleep(0.5)
        self.keyboard.type(self.input)
        commit_key = getattr(self, 'commit_key', None)
        if commit_key:
            self.keyboard.press_and_release(commit_key)
        self.assertThat(self.dash.search_string, Eventually(Equals(self.result)))

    def do_hud_test_with_engine(self, engine_name):
        self.activate_input_engine_or_skip(engine_name)
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(0.5)
        self.activate_ibus(self.hud.searchbar)
        self.addCleanup(self.deactivate_ibus, self.hud.searchbar)
        sleep(0.5)
        self.keyboard.type(self.input)
        commit_key = getattr(self, 'commit_key', None)
        if commit_key:
            self.keyboard.press_and_release(commit_key)
        self.assertThat(self.hud.search_string, Eventually(Equals(self.result)))


class IBusTestsPinyin(IBusTests):
    """Tests for the Pinyin(Chinese) input engine."""

    scenarios = [
        ('basic', {'input': 'abc1', 'result': u'\u963f\u5e03\u4ece'}),
        ('photo', {'input': 'zhaopian ', 'result': u'\u7167\u7247'}),
        ('internet', {'input': 'hulianwang ', 'result': u'\u4e92\u8054\u7f51'}),
        ('disk', {'input': 'cipan ', 'result': u'\u78c1\u76d8'}),
        ('disk_management', {'input': 'cipan guanli ', 'result': u'\u78c1\u76d8\u7ba1\u7406'}),
    ]

    def test_simple_input_dash(self):
        self.do_dash_test_with_engine("pinyin")

    def test_simple_input_hud(self):
        self.do_hud_test_with_engine("pinyin")


class IBusTestsHangul(IBusTests):
    """Tests for the Hangul(Korean) input engine."""

    scenarios = [
        ('transmission', {'input': 'xmfostmaltus ', 'result': u'\ud2b8\ub79c\uc2a4\ubbf8\uc158 '}),
        ('social', {'input': 'httuf ', 'result': u'\uc18c\uc15c '}),
        ('document', {'input': 'anstj ', 'result': u'\ubb38\uc11c '}),
        ]

    def test_simple_input_dash(self):
        self.do_dash_test_with_engine("hangul")

    def test_simple_input_hud(self):
        self.do_hud_test_with_engine("hangul")


class IBusTestsAnthy(IBusTests):
    """Tests for the Anthy(Japanese) input engine."""

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
        self.do_dash_test_with_engine("anthy")

    def test_simple_input_hud(self):
        self.do_hud_test_with_engine("anthy")


class IBusTestsPinyinIgnore(IBusTests):
    """Tests for ignoring key events while the Pinyin input engine is active."""

    def test_ignore_key_events_on_dash(self):
        self.activate_input_engine_or_skip("pinyin")
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(0.5)
        self.activate_ibus()
        self.addCleanup(self.deactivate_ibus)
        sleep(0.5)
        self.keyboard.type("cipan")
        self.keyboard.press_and_release("Tab")
        self.keyboard.type("  ")
        self.assertThat(self.dash.search_string, Eventually(NotEquals("  ")))

    def test_ignore_key_events_on_hud(self):
        self.activate_input_engine_or_skip("pinyin")
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(0.5)
        self.keyboard.type("a")
        self.activate_ibus()
        sleep(0.5)
        self.keyboard.type("riqi")
        old_selected = self.hud.selected_button
        self.keyboard.press_and_release("Down")
        new_selected = self.hud.selected_button

        self.assertEqual(old_selected, new_selected)


class IBusTestsAnthyIgnore(IBusTests):
    """Tests for ignoring key events while the Anthy input engine is active."""

    def test_ignore_key_events_on_dash(self):
        self.activate_input_engine_or_skip("anthy")
        self.dash.ensure_visible()
        self.addCleanup(self.dash.ensure_hidden)
        sleep(0.5)
        self.activate_ibus()
        self.addCleanup(self.deactivate_ibus)
        sleep(0.5)
        self.keyboard.type("shisutemu ")
        self.keyboard.press_and_release("Tab")
        self.keyboard.press_and_release("Ctrl+j")
        dash_search_string = self.dash.search_string

        self.assertNotEqual("", dash_search_string)

    def test_ignore_key_events_on_hud(self):
        self.activate_input_engine_or_skip("anthy")
        self.hud.ensure_visible()
        self.addCleanup(self.hud.ensure_hidden)
        sleep(0.5)
        self.keyboard.type("a")
        self.activate_ibus()
        self.addCleanup(self.deactivate_ibus)
        sleep(0.5)
        self.keyboard.type("hiduke")
        old_selected = self.hud.selected_button
        self.keyboard.press_and_release("Down")
        new_selected = self.hud.selected_button

        self.assertEqual(old_selected, new_selected)
