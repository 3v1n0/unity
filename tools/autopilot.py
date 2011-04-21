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
from time import sleep

import dbus
from Xlib import X
from Xlib.display import Display
from Xlib.ext.xtest import fake_input

class UnityUtil(object):
    '''Utility class for running, and poking into Unity.
    Someday when dbus is working again we can have it get launcher info from here
    for now we can just fake it and guess.'''
    
    UNITY_BUS_NAME = 'com.canonical.Unity'
    INTROSPECTION_PATH = '/com/canonical/Unity/Debug'
    INTROSPECTION_IFACE = 'com.canonical.Unity.Debug.Introspection'
    
    def __init__(self):
        self._bus = dbus.SessionBus()
        self._introspection_proxy_obj = self._bus.get_object(self.UNITY_BUS_NAME, self.INTROSPECTION_PATH)
        self._introspection_iface = dbus.Interface(self._introspection_proxy_obj,
											   self.INTROSPECTION_IFACE)
    
    def run_unity(self):
        ''' Runs unity with --reset to ensure that a default Unity/compiz session
        is being tested. If unity --reset does not return 0, a CalledProcessError
        will be raised. No exception does not mean that everything is ok, is_running
        should still be called after.'''
        ret = subprocess.check_call ('unity', '--reset')	
	
    def is_running(self):
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

    _bfb_x = 0
    _bfb_y = 0
	
    # this is totally lame. This should not be hard coded, but until I can get
    # unity to run in gdb and debug why introspection is crashing and hardlocking
    # this is the best I can do.
    _dest_x = 32
    _dest_y = 57
    
    def __init__(self):
        self._mouse = Mouse()
        self._unity = UnityUtil()
	
    def show_tooltip(self):
        '''Move mouse to a launcher and hover to show the tooltip'''
        print 'Showing tool tip...'
        self._mouse.reset()
        self._mouse.move (self._bfb_x, self._bfb_y)
        sleep (0.25)
        self._mouse.move(self._dest_x, self._dest_y)
        return self._unity.is_running()
		
    def show_quicklist(self):
        '''Move mouse to a launcher and right click'''
        print 'Showing quicklist...'
        self._mouse.reset()
        self._mouse.move (self._bfb_x, self._bfb_y)
        sleep (0.25)
        self._mouse.move(self._dest_x, self._dest_y)
        sleep(0.25)
        self._mouse.click(button=3)
        sleep(2)
        # hides quicklist
        self._mouse.click(button=3)
        return self._unity.is_running()
		
    def drag_launcher(self):
        '''Click a launcher icon and drag down to move the whole launcher'''
        print 'Dragging entire launcher...'
        self._mouse.reset()
        self._mouse.move (self._bfb_x, self._bfb_y)
        sleep (0.25)
        self._mouse.move(self._dest_x, self._dest_y)
        sleep(0.25)
        print self._mouse.position()
        print 'pressing'
        self._mouse.press()
        self._mouse.move(self._dest_x, self._dest_y + 300)
        sleep(0.25)
        print 'releasing'
        self._mouse.release()
        return self._unity.is_running()
		
    def drag_launcher_icon_along_edge_drop(self):
        '''Click a launcher icon and drag it along the edge of the launcher
        to test moving icons around on the launcher'''
        print 'Moving launcher icon along edge...'
        self._mouse.reset()		   
        self._mouse.move (self._bfb_x, self._bfb_y)
        sleep (0.25)
        self._mouse.move(self._dest_x, self._dest_y)
        self._mouse.press()
        self._mouse.move(self._dest_x + 25, self._dest_y)
        self._mouse.move(self._dest_x + 25, self._dest_y + 500)
        self._mouse.release()
        return self._unity.is_running()
	
    def drag_launcher_icon_out_and_drop(self):
        '''Click a launcher icon and drag it straight out so that when dropped
        it returns to its original position'''
        print 'Dragging launcher straight out and dropping...'
        self._mouse.reset()
        self._mouse.move (self._bfb_x, self._bfb_y)
        sleep (0.25)
        self._mouse.move(self._dest_x, self._dest_y)
        self._mouse.press()
        self._mouse.move(self._dest_x + 300, self._dest_y)
        self._mouse.release()
        return self._unity.is_running()
	
    def drag_launcher_icon_out_and_move(self):
        '''Click a launcher icon and drag it diagonally so that it changes position'''
        print 'Dragging a launcher icon out, and moving it...'''
        self._mouse.reset()
        self._mouse.move (self._bfb_x, self._bfb_y)
        sleep (0.25)
        self._mouse.move(self._dest_x, self._dest_y)
        self._mouse.press()
        self._mouse.move(self._dest_x + 300, self._dest_y)
        self._mouse.move(self._dest_x + 300, self._dest_y + 300)
        self._mouse.release()
        return self._unity.is_running()
		
if __name__ == "__main__":
    tests = UnityTests()
    if (not tests.show_tooltip()):
        print 'FAIL: SHOW TOOLTIP CRASHED UNITY'
        exit (1)
    sleep(1.5)
    if (not tests.show_quicklist()):
        print 'FAIL: SHOW QUICKLIST CRASHED UNITY'
        exit (1)
    sleep(1.5)
    if (not tests.drag_launcher()):
        print 'FAIL: DRAG LAUNCHER CRASHED UNITY'
        exit (1)
    sleep(1.5)
    if (not tests.drag_launcher_icon_along_edge_drop()):
        print 'FAIL: DRAG LAUNCHER ICON ALONG EDGE CRASHED UNITY'
        exit (1)
    sleep(1.5)
    if (not tests.drag_launcher_icon_out_and_drop()):
        print 'FAIL: DRAG LAUNCHER ICON OUT AND DROP CRASHED UNITY'
        exit (1)
    sleep(1.5)
    if (not tests.drag_launcher_icon_out_and_move()):
        print 'FAIL: DRAG LAUNCHER ICON OUT AND MOVE CRASHED UNITY'
        exit (1)
		
    print 'PASS'
