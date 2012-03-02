# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""Utility functions to get shortcut keybindings for various parts of Unity.

Inside autopilot we deal with keybindings by naming them with unique names. For
example, instead of hard-coding the fact that 'Alt+F2' opens the command lens, we
might call:

>>> keybindings.get('lens_reveal/command')
'Alt+F2'

Keybindings come from two different places:
 1) Keybindings from compiz. We can get these if we have the plugin name and
    setting name.
 2) Elsewhere. Right now we're hard-coding these in a separate dictionary.
"""

from compizconfig import Plugin, Setting
import logging
from types import NoneType
import re

from autopilot.emulators.X11 import Keyboard
from autopilot.globals import global_context


logger = logging.getLogger(__name__)


#
# Fill this dictionary with keybindings we want to store.
#
# If keybindings are from compizconfig, the value should be a 2-value tuple
# containging (plugin_name, setting_name).
#
# If keybindings are elsewhere, just store the keybinding string.
_keys = {
    # Launcher:
    "launcher/reveal": ('unityshell', 'show_launcher'),
    "launcher/keynav": ('unityshell', 'keyboard_focus'),
    "launcher/keynav/next": "Down",
    "launcher/keynav/prev": "Up",
    "launcher/keynav/activate": "Enter",
    "launcher/keynav/exit": "Escape",
    "launcher/switcher": ('unityshell', 'launcher_switcher_forward'),
    "launcher/switcher/next": "Tab",
    "launcher/switcher/prev": "Shift+Tab",
    "launcher/keynav/open-quicklist": "Right",
    "launcher/keynav/close-quicklist": "Left",
    # Dash:
    "dash/reveal": "Super",
    # Lenses
    "lens_reveal/command": ("unityshell", "execute_command"),
    "lens_reveal/apps": "Super+a",
    "lens_reveal/files": "Super+f",
    "lens_reveal/music": "Super+m",
    # Hud
    "hud/reveal": ("unityshell", "show_hud"),
    # Switcher:
    "switcher/reveal_normal": ("unityshell", "alt_tab_forward"),
    "switcher/reveal_details": "Alt+`",
    "switcher/reveal_all": ("unityshell", "alt_tab_forward_all"),
    "switcher/cancel": "Escape",
    # Shortcut Hint:
    "shortcuthint/reveal": ('unityshell', 'show_launcher'),
    "shortcuthint/cancel": "Escape",
    # These are in compiz as 'Alt+Right' and 'Alt+Left', but the fact that it
    # lists the Alt key won't work for us, so I'm defining them manually.
    "switcher/next": "Tab",
    "switcher/prev": "Shift+Tab",
    "switcher/detail_start": "Down",
    "switcher/detail_stop": "Up",
    "switcher/detail_next": "`",
    "switcher/detail_prev": "`",
    # Workspace switcher (wall):
    "workspace/move_left": ("wall", "left_key"),
    "workspace/move_right": ("wall", "right_key"),
    "workspace/move_up": ("wall", "up_key"),
    "workspace/move_down": ("wall", "down_key"),
    # Window management
    "window/minimize": ("core", "minimize_window_key"),
    # expo plugin:
    "expo/start": ("expo", "expo_key"),
    "expo/cencel": "Escape",
}



def get(binding_name):
    """Get a keyubinding, given it's well-known name.

    binding_name must be a string, or a TypeError will be raised.

    If binding_name cannot be found in the bindings dictionaries, a ValueError
    will be raised.

    """
    if not isinstance(binding_name, basestring):
        raise TypeError("binding_name must be a string.")
    if binding_name not in _keys:
        raise ValueError("Unknown binding name '%s'." % (binding_name))
    v = _keys[binding_name]
    if isinstance(v, basestring):
        return v
    else:
        return _get_compiz_keybinding(v)


def get_hold_part(binding_name):
    """Returns the part of a keybinding that must be held permenantly.

    Use this function to split bindings like "Alt+Tab" into the part that must be
    held down. See get_tap_part for the part that must be tapped.

    Raises a ValueError if the binding specified does not have multiple parts.

    """
    binding = get(binding_name)
    parts = binding.split('+')
    if len(parts) == 1:
        logger.warning("Key binding '%s' does not have a hold part.", binding_name)
        return parts[0]
    return '+'.join(parts[:-1])


def get_tap_part(binding_name):
    """Returns the part of a keybinding that must be tapped.

    Use this function to split bindings like "Alt+Tab" into the part that must be
    held tapped. See get_hold_part for the part that must be held down.

    Raises a ValueError if the binding specified does not have multiple parts.

    """
    binding = get(binding_name)
    parts = binding.split('+')
    if len(parts) == 1:
        logger.warning("Key binding '%s' does not have a tap part.", binding_name)
        return parts[0]
    return parts[-1]


def _get_compiz_keybinding(compiz_tuple):
    """Given a keybinding name, get the keybinding string from the compiz option.

    Raises ValueError if the compiz setting described does not hold a keybinding.
    Raises RuntimeError if the compiz keybinding has been disabled.

    """
    plugin_name, setting_name = compiz_tuple
    plugin = Plugin(global_context, plugin_name)
    setting = Setting(plugin, setting_name)
    if setting.Type != 'Key':
        raise ValueError("Key binding maps to a compiz option that does not hold a keybinding.")
    if not plugin.Enabled:
        logger.warning("Returning keybinding for '%s' which is in un-enabled plugin '%s'",
            setting.ShortDesc,
            plugin.ShortDesc)
    if setting.Value == "Disabled":
        raise RuntimeError("Keybinding '%s' in compiz plugin '%s' has been disabled." %
            setting.ShortDesc, plugin.ShortDesc)

    return _translate_compiz_keystroke_string(setting.Value)


def _translate_compiz_keystroke_string(keystroke_string):
    """Get a string representing the keystroke stored in `keystroke_string`.

    `keystroke_string` is a compizconfig-style keystroke string.

    The returned value is suitable for passing into the Keyboard emulator.

    """
    if not isinstance(keystroke_string, basestring):
        raise TypeError("keystroke string must be a string.")

    translations = {
        'Control': 'Ctrl',
        'Primary': 'Ctrl',
    }
    regex = re.compile('[<>]')
    parts = regex.split(keystroke_string)
    result = []
    for part in parts:
        part = part.strip()
        if part != "" and not part.isspace():
            translated = translations.get(part, part)
            if translated not in result:
                result.append(translated)

    return '+'.join(result)


class KeybindingsHelper(object):
    """A helper class that makes it easier to use unity keybindings."""
    _keyboard = Keyboard()

    def keybinding(self, binding_name, delay=None):
        """Press and release the keybinding with the given name.

        If set, the delay parameter will override the default delay set by the
        keyboard emulator.

        """
        if type(delay) not in (float, NoneType):
            raise TypeError("delay parameter must be a float if it is defined.")
        if delay:
            self._keyboard.press_and_release(get(binding_name), delay)
        else:
            self._keyboard.press_and_release(get(binding_name))

    def keybinding_hold(self, binding_name):
        """Hold down the hold-part of a keybinding."""
        self._keyboard.press(get_hold_part(binding_name))

    def keybinding_release(self, binding_name):
        """Release the hold-part of a keybinding."""
        self._keyboard.release(get_hold_part(binding_name))

    def keybinding_tap(self, binding_name):
        """Tap the tap-part of a keybinding."""
        self._keyboard.press_and_release(get_tap_part(binding_name))

