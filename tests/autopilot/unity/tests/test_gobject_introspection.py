# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2013 Canonical
# Author: Iain Lane
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

from __future__ import absolute_import

from testtools import TestCase

class GirTests(TestCase):

    """ Test that GOBject Intospection bindings can imported """

    def setUp(self):
        super(GirTests, self).setUp()

    def test_appindicator_import(self):
        imported = False

        try:
            from gi.repository import AppIndicator3
            imported = True
        except ImportError:
            # failed
            pass

        self.assertTrue(imported)

    def test_dbusmenu_import(self):
        imported = False

        try:
            from gi.repository import Dbusmenu
            imported = True
        except ImportError:
            # failed
            pass

        self.assertTrue(imported)

    def test_dee_import(self):
        imported = False

        try:
            from gi.repository import Dee
            imported = True
        except ImportError:
            # failed
            pass

        self.assertTrue(imported)

    def test_unity_import(self):
        imported = False

        try:
            from gi.repository import Unity
            imported = True
        except ImportError:
            # failed
            pass

        self.assertTrue(imported)
