#!/usr/bin/python
#
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Alex Launi
#
# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License version 3, as published 
# by the Free Software Foundation.
#
# This script is designed to run unity in a test drive manner. It will drive 
# X and test the GL calls that Unity makes, so that we can easily find out if
# we are triggering graphics driver/X bugs.

import subprocess
from sys import exit
from threading import Thread
from time import sleep

import dbus
import pynotify
from Xlib import X
from Xlib.display import Display
from Xlib.ext.xtest import fake_input

class UnityUtil(object):
    '''Utility class for poking into Unity.
    Someday when dbus is working again we can have it get launcher info from here
    for now we can just fake it and guess.'''
    
    UNITY_BUS_NAME = 'com.canonical.Unity'
    DEBUG_PATH = '/com/canonical/Unity/Debug'
    INTROSPECTION_IFACE = 'com.canonical.Unity.Debug.Introspection'
    AUTOPILOT_IFACE = 'com.canonical.Unity.Autopilot'
    
unity
    def __init__(self):
        self._bus = dbus.SessionBus()
        self._debug_proxy_obj = self._bus.get_object(self.UNITY_BUS_NAME,
                                                     self.DEBUG_PATH)
        self._introspection_iface = dbus.Interface(self._debug_proxy_obj,
                                                   self.INTROSPECTION_IFACE)
        self._autopilot_iface = dbus.Interface(self._debug_proxy_obj,
                                               self.AUTOPILOT_IFACE)

    def run_autopilot(self):
        self._autopilot_iface.Run()
    
    def bus_owned(self):
        '''Checks if Unity is running by examing the session bus, and checking if
        'com.canonical.Unity' is currently owned.''' 
        return self._bus.name_has_owner(self.UNITY_BUS_NAME)
        
class Mouse(object):
        '''Wrapper around xlib to make moving the mouse easier'''
        
        def __init__(self):
                self._display = Display()
                
        def press(self, button=1):
                '''Press mouse button at current mouse location'''
                fake_input(self._display, X.ButtonPress, button)
                self._display.sync()
                
        def release(self, button=1):
                '''Releases mouse button at current mouse location'''
                fake_input(self._display, X.ButtonRelease, button)
                self._display.sync()
                
        def click(self, button=1):
                '''Click mouse at current location'''
                self.press(button)
                sleep(0.25)
                self.release(button)
                
        def move(self, x, y):
                '''Moves mouse to location (x, y)'''
                def perform_move(x, y):
                        fake_input(self._display, X.MotionNotify, x=x, y=y)
                        self._display.sync()
                        sleep(0.001)
                        
                dest_x, dest_y = x, y
                curr_x, curr_y = self.position()
                
                # calculate a path from our current position to our destination
                dy = float(curr_y - dest_y)
                dx = float(curr_x - dest_x)
                slope = dy/dx if dx > 0 else 0
                yint = curr_y - (slope * curr_x)
                xscale = 1 if dest_x > curr_x else -1
                
                while (int(curr_x) != dest_x):
                        curr_x += xscale;
                        curr_y = int(slope * curr_x + yint) if curr_y > 0 else dest_y
                        
                        perform_move(curr_x, curr_y)
                        
                if (curr_y != dest_y):
                        yscale = 1 if dest_y > curr_y else -1   
                        while (curr_y != dest_y):
                                curr_y += yscale
                                perform_move(curr_x, curr_y)
                                

        def position(self):
                '''Returns the current position of the mouse pointer'''
                coord = self._display.screen().root.query_pointer()._data
                x, y = coord["root_x"], coord["root_y"]
                return x, y
        
        def reset(self):
                fake_input (self._display, X.MotionNotify, x = 800, y = 500)
                self._display.sync()
                
