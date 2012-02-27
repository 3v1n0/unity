# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testtools import TestCase
from testtools.matchers import raises
from compizconfig import Context, Plugin, Setting

from autopilot.utilities import get_keystroke_string_from_compiz_setting

_ctx = Context()

class CompizConfigKeyTranslateTests(TestCase):
    """Tests that the compizconfig keycode translation routes work as advertised."""

    def test_requires_setting_instance(self):
        """Function must raise TypeError unless given an instance of compizconfig.Setting."""
        self.assertThat(lambda: get_keystroke_string_from_compiz_setting(True), raises(TypeError))

    def test_requires_setting_key_type(self):
        """Passed in settings object must be of type 'Key', otherwise ValueError is raised."""
        plugin = Plugin(_ctx, "unityshell")

        invalid_settings = ('launcher_hide_mode',
            'alt_tab_timeout',
            'background_color',
            'panel_opacity',
            'icon_size')
        for setting_name in invalid_settings:
            setting = Setting(plugin, setting_name)
            self.assertThat(lambda: get_keystroke_string_from_compiz_setting(setting), raises(ValueError))
