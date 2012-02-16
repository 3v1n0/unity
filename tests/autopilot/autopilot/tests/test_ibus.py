# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"Tests to ensure unity is compatible with ibus input method."

from autopilot.emulators.ibus import set_global_input_engine
from autopilot.emulators.unity.dash import Dash
from autopilot.emulators.X11 import Keyboard
from autopilot.tests import AutopilotTestCase
from time import sleep

class IBusTests(AutopilotTestCase):

    def setUp(self):
        super(IBusTests, self).setUp()
        set_global_input_engine("pinyin")

    def tearDown(self):
        super(IBusTests, self).tearDown()
        set_global_input_engine(None)

    def test_simple_dash(self):
        """Entering 'abc1' in the dash with ibus enabled should result in u'\u963f\u5e03\u4ece'."""
        dash = Dash()
        kb = Keyboard()
        kb.press_and_release('Ctrl+Space')

        dash.ensure_visible()
        kb.type('abc1')
        sleep(1)
        dash_search_string = dash.get_search_string()
        dash.ensure_hidden()
        sleep(1)

        kb.press_and_release('Ctrl+Space')

        self.assertEqual(u'\u963f\u5e03\u4ece', dash_search_string)
