# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2011 Canonical
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
A collection of emulators that make it easier to interact with Unity.
"""

from compizconfig import Setting
from compizconfig import Plugin
import dbus
from time import sleep

from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.globals import global_context

class Unity(object):
    '''
    High level class to abstract interactions with the unity shell.

    This class should not be used directly. Instead, use one of the derived classes to
    interact with a different piece of the Unity system.
    '''

    def __init__(self):
        ## TODO
        # check that unity is running
        # acquire the debugging dbus object
        self.UNITY_BUS_NAME = 'com.canonical.Unity'
        self.DEBUG_PATH = '/com/canonical/Unity/Debug'
        self.INTROSPECTION_IFACE = 'com.canonical.Unity.Debug.Introspection'
        self._mouse = Mouse()
        self._keyboard = Keyboard()

        self._bus = dbus.SessionBus()
        self._debug_proxy_obj = self._bus.get_object(self.UNITY_BUS_NAME,
                                                     self.DEBUG_PATH)
        self._introspection_iface = dbus.Interface(self._debug_proxy_obj,
                                                   self.INTROSPECTION_IFACE)

    def get_state(self, piece='/Unity'):
        '''
        returns a full dump of unity's state via the introspection interface
        '''
        return self._introspection_iface.GetState(piece)


class Launcher(Unity):
    """
    Interact with the unity Launcher.
    """

    def __init__(self):
        super(Launcher, self).__init__()
        # set up base launcher vars
        self.x = 0
        self.y = 120
        self.width = 0
        self.height = 0
        self.show_timeout = 1
        self.hide_timeout = 1

    def move_mouse_to_reveal_pos(self):
        self._mouse.move(self.x, self.y)
        sleep(self.show_timeout)

    def move_mouse_outside_of_boundry(self):
        self._mouse.move(self.x + (self.width *2), self._mouse.y)
        sleep(self.hide_timeout)

    def is_showing(self):
        state = self.__get_state()
        return not bool(state['hidden'])
    
    def __get_state(self):
        # get the state for the 'launcher' piece
        return super(Launcher, self).get_state('/Unity/Launcher')[0]


class UnityLauncherIconTooltip(Unity):
    """
    Interact with the Launcher Icon Tooltips?
    """
    RENDER_TIMEOUT_MS = 0
    SHOW_X_POS = 0
    SHOW_Y_POS = 32
    HIDE_X_POS = 64
    HIDE_Y_POS = 0
    WIDTH = 0
    HEIGHT = 0

    def __init__(self, TooltipText=None):
        self.mouse = Mouse()

    def setUp(self):
        self.mouse.move(200, 200)

    def show_on_mouse_hover(self):
        self.mouse.move(self.SHOW_X_POS, self.SHOW_Y_POS)
        sleep(self.RENDER_TIMEOUT_MS)

    def hide_on_mouse_out(self):
        self.mouse.move(self.HIDE_X_POS, self.HIDE_Y_POS)

    def isRendered(self):
        return False


class Switcher(Unity):
    """
    Interact with the Unity switcher.
    """

    def __init__(self):
        super(Switcher, self).__init__()

    def initiate(self):
        self._keyboard.press('^A^T')
        self._keyboard.release('^T')

    def initiate_detail_mode(self):
        self._keyboard.press('^A`')
        self._keyboard.release('`')

    def terminate(self):
        self._keyboard.release('^A')

    def next_icon(self):
        self._keyboard.press_and_release('^T')

    def previous_icon(self):
        self._keyboard.press_and_release('^S^T')

    def show_details(self):
        self._keyboard.press_and_release('`')

    def hide_details(self):
        self._keyboard.press_and_release('^U')

    def next_detail(self):
        self._keyboard.press_and_release('`')

    def previous_detail(self):
        self._keyboard.press_and_release('^S`')

    def __get_icon(self, index):
        import ipdb; ipdb.set_trace()
        return self.get_state('/Unity/SwitcherController/SwitcherModel')[0]['children-of-men'][index][1][0]

    def get_icon_name(self, index):
        return self.__get_icon(index)['tooltip-text']

    def get_icon_desktop_file(self, index):
        try:
            return self.__get_icon(index)['desktop-file']
        except:
            return None

    def get_model_size(self):
        return len(self.get_state('/Unity/SwitcherController/SwitcherModel')[0]['children-of-men'])

    def get_selection_index(self):
        return int(self.get_state('/Unity/SwitcherController/SwitcherModel')[0]['selection-index'])

    def get_last_selection_index(self):
        return bool(self.get_state('/Unity/SwitcherController/SwitcherModel')[0]['last-selection-index'])

    def get_is_visible(self):
        return bool(self.get_state('/Unity/SwitcherController')[0]['visible'])

class Dash(Unity):
	"""
	An emulator class that makes it easier to interact with the unity dash.
	"""

	def __init__(self):
		self.plugin = Plugin(global_context, "unityshell")
		self.setting = Setting(self.plugin, "show_launcher")
		super(Dash, self).__init__()

	def toggle_reveal(self):
		"""
		Reveals the dash if it's currently hidden, hides it otherwise.
		"""
		self._keyboard.press_and_release("^W")
		sleep(1)

	def ensure_visible(self):
		"""
		Ensures the dash is visible.
		"""
		if not self.get_is_visible():
			self.toggle_reveal();

	def ensure_hidden(self):
		"""
		Ensures the dash is hidden.
		"""
		if self.get_is_visible():
			self.toggle_reveal();

	def get_is_visible(self):
		"""
		Is the dash visible?
		"""
        return bool(self.get_state("/Unity/DashController")[0]["visible"])

	def get_search_string(self):
		"""
		Return the current dash search bar search string.
		"""
        return unicode(self.get_state("//SearchBar")[0]['search_string'])
