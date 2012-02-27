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
    ]

    def test_translation(self):
        self.assertThat(translate_compiz_keystroke_string(self.input), Equals(self.expected))


