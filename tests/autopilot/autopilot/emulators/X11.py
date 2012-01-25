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

"""A collection of emulators for X11 - namely keyboards and mice. 

In the future we may also need other devices.

"""


from time import sleep

from Xlib import X
from Xlib import XK
from Xlib.display import Display
from Xlib.ext.xtest import fake_input

_PRESSED_KEYS = []
_DISPLAY = Display()

class Keyboard(object):
    """Wrapper around xlib to make faking keyboard input possible."""
    
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
        self.shifted_keys = [k[1] for k in _DISPLAY._keymap_codes if k]

    def press(self, keys, delay=0.2):
        """Send key press events only.

        Keys must be a sequence type (string, tuple, or list)  of keys you want
        pressed. For keys that don't have a single-character represetnation,
        you must use the X11 keysym name. For example:

        press(['Meta_L', 'F2'])

        presses the 'Alt+F2' combination.

        """
        self.__perform_on_keys(keys, X.KeyPress)            
        sleep(delay)

    def release(self, keys, delay=0.2):
        """Send key release events only.

        Keys must be a sequence type (string, tuple, or list)  of keys you want
        pressed. For keys that don't have a single-character represetnation,
        you must use the X11 keysym name. For example:

        release(['Meta_L', 'F2'])

        releases the 'Alt+F2' combination.

        """
        self.__perform_on_keys(keys, X.KeyRelease)
        sleep(delay)
        
    def press_and_release(self, keys, delay=0.2):
        """Press and release all items in 'keys'.

        This is the same as calling 'press(keys);release(keys)'.
        
        Keys must be a sequence type (string, tuple, or list)  of keys you want
        pressed. For keys that don't have a single-character represetnation,
        you must use the X11 keysym name. For example:

        press_and_release(['Meta_L', 'F2'])

        presses the 'Alt+F2' combination, and then releases both keys.

        """
        self.press(keys, delay)
        self.release(keys, delay)

    def type(self, string, delay=0.1):
        """Simulate a user typing a string of text. """
        for key in string:
            self.press(key, delay)
            self.release(key, delay)

    @staticmethod
    def cleanup():
        """Generate KeyRelease events for any un-released keys.

        Make sure you call this at the end of any test to release 
        any keys that were pressed and not released.

        """
        for keycode in _PRESSED_KEYS:
            fake_input(_DISPLAY, X.KeyRelease, keycode)

    def __perform_on_keys(self, keys, event):
        keycode = 0
        shift_mask = 0

        for key in keys:
            keycode, shift_mask = self.__char_to_keycode(key)

            if shift_mask != 0:
                fake_input(_DISPLAY, event, 50)

            if event == X.KeyPress:
                _PRESSED_KEYS.append(keycode)
            elif event == X.KeyRelease:
                _PRESSED_KEYS.remove(keycode)

            fake_input(_DISPLAY, event, keycode)
        _DISPLAY.sync()

    def __get_keysym(self, key) :
        keysym = XK.string_to_keysym(key)
        if keysym == 0 :
            # Unfortunately, although this works to get the correct keysym
            # i.e. keysym for '#' is returned as "numbersign"
            # the subsequent display.keysym_to_keycode("numbersign") is 0.
            keysym = XK.string_to_keysym(self._special_X_keysyms[key])
        return keysym
        
    def __is_shifted(self, key) :
        return len(key) == 1 and ord(key) in self.shifted_keys

    def __char_to_keycode(self, key) :
        keysym = self.__get_keysym(key)
        keycode = _DISPLAY.keysym_to_keycode(keysym)
        if keycode == 0 :
            print "Sorry, can't map", key
            
        if (self.__is_shifted(key)) :
            shift_mask = X.ShiftMask
        else :
            shift_mask = 0
        return keycode, shift_mask


class Mouse(object):
    """Wrapper around xlib to make moving the mouse easier."""

    @property
    def x(self):
        """Mouse position X coordinate."""
        return self.position()[0]

    @property
    def y(self):
        """Mouse position Y coordinate."""
        return self.position()[1]
		
    def press(self, button=1):
        """Press mouse button at current mouse location."""
        fake_input(_DISPLAY, X.ButtonPress, button)
        _DISPLAY.sync()
		
    def release(self, button=1):
        """Releases mouse button at current mouse location."""
        fake_input(_DISPLAY, X.ButtonRelease, button)
        _DISPLAY.sync()
		
    def click(self, button=1):
        """Click mouse at current location."""
        self.press(button)
        sleep(0.25)
        self.release(button)
		
    def move(self, x, y, animate=True):
        """Moves mouse to location (x, y)."""
        def perform_move(x, y):
            fake_input(_DISPLAY, X.MotionNotify, x=x, y=y)
            _DISPLAY.sync()
            sleep(0.001)

        if not animate:
            perform_move(x, y)
			
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
        """Returns the current position of the mouse pointer."""
        coord = _DISPLAY.screen().root.query_pointer()._data
        x, y = coord["root_x"], coord["root_y"]
        return x, y
	
    def reset(self):
        self.move(16, 13, animate=False)
        self.click()
        self.move(800, 500, animate=False)
