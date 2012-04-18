# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"Autopilot-specific matchers."

from testtools.matchers import Matcher


class Eventually(Matcher):
    """Asserts that a value will eventually equal a given Matcher object."""

    def __init__(self, matcher):
        super(Eventually, self).__init__()
        match_fun = getattr(matcher, 'match', None)
        if match_fun is None or not callable(match_fun):
            raise TypeError("Eventually must be called with a testtools matcher argument.")
        self.matcher = matcher

    def match(self, value):
        wait_fun = getattr(value, 'wait_for', None)
        if wait_fun is None or not callable(wait_fun):
            raise TypeError("Eventually can only be used against autopilot attributes that have a wait_for funtion.")
        wait_fun(self.matcher)

    def __str__(self):
        return "Eventually " + str(self.matcher)