class UnityTests(object):
        '''Runs a series of unity actions, triggering GL calls'''
        
        # this is totally lame. This should not be hard coded, but until I can get
        # unity to run in gdb and debug why introspection is crashing and hardlocking
        # this is the best I can do.
        _dest_x = 32
        _dest_y = 57
        
        def __init__(self, unity):
                self._mouse = Mouse()
                self._unity = UnityUtil()
                assert unity.bus_owned()
                
        def show_tooltip(self):
                '''Move mouse to a launcher and hover to show the tooltip'''
                print 'Showing tool tip...'
                self._mouse.reset()
                self._mouse.move(self._dest_x, self._dest_y)
                
        def show_quicklist(self):
                '''Move mouse to a launcher and right click'''
                print 'Showing quicklist...'
                self._mouse.reset()
                self._mouse.move(self._dest_x, self._dest_y)
                sleep(0.25)
                self._mouse.click(button=3)
                sleep(2)
                # hides quicklist
                self._mouse.click(button=3)
                
        def drag_launcher(self):
                '''Click a launcher icon and drag down to move the whole launcher'''
                print 'Dragging entire launcher...'
                self._mouse.reset()
                self._mouse.move(self._dest_x, self._dest_y)
                sleep(0.25)
                self._mouse.press()
                self._mouse.move(self._dest_x, self._dest_y + 300)
                sleep(0.25)
                self._mouse.release()
                
        def drag_launcher_icon_along_edge_drop(self):
                '''Click a launcher icon and drag it along the edge of the launcher
                to test moving icons around on the launcher'''
                print 'Moving launcher icon along edge...'
                self._mouse.reset()
                self._mouse.move(self._dest_x, self._dest_y)
                self._mouse.press()
                self._mouse.move(self._dest_x + 25, self._dest_y)
                self._mouse.move(self._dest_x + 25, self._dest_y + 500)
                self._mouse.release()
        
        def drag_launcher_icon_out_and_drop(self):
                '''Click a launcher icon and drag it straight out so that when dropped
                it returns to its original position'''
                print 'Dragging launcher straight out and dropping...'
                self._mouse.reset()
                self._mouse.move(self._dest_x, self._dest_y)
                self._mouse.press()
                self._mouse.move(self._dest_x + 300, self._dest_y)
                self._mouse.release()
        
        def drag_launcher_icon_out_and_move(self):
                '''Click a launcher icon and drag it diagonally so that it changes position'''
                print 'Dragging a launcher icon out, and moving it...'''
                self._mouse.reset()
                self._mouse.move(self._dest_x, self._dest_y)
                self._mouse.press()
                self._mouse.move(self._dest_x + 300, self._dest_y)
                self._mouse.move(self._dest_x + 300, self._dest_y + 300)
                self._mouse.release()

class UnityTestRunner(Thread):
    '''Runs Unity tests, reports tests' status.'''
    def __init__(self, unity_instance):
        Thread.__init__(self)
        self._unity = unity_instance

    def run(self):
        # Wait 10 seconds and show a notificationt that we're going to run an automated Unity
        sleep(10)
        if pynotify.init('Unity Autopilot'):
            n = pynotify.Notification('Autopilot',
                                      'Your automated test run of Unity will begin in just a moment.')
            n.show()

        # Wait 20 more seconds for compiz and unity to start
        sleep(20)
        try:
            self._unity_util = UnityUtil ()
            self._tests = UnityTests(self._unity_util)
        except:
            print 'FAIL: UNITY FAILED TO LAUNCH'
            exit(1)
        
        tests = {'Show tooltip': self._tests.show_tooltip,
                 'Show quicklist': self._tests.show_quicklist,
                 'Drag launcher': self._tests.drag_launcher,
                 'Drag launcher icon along edge and drop': self._tests.drag_launcher_icon_along_edge_drop,
                 'Drag launcher icon out and drop': self._tests.drag_launcher_icon_out_and_drop,
                 'Drag launcher icon out and move': self._tests.drag_launcher_icon_out_and_move
        }
                 
        for (name, test) in tests.items():
            sleep(2)
            if self._test_passed(test):
                continue
            else:
                print 'FAIL: %d CRASHED UNITY' % name
                exit(1)
            
        print 'ALL TESTS PASSED'

    def _test_passed(self, test):
        '''Returns whether or not the test pass, and if Unity is still running'''
        test()
        return self._unity_util.bus_owned() and self._unity.poll() is None
