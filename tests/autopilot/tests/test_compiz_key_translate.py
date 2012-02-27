# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from testscenarios import TestWithScenarios
from testtools import TestCase
from testtools.matchers import raises, Equals

from autopilot.utilities import translate_compiz_keystroke_string

class KeyTranslateArgumentTests(TestWithScenarios, TestCase):
    """Tests that the compizconfig keycode translation routes work as advertised."""

    scenarios = [
        ('bool', {'input': True}),
        ('int', {'input': 42}),
        ('float', {'input': 0.321}),
        ('none', {'input': None}),
    ]

    def test_requires_string_instance(self):
        """Function must raise TypeError unless given an instance of basestring."""
        self.assertThat(lambda: translate_compiz_keystroke_string(self.input), raises(TypeError))


class TranslationTests(TestWithScenarios, TestCase):
    """Test that we get the result we expect, with the given input."""

    scenarios = [
        ('empty string', dict(input='', expected='')),
        ('single simpe letter', dict(input='a', expected='a')),
        ('trailing space', dict(input='d   ', expected='d')),
        ('only whitespace', dict(input='\t\n   ', expected='')),
        ('special key: Ctrl', dict(input='<Control>', expected='Ctrl')),
        ('special key: Primary', dict(input='<Primary>', expected='Ctrl')),
        ('special key: Alt', dict(input='<Alt>', expected='Alt')),
        ('special key: Shift', dict(input='<Shift>', expected='Shift')),
        ('direction key up', dict(input='Up', expected='Up')),
        ('direction key down', dict(input='Down', expected='Down')),
        ('direction key left', dict(input='Left', expected='Left')),
        ('direction key right', dict(input='Right', expected='Right')),
        ('Ctrl+a', dict(input='<Control>a', expected='Ctrl+a')),
        ('Primary+a', dict(input='<Control>a', expected='Ctrl+a')),
        ('Shift+s', dict(input='<Shift>s', expected='Shift+s')),
        ('Alt+d', dict(input='<Alt>d', expected='Alt+d')),
        ('Super+w', dict(input='<Super>w', expected='Super+w')),
        ('Ctrl+Up', dict(input='<Control>Up', expected='Ctrl+Up')),
        ('Primary+Down', dict(input='<Control>Down', expected='Ctrl+Down')),
        ('Alt+Left', dict(input='<Alt>Left', expected='Alt+Left')),
        ('Shift+F3', dict(input='<Shift>F3', expected='Shift+F3')),
        ('duplicate keys Ctrl+Ctrl', dict(input='<Control><Control>', expected='Ctrl')),
        ('duplicate keys Ctrl+Primary', dict(input='<Control><Primary>', expected='Ctrl')),
        ('duplicate keys Ctrl+Primary', dict(input='<Primary><Control>', expected='Ctrl')),
        ('duplicate keys Alt+Alt', dict(input='<Alt><Alt>', expected='Alt')),
        ('duplicate keys Ctrl+Primary+left', dict(input='<Control><Primary>Left', expected='Ctrl+Left')),
        ('first key wins', dict(input='<Control><Alt>Down<Alt>', expected='Ctrl+Alt+Down')),
        ('Getting silly now', dict(input='<Control><Primary><Shift><Shift><Alt>Left', expected='Ctrl+Shift+Alt+Left')),
    ]

    def test_translation(self):
        self.assertThat(translate_compiz_keystroke_string(self.input), Equals(self.expected))


