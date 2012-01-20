# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License version 3, as published 
# by the Free Software Foundation.

from subprocess import call
from time import sleep

from testtools import TestCase
from testtools.matchers import Equals
from testtools.matchers import NotEquals

from compizconfig import Setting
from compizconfig import Plugin

from autopilot.globals import global_context
from autopilot.emulators.unity import Switcher
from autopilot.emulators.bamf import Bamf
from autopilot.glibrunner import GlibRunner



class SwitcherTests(TestCase):
    """Test the switcher."""
    run_test_with = GlibRunner

    def set_timeout_setting(self, value):
        self.setting.Value = value
        global_context.Write()
        
    def setUp(self):
        self.plugin = Plugin(global_context, "unityshell")
        self.setting = Setting(self.plugin, "alt_tab_timeout")
        self.bamf = Bamf()

        self.bamf.launch_application("gucharmap.desktop")
        self.bamf.launch_application("gcalctool.desktop")
        self.bamf.launch_application("mahjongg.desktop")
        
        super(SwitcherTests, self).setUp()
        
        self.server = Switcher()

    def tearDown(self):
        call(["killall", "gcalctool"])
        call(["killall", "gucharmap"])
        call(["killall", "mahjongg"])
        super(SwitcherTests, self).tearDown()
        sleep(1)

    def test_switcher_move_next(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.server.initiate()
        sleep(.2)
        
        start = self.server.get_selection_index()
        self.server.next_icon()
        sleep(.2)
        
        end = self.server.get_selection_index()
        self.server.terminate()
        
        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start+1))
        self.set_timeout_setting(True)
    
    def test_switcher_move_prev(self):
        self.set_timeout_setting(False)
        sleep(1)

        self.server.initiate()
        sleep(.2)
        
        start = self.server.get_selection_index()
        self.server.previous_icon()
        sleep(.2)
        
        end = self.server.get_selection_index()
        self.server.terminate()
        
        self.assertThat(start, NotEquals(0))
        self.assertThat(end, Equals(start-1))
        self.set_timeout_setting(True)
