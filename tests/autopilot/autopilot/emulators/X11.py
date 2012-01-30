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

"""
A collection of emulators for X11 - namely keyboards and mice. In the future we may 
also need other devices.
"""


from time import sleep

from Xlib import X
from Xlib import XK
from Xlib.display import Display
from Xlib.ext.xtest import fake_input
import gtk.gdk

class Keyboard(object):
    '''Wrapper around xlib to make faking keyboard input possible'''
    _lame_hardcoded_keycodes = {
        'E' : 9, # escape
        'A' : 64, 
        'C' : 37,
        'S' : 50,
        'T' : 23,
        'W' : 133,
        'U' : 111, # up arrow
        'D' : 116, # down arrow
        'L' : 113, # left arrow
        'R' : 114, # right arrow
        '1' : 67   # f1
        }

    _special_X_keysyms = {
        ' ' : "space",
        '\t' : "Tab",
        '\n' : "Return",  # for some reason this needs to be cr, not lf
        '\r' : "Return",
        '\e' : "Escape",
        '!' : "exclam",
        '#' : "numbersign",
        '%' : "percent",
        '$' : "dollar",
        '&' : "ampersand",
        '"' : "quotedbl",
        '\'' : "apostrophe",
        '(' : "parenleft",
        ')' : "parenright",
        '*' : "asterisk",
        '=' : "equal",
        '+' : "plus",
        ',' : "comma",
        '-' : "minus",
        '.' : "period",
        '/' : "slash",
        ':' : "colon",
        ';' : "semicolon",
        '<' : "less",
        '>' : "greater",
        '?' : "question",
        '@' : "at",
        '[' : "bracketleft",
        ']' : "bracketright",
        '\\' : "backslash",
        '^' : "asciicircum",
        '_' : "underscore",
        '`' : "grave",
        '{' : "braceleft",
        '|' : "bar",
        '}' : "braceright",
        '~' : "asciitilde"
        }
    
    def __init__(self):
        self._display = Display()

    def press(self, keys):
        """
        Send key press events for every key in the 'keys' string.
        """
        self.__perform_on_keys(keys, X.KeyPress)            
        sleep(0.2)

    def release(self, keys):
        """
        Send key release events for every key in the 'keys' string.
        """
        self.__perform_on_keys(keys, X.KeyRelease)
        sleep(0.2)
        
    def press_and_release(self, keys):
        """
        Send key press events for every key in the 'keys' string, then send
        key release events for every key in the 'keys' string. 

        This method is not appropriate for simulating a user typing a string
        of text, since it presses all the keys, and then releases them all. 
        """
        self.press(keys)
        self.release(keys)

    def type(self, keys):
        """
        Simulate a user typing the keys specified in 'keys'. 

        Each key will be pressed and released before the next key is processed. If
        you need to simulate multiple keys being pressed at the same time, use the 
        'press_and_release' method above.
        """
        for key in keys:
            self.press(key)
            self.release(key)

    def __perform_on_keys(self, keys, event):
        control_key = False
        keycode = 0
        shift_mask = 0

        for index in range(len(keys)):
            key = keys[index]
            if control_key:
                keycode = self._lame_hardcoded_keycodes[key]
                shift_mask = 0
                control_key = False
            elif index < len(keys) and key == '^' and keys[index+1] in self._lame_hardcoded_keycodes:
                control_key = True
                continue
            else:
                keycode, shift_mask = self.__char_to_keycode(key)

            if shift_mask != 0:
                fake_input(self._display, event, 50)

            fake_input(self._display, event, keycode)
        self._display.sync()

    def __get_keysym(self, key) :
        keysym = XK.string_to_keysym(key)
        if keysym == 0 :
            # Unfortunately, although this works to get the correct keysym
            # i.e. keysym for '#' is returned as "numbersign"
            # the subsequent display.keysym_to_keycode("numbersign") is 0.
            keysym = XK.string_to_keysym(self._special_X_keysyms[key])
        return keysym
        
    def __is_shifted(self, key) :
        if key.isupper() :
            return True
        if "~!@#$%^&*()_+{}|:\"<>?".find(key) >= 0 :
            return True
        return False

    def __char_to_keycode(self, key) :
        keysym = self.__get_keysym(key)
        keycode = self._display.keysym_to_keycode(keysym)
        if keycode == 0 :
            print "Sorry, can't map", key
            
        if (self.__is_shifted(key)) :
            shift_mask = X.ShiftMask
        else :
            shift_mask = 0

        return keycode, shift_mask

class Mouse(object):
    '''Wrapper around xlib to make moving the mouse easier'''
    
    def __init__(self):
        self._display = Display()

    @property
    def x(self):
        return self.position()[0]

    @property
    def y(self):
        return self.position()[1]
        
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
        
    def move(self, x, y, animate=True, rate=10):
        '''Moves mouse to location (x, y)'''
        def perform_move(x, y, sync):
            fake_input(self._display, X.MotionNotify, sync, X.CurrentTime, X.NONE, x=x, y=y)
            self._display.sync()
            sleep(0.001)

        if not animate:
            perform_move(x, y, False)
            
        dest_x, dest_y = x, y
        curr_x, curr_y = self.position()
        
        # calculate a path from our current position to our destination
        dy = float(curr_y - dest_y)
        dx = float(curr_x - dest_x)
        slope = dy/dx if dx > 0 else 0
        yint = curr_y - (slope * curr_x)
        xscale = rate if dest_x > curr_x else -rate

        while (int(curr_x) != dest_x):
            target_x = min(curr_x + xscale, dest_x) if dest_x > curr_x else max(curr_x + xscale, dest_x)
            perform_move(target_x - curr_x, 0, True)
            curr_x = target_x;
            
        if (curr_y != dest_y):
            yscale = rate if dest_y > curr_y else -rate
            while (curr_y != dest_y):
                target_y = min(curr_y + yscale, dest_y) if dest_y > curr_y else max(curr_y + yscale, dest_y)
                perform_move(0, target_y - curr_y, True)
                curr_y = target_y
                
    def position(self):
        '''Returns the current position of the mouse pointer'''
        coord = self._display.screen().root.query_pointer()._data
        x, y = coord["root_x"], coord["root_y"]
        return x, y
    
    def reset(self):
        self.move(16, 13, animate=False)
        self.click()
        self.move(800, 500, animate=False)


class ScreenGeometry:
    """Get details about screen geometry."""

    def __init__(self):
        self._default_screen = gtk.gdk.screen_get_default()

    def get_num_monitors(self):
        """Get the number of monitors attached to the PC."""
        return self._default_screen.get_n_monitors()

    def get_monitor_geometry(self, monitor_number):
        """Get the geometry for a particular monitor.

        Returns a tuple containing (x,y,width,height).

        """
        if monitor_number >= self.get_num_monitors():
            raise ValueError('Specified monitor number is out of range.')
        return tuple(self._default_screen.get_monitor_geometry(monitor_number))

    def move_mouse_to_monitor(self, monitor_number):
        """Move the mouse to the center of the specified monitor."""
        geo = self.get_monitor_geometry(monitor_number)
        x = geo[0] + (geo[2]/2)
        y = geo[1] + (geo[3]/2)
        Mouse().move(x,y, False) #dont animate this or it might not get there due to barriers
