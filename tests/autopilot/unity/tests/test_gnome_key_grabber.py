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
import logging

from autopilot.matchers import *
from testtools.matchers import *

log = logging.getLogger(__name__)

class Accelerator:

    def __init__(self, accelerator=None, shortcut=None, action=0):
        self.accelerator = accelerator
        self.shortcut = shortcut
        self.action = action

    def __str__(self):
        if self.action:
            return "%u '%s'" % (self.action, self.shortcut)
        else:
            return "'%s'" % self.shortcut

class GnomeKeyGrabberTests(unity.tests.UnityTestCase):
    """Gnome key grabber tests"""

    def setUp(self):
        super(GnomeKeyGrabberTests, self).setUp()

        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

        bus = dbus.SessionBus()
        proxy = bus.get_object('org.gnome.Shell', '/org/gnome/Shell')
        self.interface = dbus.Interface(proxy, 'org.gnome.Shell')

        self.activatable = set()
        self.activated = [False]
        self.active = True

        def accelerator_activated(action, device):
            if self.active and action in self.activatable:
                log.info('%d activated' % action)
                self.activated[0] = True

        self.signal = self.interface.connect_to_signal('AcceleratorActivated', accelerator_activated)

    def tearDown(self):
        self.active = False

        super(GnomeKeyGrabberTests, self).tearDown()

    def press_accelerator(self, accelerator):
        self.activated[0] = False

        # Press accelerator shortcut
        log.info("pressing '%s'" % accelerator.shortcut)
        self.keyboard.press_and_release(accelerator.shortcut)

        loop = glib.MainLoop()

        def wait():
            loop.quit()
            return False

        glib.timeout_add_seconds(1, wait)
        loop.run()

        return self.activated[0]

    def check_accelerator(self, accelerator):
        # Check that accelerator works
        self.assertTrue(self.press_accelerator(accelerator))

        # Remove accelerator
        log.info('ungrabbing %s' % accelerator)
        self.assertTrue(self.interface.UngrabAccelerator(accelerator.action))

        # Check that accelerator does not work
        self.assertFalse(self.press_accelerator(accelerator))

        # Try removing accelerator
        log.info('ungrabbing %s (should fail)' % accelerator)
        self.assertFalse(self.interface.UngrabAccelerator(accelerator.action))

    def test_grab_accelerators(self):
        accelerators = [Accelerator('<Shift><Control>x', 'Shift+Control+x'),
                        Accelerator('<Control><Alt>y', 'Control+Alt+y'),
                        Accelerator('<Shift><Alt>z', 'Shift+Alt+z')]

        actions = self.interface.GrabAccelerators([(accelerator.accelerator, 0) for accelerator in accelerators])

        self.activatable.clear()

        for accelerator, action in zip(accelerators, actions):
            accelerator.action = action
            self.activatable.add(action)
            log.info('grabbed %s' % accelerator)

        def clean_up_test_grab_accelerators():
            self.activatable.clear()

            for accelerator in accelerators:
                log.info('unconditionally ungrabbing %s' % accelerator)
                self.interface.UngrabAccelerator(accelerator.action)

        self.addCleanup(clean_up_test_grab_accelerators)

        for accelerator in accelerators:
            self.check_accelerator(accelerator)

    def test_grab_accelerator(self):
        accelerator = Accelerator('<Shift><Control><Alt>a', 'Shift+Control+Alt+a')
        accelerator.action = self.interface.GrabAccelerator(accelerator.accelerator, 0)

        self.activatable.clear()
        self.activatable.add(accelerator.action)

        log.info('grabbed %s' % accelerator)

        def clean_up_test_grab_accelerator():
            self.activatable.clear()
            log.info('unconditionally ungrabbing %s' % accelerator)
            self.interface.UngrabAccelerator(accelerator.action)

        self.addCleanup(clean_up_test_grab_accelerator)

        self.check_accelerator(accelerator)

    def test_grab_same_accelerator(self):
        accelerators = [Accelerator('<Shift><Control><Alt>b', 'Shift+Control+Alt+b') for i in xrange(3)]
        actions = self.interface.GrabAccelerators([(accelerator.accelerator, 0) for accelerator in accelerators])

        self.activatable.clear()

        for accelerator, action in zip(accelerators, actions):
            accelerator.action = action
            self.activatable.add(action)
            log.info('grabbed %s' % accelerator)

        def clean_up_test_grab_same_accelerator():
            self.activatable.clear()

            for accelerator in accelerators:
                log.info('unconditionally ungrabbing %s' % accelerator)
                self.interface.UngrabAccelerator(accelerator.action)

        self.addCleanup(clean_up_test_grab_same_accelerator)

        for accelerator in accelerators:
            # Check that accelerator works
            self.assertTrue(self.press_accelerator(accelerator))

            # Remove accelerator
            log.info('ungrabbing %s' % accelerator)
            self.assertTrue(self.interface.UngrabAccelerator(accelerator.action))

            # This accelerator cannot activate any more
            self.activatable.remove(accelerator.action)

        # Add them all again for one final check
        for accelerator in accelerators:
            self.activatable.add(accelerator.action)

        # Check that signal was not emitted
        self.assertFalse(self.press_accelerator(accelerators[0]))
