# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

import logging

from autopilot.keybindings import KeybindingsHelper
from autopilot.emulators.unity import UnityIntrospectionObject


logger = logging.getLogger(__name__)


class ShortcutController(UnityIntrospectionObject, KeybindingsHelper):
    """ShortcutController proxy class."""

    def show(self):
        logger.debug("Revealing shortcut hint with keyboard.")
        self.keybinding_hold("shortcuthint/reveal")

    def hide(self):
        logger.debug("Un-revealing shortcut hint with keyboard.")
        self.keybinding_release("shortcuthint/reveal")

    def cancel(self):
        logger.debug("Hide the shortcut hint with keyboard, without releasing the reveal key.")
        self.keybinding("shortcuthint/cancel")

    def get_geometry(self):
        self.refresh_state()
        return (self.x, self.y, self.width, self.height)

    def get_show_timeout(self):
        self.redresh_state()
        return self.timeout_duration / 1000.0

    def is_enabled(self):
        self.refresh_state()
        return bool(self.enabled)

    def is_visible(self):
        self.refresh_state()
        return bool(self.visible)
