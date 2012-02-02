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

from autopilot.emulators.X11 import Keyboard, Mouse, ScreenGeometry
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
        self._screen = ScreenGeometry()

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
        self.x = 0
        self.y = 120
        self.width = 0
        self.height = 0
        self.show_timeout = 1
        self.hide_timeout = 1
        self.grabbed = False

        state = self.__get_state(0)
        self.icon_width = int(state['icon-size'])

    def move_mouse_to_right_of_launcher(self, monitor):
        (x, y, w, h) = self.launcher_geometry(monitor)
        self._mouse.move(x + w + 10, y + h / 2, False)
        sleep(self.show_timeout)

    def move_mouse_over_launcher(self, monitor):
        (x, y, w, h) = self.launcher_geometry(monitor)
        self._screen.move_mouse_to_monitor(monitor);
        self._mouse.move(x + w / 2, y + h / 2);

    def reveal_launcher(self, monitor):
        (x, y, w, h) = self.launcher_geometry(monitor)
        self._mouse.move(x - 1200, y + h / 2)
        sleep(self.show_timeout)

    def keyboard_reveal_launcher(self):
        self._keyboard.press('Super')
        sleep(1)

    def keyboard_unreveal_launcher(self):
        self._keyboard.release('Super')
        sleep(1)

    def grab_switcher(self):
        self._keyboard.press_and_release('Alt+F1')
        self.grabbed = True

    def switcher_enter_quicklist(self):
        if self.grabbed:
            self._keyboard.press_and_release('Right')

    def switcher_exit_quicklist(self):
        if self.grabbed:
            self._keyboard.press_and_release('Left')

    def start_switcher(self):
        self._keyboard.press('Super+Tab')
        self._keyboard.release('Tab')
        sleep(1)

    def end_switcher(self, cancel):
        if cancel:
            self._keyboard.press_and_release('Escape')
            if self.grabbed != True:
                self._keyboard.release('Super')
        else:
            if self.grabbed:
                self._keyboard.press_and_release('\n')
            else:
                self._keyboard.release('Super')
        self.grabbed = False

    def switcher_next(self):
        if self.grabbed:
            self._keyboard.press_and_release('Down')
        else:
            self._keyboard.press_and_release('Tab')

    def switcher_prev(self):
        if self.grabbed:
            self._keyboard.press_and_release('Up')
        else:
            self._keyboard.press_and_release('Shift+Tab')

    def is_quicklist_open(self, monitor):
        state = self.__get_state(monitor)
        return bool(state['quicklist-open'])

    def is_showing(self, monitor):
        state = self.__get_state(monitor)
        return not bool(state['hidden'])

    def key_nav_is_active(self):
        state = self.__get_controller_state()
        return bool(state['key_nav_is_active'])

    def key_nav_monitor(self):
        state = self.__get_controller_state()
        return int(state['key_nav_launcher_monitor'])

    def key_nav_is_grabbed(self):
        state = self.__get_controller_state()
        return bool(state['key_nav_is_grabbed'])

    def key_nav_selection(self):
        state = self.__get_controller_state()
        return int(state['key_nav_selection'])

    def launcher_geometry(self, monitor):
        state = self.__get_state(monitor);
        x = int(state['x'])
        y = int(state['y'])
        width = int(state['width'])
        height = int(state['height'])
        return (x, y, width, height)

    def num_launchers(self):
        return len(super(Launcher, self).get_state('/Unity/LauncherController/Launcher'))

    def __get_controller_state(self):
        return super(Launcher, self).get_state('/Unity/LauncherController')[0]

    def __get_state(self, monitor):
        # get the state for the 'launcher' piece
        return super(Launcher, self).get_state('/Unity/LauncherController/Launcher[monitor=%s]' % (monitor))[0]

    def get_launcher_icons(self):
        """Get a list of launcher icons in this launcher."""
        icons = self.get_state("//Launcher/LauncherIcon")
        return [LauncherIcon(icon_dict) for icon_dict in icons]

    def click_launcher_icon(self, icon, monitor=0, button=1):
        """Move the mouse over the launcher icon, and click it."""
        self.reveal_launcher(monitor)
        self._mouse.move(icon.x, icon.y + (self.icon_width / 2))
        self._mouse.click(button)
        self.move_mouse_to_right_of_launcher(monitor)

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
        self._keyboard.press('Alt')
        self._keyboard.press_and_release('Tab')
        sleep(1)

    def initiate_detail_mode(self):
        """Start detail mode with alt+`"""
        self._keyboard.press('Alt')
        self._keyboard.press_and_release('`')

    def terminate(self):
        """Stop switcher."""
        self._keyboard.release('Alt')

    def next_icon(self):
        """Move to the next application."""
        self._keyboard.press_and_release('Tab')

    def previous_icon(self):
        """Move to the previous application."""
        self._keyboard.press_and_release('Shift+Tab')

    def show_details(self):
        """Show detail mode."""
        self._keyboard.press_and_release('`')

    def hide_details(self):
        """Hide detail mode."""
        self._keyboard.press_and_release('Up')

    def next_detail(self):
        """Move to next detail in the switcher."""
        self._keyboard.press_and_release('`')

    def previous_detail(self):
        """Move to the previous detail in the switcher."""
        self._keyboard.press_and_release('Shift+`')

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

    def get_icon_name(self, index):
        return self.__get_icon(index)['tooltip-text']

    def get_icon_desktop_file(self, index):
        try:
            return self.__get_icon(index)['desktop-file']
        except:
            return None

    def get_model_size(self):
        return len(self.__get_model()['Children'])

    def get_selection_index(self):
        return int(self.__get_model()['selection-index'])

    def get_last_selection_index(self):
        return bool(self.__get_model()['last-selection-index'])

    def get_is_visible(self):
        return bool(self.__get_controller()['visible'])

    def get_switcher_icons(self):
        """Get all icons in the switcher model.

        The switcher needs to be initiated in order to get the model.

        """
        return [LauncherIcon(i) for i in self.get_state('//SwitcherModel/LauncherIcon')]

    def __get_model(self):
        return self.get_state('/Unity/SwitcherController/SwitcherModel')[0]

    def __get_controller(self):
        return self.set_state('/unity/SwitcherController')[0]

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
        self._keyboard.press_and_release("Super")
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

    def get_current_lens(self):
        """Returns the id of the current lens.

        For example, the default lens is 'home.lens', the run-command lens is
        'commands.lens'.

        """
        return unicode(self.get_state("//DashController/DashView/LensBar")[0]['active-lens'])

    def reveal_application_lens(self):
        """Reveal the application lense."""
        self._keyboard.press('Super')
        self._keyboard.press_and_release("a")
        self._keyboard.release('Super')

    def reveal_music_lens(self):
        """Reveal the music lense."""
        self._keyboard.press('Super')
        self._keyboard.press_and_release("m")
        self._keyboard.release('Super')

    def reveal_file_lens(self):
        """Reveal the file lense."""
        self._keyboard.press('Super')
        self._keyboard.press_and_release("f")
        self._keyboard.release('Super')

    def reveal_command_lens(self):
        """Reveal the 'run command' lens."""
        self._keyboard.press_and_release('Alt+F2')
