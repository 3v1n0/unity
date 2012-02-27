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
import re

from autopilot.globals import global_context


logger = logging.getLogger(__name__)


# Fill this dictionary for all keybindings stored in compizconfig. The key is the
# name used in emulators & tests. The value is a tuple of:
# (compiz plugin name, setting_name)
# The plugin name is probably either 'unityshell', or 'core' (for options that
# appear under "General Options" in CCSM).
_compiz_keys = {
    "launcher/reveal": ('unityshell', 'show_launcher'),
}


# For key bindings that aren't stored in compizconfig, fill this dictionary.
_other_keys = {
    "lens_reveal/command": "Alt+F2",
}


def get(binding_name):
    """Get a keyubinding, given it's well-known name.

    binding_name must be a string, or a TypeError will be raised.

    If binding_name cannot be found in the bindings dictionaries, a ValueError
    will be raised.

    """
    if not isinstance(binding_name, basestring):
        raise TypeError("binding_name must be a string.")
    if _compiz_keys.has_key(binding_name):
        return _get_compiz_keybinding(binding_name)
    elif _other_keys.has_key(binding_name):
        return _other_keys[binding_name]
    else:
        raise ValueError("Unknown binding name '%s'." % (binding_name))


def _get_compiz_keybinding(binding_name):
    """Given a keybinding name, get the keybinding string from the compiz option.

    Raises ValueError if the compiz setting described does not hold a keybinding.
    Raises RuntimeError if the compiz keybinding has been disabled.

    """
    plugin_name, setting_name = _compiz_keys[binding_name]
    plugin = Plugin(global_context, plugin_name)
    setting = Setting(plugin, setting_name)
    if setting.Type != 'Key':
        raise ValueError("Key binding '%s' maps to a compiz option that does not hold a keybinding." % (binding_name))
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
