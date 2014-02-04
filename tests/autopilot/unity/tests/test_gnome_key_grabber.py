# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2014 Canonical Ltd.
# Author: William Hua <william.hua@canonical.com>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import dbus
import glib
import unity

from autopilot.matchers import *
from testtools.matchers import *

class GnomeKeyGrabberTests(unity.tests.UnityTestCase):
    """Gnome key grabber tests"""

    def setUp(self):
        super(GnomeKeyGrabberTests, self).setUp()

        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

        bus = dbus.SessionBus()
        proxy = bus.get_object('org.gnome.Shell', '/org/gnome/Shell')
        self.interface = dbus.Interface(proxy, 'org.gnome.Shell')

    def check_accelerator(self, action, shortcut):
        activated = [False]

        def accelerator_activated(a, b):
            if a == action:
                activated[0] = True

        self.interface.connect_to_signal('AcceleratorActivated', accelerator_activated)

        # Activate the accelerator, should emit signal
        self.keyboard.press_and_release(shortcut)

        loop = glib.MainLoop()

        def wait():
            loop.quit()
            return False

        glib.timeout_add_seconds(1, wait)
        loop.run()

        # Check that signal was emitted
        self.assertTrue(activated[0])

        # Remove accelerator, make sure it was removed
        self.assertTrue(self.interface.UngrabAccelerator(action))

        activated[0] = False

        # Activate the accelerator, should not emit signal
        self.keyboard.press_and_release(shortcut)

        loop = glib.MainLoop()

        def wait():
            loop.quit()
            return False

        glib.timeout_add_seconds(1, wait)
        loop.run()

        # Check that signal was not emitted
        self.assertFalse(activated[0])

        # Try removing accelerator, should fail
        self.assertFalse(self.interface.UngrabAccelerator(action))

    def test_grab_accelerators(self):
        accelerators = [('<Shift><Control>x', 'Shift+Control+x'),
                        ('<Control><Alt>y', 'Control+Alt+y'),
                        ('<Shift><Alt>z', 'Shift+Alt+z')]

        actions = self.interface.GrabAccelerators([(accelerator[0], 0) for accelerator in accelerators])

        for accelerator, action in zip(accelerators, actions):
            self.check_accelerator(action, accelerator[1])

    def test_grab_accelerator(self):
        action = self.interface.GrabAccelerator('<Shift><Control><Alt>a', 0)
        self.check_accelerator(action, 'Shift+Control+Alt+a')

    def test_grab_same_accelerator(self):
        actions = self.interface.GrabAccelerators(3 * [('<Shift><Control><Alt>b', 0)])
        removed = set()

        for action in actions:
            activated = [False]

            def accelerator_activated(a, b):
                if a in actions and a not in removed:
                    activated[0] = True

            self.interface.connect_to_signal('AcceleratorActivated', accelerator_activated)

            # Activate the accelerator, should emit signal
            self.keyboard.press_and_release('Shift+Control+Alt+b')

            loop = glib.MainLoop()

            def wait():
                loop.quit()
                return False

            glib.timeout_add_seconds(1, wait)
            loop.run()

            # Check that signal was emitted
            self.assertTrue(activated[0])

            # Remove accelerator, make sure it was removed
            self.assertTrue(self.interface.UngrabAccelerator(action))

            removed.add(action)

        activated = False

        def accelerator_activated(a, b):
            if a in actions:
                activated = True

        self.interface.connect_to_signal('AcceleratorActivated', accelerator_activated)

        # Activate the accelerator, should not emit signal
        self.keyboard.press_and_release('Shift+Control+Alt+b')

        loop = glib.MainLoop()

        def wait():
            loop.quit()
            return False

        glib.timeout_add_seconds(1, wait)
        loop.run()

        # Check that signal was not emitted
        self.assertFalse(activated)
