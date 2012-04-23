# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Marco Trevisan (Treviño)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

import logging

from autopilot.emulators.unity import UnityIntrospectionObject
from autopilot.keybindings import KeybindingsHelper


logger = logging.getLogger(__name__)


class ShortcutController(UnityIntrospectionObject, KeybindingsHelper):
    """ShortcutController proxy class."""

    def show(self):
        """Push the keys necessary to reveal the shortcut hint, but don't block."""
        logger.debug("Revealing shortcut hint with keyboard.")
        self.keybinding_hold("shortcuthint/reveal")

    def ensure_visible(self):
        """Block until the shortcut hint is visible."""
        if not self.visible:
            logger.debug("Revealing shortcut hint with keyboard.")
            self.keybinding_hold("shortcuthint/reveal")
            self.visible.wait_for(True)

    def hide(self):
        """Release the keys keeping the shortcut hint open, but don't block."""
        logger.debug("Un-revealing shortcut hint with keyboard.")
        self.keybinding_release("shortcuthint/reveal")

    def ensure_hidden(self):
        """Block until the shortcut hint is hidden."""
        if self.visible:
            logger.debug("Un-revealing shortcut hint with keyboard.")
            self.keybinding_release("shortcuthint/reveal")
            self.visible.wait_for(False)

    def cancel(self):
        logger.debug("Hide the shortcut hint with keyboard, without releasing the reveal key.")
        self.keybinding("shortcuthint/cancel")

    def get_show_timeout(self):
        return self.timeout_duration / 1000.0
