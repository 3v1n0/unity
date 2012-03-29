# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards, Martin Mrazik
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"Tests to ensure unity is compatible with ibus input method."

from autopilot.emulators.ibus import (
    set_active_engines,
    get_available_input_engines,
    )
from autopilot.tests import AutopilotTestCase, multiply_scenarios

from time import sleep

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

    def activate_ibus(self):
        # it would be nice to be able to tell if it's currently active or not.
        self.keyboard.press_and_release('Ctrl+Space')

    def deactivate_ibus(self):
        # it would be nice to be able to tell if it's currently active or not.
        self.keyboard.press_and_release('Ctrl+Space')


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
        self.activate_input_engine_or_skip("pinyin")
        self.dash.ensure_visible()
        sleep(0.5)
        self.activate_ibus()
        sleep(0.5)
        self.keyboard.type(self.input)
        dash_search_string = self.dash.search_string
        self.deactivate_ibus()
        self.dash.ensure_hidden()

        self.assertEqual(self.result, dash_search_string)

    def test_simple_input_hud(self):
        self.activate_input_engine_or_skip("pinyin")
        self.hud.ensure_visible()
        sleep(0.5)
        self.activate_ibus()
        sleep(0.5)
        self.keyboard.type(self.input)
        hud_search_string = self.hud.search_string
        self.deactivate_ibus()
        self.hud.ensure_hidden()

        self.assertEqual(self.result, hud_search_string)


class IBusTestsHangul(IBusTests):
    """Tests for the Hangul(Korean) input engine."""

    scenarios = [
        ('transmission', {'input': 'xmfostmaltus ', 'result': u'\ud2b8\ub79c\uc2a4\ubbf8\uc158 '}),
        ('social', {'input': 'httuf ', 'result': u'\uc18c\uc15c '}),
        ('document', {'input': 'anstj ', 'result': u'\ubb38\uc11c '}),
        ]

    def test_simple_input_dash(self):
        self.activate_input_engine_or_skip("hangul")
        self.dash.ensure_visible()
        sleep(0.5)
        self.activate_ibus()
        sleep(0.5)
        self.keyboard.type(self.input)
        dash_search_string = self.dash.search_string
        self.deactivate_ibus()
        self.dash.ensure_hidden()

        self.assertEqual(self.result, dash_search_string)

    def test_simple_input_hud(self):
        self.activate_input_engine_or_skip("hangul")
        self.hud.ensure_visible()
        sleep(0.5)
        self.activate_ibus()
        sleep(0.5)
        self.keyboard.type(self.input)
        hud_search_string = self.hud.search_string
        self.deactivate_ibus()
        self.hud.ensure_hidden()

        self.assertEqual(self.result, hud_search_string)


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
        self.activate_input_engine_or_skip("anthy")
        self.dash.ensure_visible()
        sleep(0.5)
        self.activate_ibus()
        sleep(0.5)
        self.keyboard.type(self.input)
        self.keyboard.press_and_release(self.commit_key)
        dash_search_string = self.dash.search_string
        self.deactivate_ibus()
        self.dash.ensure_hidden()

        self.assertEqual(self.result, dash_search_string)

    def test_simple_input_hud(self):
        self.activate_input_engine_or_skip("anthy")
        self.hud.ensure_visible()
        sleep(0.5)
        self.activate_ibus()
        sleep(0.5)
        self.keyboard.type(self.input)
        self.keyboard.press_and_release(self.commit_key)
        hud_search_string = self.hud.search_string
        self.deactivate_ibus()
        self.hud.ensure_hidden()

        self.assertEqual(self.result, hud_search_string)
