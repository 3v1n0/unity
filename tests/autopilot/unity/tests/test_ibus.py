# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards, Martin Mrazik
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Tests to ensure unity is compatible with ibus input method."""

from __future__ import absolute_import

from autopilot.emulators.ibus import (
    get_active_input_engines,
    set_active_engines,
    get_available_input_engines,
    get_gconf_option,
    set_gconf_option,
    )
from autopilot.matchers import Eventually
from autopilot.testcase import multiply_scenarios
from testtools.matchers import Equals, NotEquals
from unity.emulators.dash import Dash
from unity.emulators.hud import Hud

from unity.tests import UnityTestCase


class IBusTests(UnityTestCase):
    """Base class for IBus tests."""

    def setUp(self):
        super(IBusTests, self).setUp()
        self.set_correct_ibus_trigger_keys()

    def set_correct_ibus_trigger_keys(self):
        """Set the correct keys to trigger IBus.

        This method configures the ibus trigger keys inside gconf, and also sets
        self.activate_binding and self.activate_release_binding.

        This method adds a cleanUp to reset the old keys once the test is done.

        """
        # get the existing keys:
        trigger_hotkey_path = '/desktop/ibus/general/hotkey/trigger'
        old_keys = get_gconf_option(trigger_hotkey_path)

        self.activate_binding = 'Control+space'
        activate_release_binding_option = 'Alt+Release+Control_L'
        new_keys = [self.activate_binding, activate_release_binding_option]

        if new_keys != old_keys:
            set_gconf_option(trigger_hotkey_path, new_keys)
            self.addCleanup(set_gconf_option, trigger_hotkey_path, old_keys)
        self.activate_release_binding = 'Alt+Control_L'

    @classmethod
    def setUpClass(cls):
        cls._old_engines = None

    @classmethod
    def tearDownClass(cls):
        if cls._old_engines is not None:
            set_active_engines(cls._old_engines)

    def activate_input_engine_or_skip(self, engine_name):
        """Activate the input engine 'engine_name', or skip the test if the
        engine name is not avaialble (probably because it's not been installed).

        """
        available_engines = get_available_input_engines()
        if engine_name in available_engines:
            if get_active_input_engines() != [engine_name]:
                IBusTests._old_engines = set_active_engines([engine_name])
        else:
            self.skip("This test requires the '%s' engine to be installed." % (engine_name))

    def activate_ibus(self, widget):
        """Activate IBus, and wait till it's actived on 'widget'."""
        self.assertThat(widget.im_active, Equals(False))
        self.keyboard.press_and_release(self.activate_binding)
        self.assertThat(widget.im_active, Eventually(Equals(True)))

    def deactivate_ibus(self, widget):
        """Deactivate ibus, and wait till it's inactive on 'widget'."""
        self.assertThat(widget.im_active, Equals(True))
        self.keyboard.press_and_release(self.activate_binding)
        self.assertThat(widget.im_active, Eventually(Equals(False)))


class IBusWidgetScenariodTests(IBusTests):
    """A class that includes scenarios for the hud and dash widgets."""

    scenarios = [
        ('dash', {'widget': Dash()}),
        ('hud', {'widget': Hud()})
    ]

    def do_ibus_test(self):
        """Do the basic IBus test on self.widget using self.input and self.result."""
        self.widget.ensure_visible()
        self.addCleanup(self.widget.ensure_hidden)
        self.activate_ibus(self.widget.searchbar)
        self.keyboard.type(self.input)
        commit_key = getattr(self, 'commit_key', None)
        if commit_key:
            self.keyboard.press_and_release(commit_key)
        self.deactivate_ibus(self.widget.searchbar)
        self.assertThat(self.widget.search_string, Eventually(Equals(self.result)))



class IBusTestsPinyin(IBusWidgetScenariodTests):
    """Tests for the Pinyin(Chinese) input engine."""

    engine_name = "pinyin"

    scenarios = multiply_scenarios(
        IBusWidgetScenariodTests.scenarios,
        [
            ('basic', {'input': 'abc1', 'result': u'\u963f\u5e03\u4ece'}),
            #('photo', {'input': 'zhaopian ', 'result': u'\u7167\u7247'}),
            #('internet', {'input': 'hulianwang ', 'result': u'\u4e92\u8054\u7f51'}),
            #('disk', {'input': 'cipan ', 'result': u'\u78c1\u76d8'}),
            #('disk_management', {'input': 'cipan guanli ', 'result': u'\u78c1\u76d8\u7ba1\u7406'}),
        ]
    )

    def setUp(self):
        super(IBusTestsPinyin, self).setUp()
        self.activate_input_engine_or_skip(self.engine_name)

    def test_pinyin(self):
        self.do_ibus_test()


class IBusTestsHangul(IBusWidgetScenariodTests):
    """Tests for the Hangul(Korean) input engine."""

    engine_name = "hangul"

    scenarios = multiply_scenarios(
        IBusWidgetScenariodTests.scenarios,
            [
                ('transmission', {'input': 'xmfostmaltus ', 'result': u'\ud2b8\ub79c\uc2a4\ubbf8\uc158 '}),
                ('social', {'input': 'httuf ', 'result': u'\uc18c\uc15c '}),
                ('document', {'input': 'anstj ', 'result': u'\ubb38\uc11c '}),
            ]
        )

    def setUp(self):
        super(IBusTestsHangul, self).setUp()
        self.activate_input_engine_or_skip(self.engine_name)

    def test_hangul(self):
        self.do_ibus_test()


class IBusTestsAnthy(IBusWidgetScenariodTests):
    """Tests for the Anthy(Japanese) input engine."""

    engine_name = "anthy"

    scenarios = multiply_scenarios(
        IBusWidgetScenariodTests.scenarios,
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

    def setUp(self):
        super(IBusTestsAnthy, self).setUp()
        self.activate_input_engine_or_skip(self.engine_name)

    def test_anthy(self):
        self.do_ibus_test()


class IBusTestsPinyinIgnore(IBusTests):
    """Tests for ignoring key events while the Pinyin input engine is active."""

    engine_name = "pinyin"

    def setUp(self):
        super(IBusTestsPinyinIgnore, self).setUp()
        self.activate_input_engine_or_skip(self.engine_name)

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

    scenarios = None
    engine_name = "anthy"

    def setUp(self):
        super(IBusTestsAnthyIgnore, self).setUp()
        self.activate_input_engine_or_skip(self.engine_name)

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


class IBusActivationTests(IBusTests):

    """This class contains tests that make sure each IBus engine can activate
    and deactivate correctly with various keystrokes.

    """

    scenarios = multiply_scenarios(
            IBusWidgetScenariodTests.scenarios,
            [ (e, {'engine_name': e}) for e in ('pinyin','anthy','hangul') ]
        )

    def setUp(self):
        super(IBusActivationTests, self).setUp()
        self.activate_input_engine_or_skip(self.engine_name)

    def activate_ibus_on_release(self, widget):
        """Activate IBus when keys have been released, and wait till it's actived
        on 'widget'.

        """
        self.assertThat(widget.im_active, Equals(False))
        self.keyboard.press_and_release(self.activate_release_binding)
        self.assertThat(widget.im_active, Eventually(Equals(True)))

    def deactivate_ibus_on_release(self, widget):
        """Activate IBus when keys have been released, and wait till it's actived
        on 'widget'.

        """
        self.assertThat(widget.im_active, Equals(True))
        self.keyboard.press_and_release(self.activate_release_binding)
        self.assertThat(widget.im_active, Eventually(Equals(False)))

    def test_activate(self):
        """Tests the ibus activation using the "key-down" keybinding."""
        self.widget.ensure_visible()
        self.addCleanup(self.widget.ensure_hidden)
        self.assertThat(self.widget.searchbar.im_active, Equals(False))

        self.keyboard.press(self.activate_binding)
        self.addCleanup(self.keyboard.release, self.activate_binding)

        self.assertThat(self.widget.searchbar.im_active, Eventually(Equals(True)))
        self.keyboard.release(self.activate_binding)

        self.deactivate_ibus(self.widget.searchbar)

    def test_deactivate(self):
        """Tests the ibus deactivation using the "key-down" keybinding"""
        self.widget.ensure_visible()
        self.addCleanup(self.widget.ensure_hidden)
        self.activate_ibus(self.widget.searchbar)

        self.assertThat(self.widget.searchbar.im_active, Equals(True))
        self.keyboard.press(self.activate_binding)
        self.addCleanup(self.keyboard.release, self.activate_binding)
        self.assertThat(self.widget.searchbar.im_active, Eventually(Equals(False)))
        self.keyboard.release(self.activate_binding)
        self.assertThat(self.widget.searchbar.im_active, Eventually(Equals(False)))

    def test_activate_on_release(self):
        """Tests the ibus activation using "key-up" keybinding"""
        self.widget.ensure_visible()
        self.addCleanup(self.widget.ensure_hidden)

        self.assertThat(self.widget.searchbar.im_active, Equals(False))
        self.keyboard.press(self.activate_release_binding)
        self.addCleanup(self.keyboard.release, self.activate_release_binding)
        self.assertThat(self.widget.searchbar.im_active, Eventually(Equals(False)))
        self.keyboard.release(self.activate_release_binding)
        self.assertThat(self.widget.searchbar.im_active, Eventually(Equals(True)))

        self.deactivate_ibus_on_release(self.widget.searchbar)

    def test_deactivate_on_release(self):
        """Tests the ibus deactivation using "key-up" keybinding"""
        self.widget.ensure_visible()
        self.addCleanup(self.widget.ensure_hidden)
        self.activate_ibus_on_release(self.widget.searchbar)

        self.assertThat(self.widget.searchbar.im_active, Equals(True))
        self.keyboard.press(self.activate_release_binding)
        self.addCleanup(self.keyboard.release, self.activate_release_binding)
        self.assertThat(self.widget.searchbar.im_active, Eventually(Equals(True)))
        self.keyboard.release(self.activate_release_binding)
        self.assertThat(self.widget.searchbar.im_active, Eventually(Equals(False)))
