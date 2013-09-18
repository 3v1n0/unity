# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Authors: Thomi Richards, Martin Mrazik, Łukasz 'sil2100' Zemczak
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Tests to ensure unity is compatible with ibus input method."""

from __future__ import absolute_import

from unity.emulators.ibus import (
    get_active_input_engines,
    set_active_engines,
    get_available_input_engines,
    get_ibus_bus,
    )
from autopilot.matchers import Eventually
from autopilot.testcase import multiply_scenarios
from testtools.matchers import Equals, NotEquals

from unity.tests import UnityTestCase

from gi.repository import GLib
from gi.repository import IBus
import time
import dbus
import threading


# See lp:ibus-query
class IBusQuery:
    """A simple class allowing string queries to the IBus engine."""

    def __init__(self):
        self._bus = IBus.Bus()
        self._dbusconn = dbus.connection.Connection(IBus.get_address())

        # XXX: the new IBus bindings do not export create_input_context for
        #  introspection. This is troublesome - so, to workaround this problem
        #  we're directly fetching a new input context manually
        ibus_obj = self._dbusconn.get_object(IBus.SERVICE_IBUS, IBus.PATH_IBUS)
        self._test = dbus.Interface(ibus_obj, dbus_interface="org.freedesktop.IBus")
        path = self._test.CreateInputContext("IBusQuery")
        self._context = IBus.InputContext.new(path, self._bus.get_connection(), None)

        self._glibloop = GLib.MainLoop()

        self._context.connect("commit-text", self.__commit_text_cb)
        self._context.connect("update-preedit-text", self.__update_preedit_cb)
        self._context.connect("disabled", self.__disabled_cb)

        self._context.set_capabilities (9)

    def __commit_text_cb(self, context, text):
        self.result += text.text
        self._preedit = ''

    def __update_preedit_cb(self, context, text, cursor_pos, visible):
        if visible:
            self._preedit = text.text

    def __disabled_cb(self, a):
        self.result += self._preedit
        self._glibloop.quit()

    def __abort(self):
        self._abort = True

    def poll(self, engine, ibus_input):
        if len(ibus_input) <= 0:
            return None

        self.result = ''
        self._preedit = ''
        self._context.focus_in()
        self._context.set_engine(engine)

        # Timeout in case of the engine not being installed
        self._abort = False
        timeout = threading.Timer(4.0, self.__abort)
        timeout.start()
        while self._context.get_engine() is None:
            if self._abort is True:
                print "Error! Could not set the engine correctly."
                return None
            continue
        timeout.cancel()

        for c in ibus_input:
            self._context.process_key_event(ord(c), 0, 0)

        self._context.set_engine('')
        self._context.focus_out()

        GLib.timeout_add_seconds(5, lambda *args: self._glibloop.quit())
        self._glibloop.run()

        return unicode(self.result, "UTF-8")



class IBusTests(UnityTestCase):
    """Base class for IBus tests."""

    def setUp(self):
        super(IBusTests, self).setUp()
        self.set_correct_ibus_trigger_keys()
        self._ibus_query = None

    def set_correct_ibus_trigger_keys(self):
        """Set the correct keys to trigger IBus.

        This method configures the ibus trigger keys inside gconf, and also sets
        self.activate_binding and self.activate_release_binding.

        This method adds a cleanUp to reset the old keys once the test is done.

        """
        bus = get_ibus_bus()
        config = bus.get_config()

        variant = config.get_value('general/hotkey', 'triggers')
        shortcuts = []

        # If none, assume default
        if variant != None:
          shortcuts = variant.unpack()
        else:
          shortcuts = ['<Super>space']

        # IBus uses the format '<mod><mod><mod>key'
        # Autopilot uses the format 'mod+mod+mod+key'
        # replace all > with a +, and ignore the < char

        shortcut = ""
        for c in shortcuts[0]:
          if c == '>':
            shortcut += '+'
          elif c != '<':
            shortcut += c

        self.activate_binding = shortcut
        activate_release_binding_option = 'Alt+Release+Control_L'
        self.activate_release_binding = 'Alt+Control_L'

    @classmethod
    def setUpClass(cls):
        cls._old_engines = None

    @classmethod
    def tearDownClass(cls):
        if cls._old_engines is not None:
            set_active_engines(cls._old_engines)
        bus = get_ibus_bus()
        bus.exit(restart=True)

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
        """Activate IBus. """
        self.keyboard.press_and_release(self.activate_binding)

    def deactivate_ibus(self, widget):
        """Deactivate IBus. """
        self.keyboard.press_and_release(self.activate_binding)


class IBusWidgetScenariodTests(IBusTests):
    """A class that includes scenarios for the hud and dash widgets."""

    # Use lambdas here so we don't require DBus service at module import time.
    scenarios = [
        ('dash', {'widget': 'dash'}),
        ('hud', {'widget': 'hud'})
    ]

    def try_ibus_query(self):
        """This helper method tries to query ibus, and if it has connection problems,
        it restarts the ibus connection.
        It is to be used in a loop until it returns True, which means we probably
        got a proper result - stored in self.result

        """
        self.result = None
        try:
            self._ibus_query = IBusQuery()
        except:
            # Here is a tricky situation. Probably for some reason the ibus connection
            # got busted. In this case, restarting the connection from IBusQuery is not
            # enough. We have to restart the global ibus connection to be sure
            self._ibus_query = None
            get_ibus_bus()
            return False
        self.result = self._ibus_query.poll(self.engine_name, self.input)
        return self.result is not None


    def do_ibus_test(self):
        """Do the basic IBus test on self.widget using self.input and self.result."""
        try:
            result = self.result
        except:
            self.assertThat(self.try_ibus_query, Eventually(Equals(True)))
            result = self.result

        widget = getattr(self.unity, self.widget)
        widget.ensure_visible()
        self.addCleanup(widget.ensure_hidden)
        self.activate_ibus(widget.searchbar)
        self.keyboard.type(self.input)
        commit_key = getattr(self, 'commit_key', None)
        if commit_key:
            self.keyboard.press_and_release(commit_key)
        self.deactivate_ibus(widget.searchbar)
        self.assertThat(widget.search_string, Eventually(Equals(result)))



class IBusTestsPinyin(IBusWidgetScenariodTests):
    """Tests for the Pinyin(Chinese) input engine."""

    engine_name = "pinyin"

    scenarios = multiply_scenarios(
        IBusWidgetScenariodTests.scenarios,
        [
            ('photo', {'input': 'zhaopian ', 'result' : u'\u7167\u7247' }),
            ('internet', {'input': 'hulianwang ', 'result' : u'\u4e92\u8054\u7f51'}),
            ('hello', {'input': 'ninhao ', 'result' : u'\u60a8\u597d' }),
            ('management', {'input': 'guanli ', 'result' : u'\u7ba1\u7406' }),
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
            ('system', {'input': 'shisutemu ', 'result' : u'\u30b7\u30b9\u30c6\u30e0' }),
            ('game', {'input': 'ge-mu ', 'result' : u'\u30b2\u30fc\u30e0' }),
            ('user', {'input': 'yu-za- ', 'result' : u'\u30e6\u30fc\u30b6\u30fc' }),
        ],
        [
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
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)
        self.activate_ibus(self.unity.dash.searchbar)
        self.keyboard.type("cipan")
        self.keyboard.press_and_release("Tab")
        self.keyboard.type("  ")
        self.deactivate_ibus(self.unity.dash.searchbar)
        self.assertThat(self.unity.dash.search_string, Eventually(NotEquals("  ")))

    def test_ignore_key_events_on_hud(self):
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)

        self.keyboard.type("a")
        self.activate_ibus(self.unity.hud.searchbar)
        self.keyboard.type("riqi")
        old_selected = self.unity.hud.selected_button
        self.keyboard.press_and_release("Down")
        new_selected = self.unity.hud.selected_button
        self.deactivate_ibus(self.unity.hud.searchbar)

        self.assertEqual(old_selected, new_selected)


class IBusTestsAnthyIgnore(IBusTests):
    """Tests for ignoring key events while the Anthy input engine is active."""

    scenarios = None
    engine_name = "anthy"

    def setUp(self):
        super(IBusTestsAnthyIgnore, self).setUp()
        self.activate_input_engine_or_skip(self.engine_name)

    def test_ignore_key_events_on_dash(self):
        self.unity.dash.ensure_visible()
        self.addCleanup(self.unity.dash.ensure_hidden)
        self.activate_ibus(self.unity.dash.searchbar)
        self.keyboard.type("shisutemu ")
        self.keyboard.press_and_release("Tab")
        self.keyboard.press_and_release("Enter")
        self.deactivate_ibus(self.unity.dash.searchbar)
        dash_search_string = self.unity.dash.search_string

        self.assertNotEqual("", dash_search_string)

    def test_ignore_key_events_on_hud(self):
        self.unity.hud.ensure_visible()
        self.addCleanup(self.unity.hud.ensure_hidden)
        self.keyboard.type("a")
        self.activate_ibus(self.unity.hud.searchbar)
        self.keyboard.type("hiduke")
        old_selected = self.unity.hud.selected_button
        self.keyboard.press_and_release("Down")
        new_selected = self.unity.hud.selected_button
        self.deactivate_ibus(self.unity.hud.searchbar)

        self.assertEqual(old_selected, new_selected)
