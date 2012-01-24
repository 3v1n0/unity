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

"""A collection of emulators that make it easier to interact with Unity."""

from compizconfig import Setting
from compizconfig import Plugin
import dbus
from time import sleep

from autopilot.emulators.X11 import Keyboard, Mouse
from autopilot.globals import global_context

class Unity(object):
    """High level class to abstract interactions with the unity shell.

    This class should not be used directly. Instead, use one of the derived 
    classes to interact with a different piece of the Unity system.

    """

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
        """Returns a full dump of unity's state."""
        return self._introspection_iface.GetState(piece)


class Launcher(Unity):
    """Interact with the unity Launcher."""

    def __init__(self):
        super(Launcher, self).__init__()
        # set up base launcher vars
        state = self.__get_state()
        self.icon_width = int(state['icon-size'])

        self.reveal_pos = (0, 120)
        self.hide_pos = (self.icon_width *2, 120)
        
        self.show_timeout = 1
        self.hide_timeout = 1
        

    def move_mouse_to_reveal_pos(self):
        """Move the mouse to the launcher reveal position."""
        self._mouse.move(*self.reveal_pos)
        sleep(self.show_timeout)

    def move_mouse_outside_of_boundry(self):
        """Move the mouse outside the launcher."""
        self._mouse.move(*self.hide_pos)
        sleep(self.hide_timeout)

    def is_showing(self):
        """Is the launcher showing?"""
        state = self.__get_state()
        return not bool(state['hidden'])
    
    def __get_state(self):
        # get the state for the 'launcher' piece
        return super(Launcher, self).get_state('/Unity/Launcher')[0]

    def get_launcher_icons(self):
        """Get a list of launcher icons in this launcher."""
        icons = self.get_state("//Launcher/LauncherIcon")
        return [LauncherIcon(icon_dict) for icon_dict in icons]

    def click_launcher_icon(self, icon, button=1):
        """Move the mouse over the launcher icon, and click it."""
        self.move_mouse_to_reveal_pos()
        self._mouse.move(icon.x, icon.y + (self.icon_width / 2))
        self._mouse.click(button)
        self.move_mouse_outside_of_boundry()


class LauncherIcon:
    """Holds information about a launcher icon. 

    Do not instantiate an instance of this class yourself. Instead, use the 
    appropriate methods in the Launcher class instead.

    """

    def __init__(self, icon_dict):
        self.tooltip_text = icon_dict['tooltip-text']
        self.x = icon_dict['x']
        self.y = icon_dict['y']
        self.num_windows = icon_dict['related-windows']
        self.visible = icon_dict['quirk-visible']
        self.active = icon_dict['quirk-active']
        self.running = icon_dict['quirk-running']
        self.presented = icon_dict['quirk-presented']
        self.urgent = icon_dict['quirk-urgent']

class Switcher(Unity):
    """Interact with the Unity switcher."""

    def __init__(self):
        super(Switcher, self).__init__()

    def initiate(self):
        """Start the switcher with alt+tab."""
        self._keyboard.press(['Meta_L'])
        self._keyboard.press_and_release(['Tab'])

    def initiate_detail_mode(self):
        """Start detail mode with alt+`"""
        self._keyboard.press(['Meta_L'])
        self._keyboard.press_and_release('`')

    def terminate(self):
        """Stop switcher."""
        self._keyboard.release(['Meta_L'])

    def next_icon(self):
        """Move to the next application."""
        self._keyboard.press_and_release(['Tab'])

    def previous_icon(self):
        """Move to the previous application."""
        self._keyboard.press_and_release(['Shift_L','Tab'])

    def show_details(self):
        """Show detail mode."""
        self._keyboard.press_and_release('`')

    def hide_details(self):
        """Hide detail mode."""
        self._keyboard.press_and_release(['Up'])

    def next_detail(self):
        """Move to next detail in the switcher."""
        self._keyboard.press_and_release('`')

    def previous_detail(self):
        """Move to the previous detail in the switcher."""
        self._keyboard.press_and_release(['Shift_L','`'])

    def __get_icon(self, index):
        return self.__get_model()['Children'][index][1][0]

    @property
    def current_icon(self):
        """Get the currently-selected icon."""
        if not self.get_is_visible:
            return None
        model = self.__get_model()
        sel_idx = self.get_selection_index()
        try:
            return LauncherIcon(model['Children'][sel_idx][1])
        except KeyError:
            return None

    def get_model_size(self):
        return len(self.__get_model()['Children'])

    def get_selection_index(self):
        return int(self.__get_model()['selection-index'])

    def get_last_selection_index(self):
        return bool(self.__get_model()['last-selection-index'])

    def get_is_visible(self):
        return bool(self.__get_controller()['visible'])

    def __get_model(self):
        return self.get_state('/Unity/SwitcherController/SwitcherModel')[0]

    def __get_controller(self):
        return self.set_state('/unity/SwitcherController')[0]

class Dash(Unity):
    """An emulator class that makes it easier to interact with the unity dash."""

    def __init__(self):
        self.plugin = Plugin(global_context, "unityshell")
        self.setting = Setting(self.plugin, "show_launcher")
        super(Dash, self).__init__()

    def toggle_reveal(self):
        """Reveals the dash if it's currently hidden, hides it otherwise."""
        self._keyboard.press_and_release(['Super_L'])
        sleep(1)

    def ensure_visible(self):
        """Ensures the dash is visible."""
        if not self.get_is_visible():
            self.toggle_reveal();

    def ensure_hidden(self):
        """Ensures the dash is hidden."""
        if self.get_is_visible():
            self.toggle_reveal();

    def get_is_visible(self):
        """Is the dash visible?"""
        return bool(self.get_state("/Unity/DashController")[0]["visible"])

    def get_search_string(self):
        """Return the current dash search bar search string."""
        return unicode(self.get_state("//SearchBar")[0]['search_string'])

    def get_current_lens(self):
        """Returns the id of the current lens. 

        For example, the default lens is 'home.lens', the run-command lens is
        'commands.lens'.

        """
        return unicode(self.get_state("//DashController/DashView/LensBar")[0]['active-lens'])

    def reveal_application_lens(self):
        """Reveal the application lense."""
        self._keyboard.press(['Super_L'])
        self._keyboard.press_and_release("a")
        self._keyboard.release(['Super_L'])

    def reveal_music_lens(self):
        """Reveal the music lense."""
        self._keyboard.press(['Super_L'])
        self._keyboard.press_and_release("m")
        self._keyboard.release(['Super_L'])

    def reveal_file_lens(self):
        """Reveal the file lense."""
        self._keyboard.press(['Super_L'])
        self._keyboard.press_and_release("f")
        self._keyboard.release(['Super_L'])

    def reveal_command_lens(self):
        """Reveal the 'run command' lens."""
        self._keyboard.press_and_release(['Alt_L','F2'])
