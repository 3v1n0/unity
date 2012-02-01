# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This script is designed to run unity in a test drive manner. It will drive
# X and test the GL calls that Unity makes, so that we can easily find out if
# we are triggering graphics driver/X bugs.

"""Various utility classes and functions that are useful when running tests."""

from Xlib import X, display, protocol

_display = display.Display()


def make_window_skip_taskbar(window, set_flag=True):
    """Set the skip-taskbar kint on an X11 window.

    'window' should be an Xlib window object.
    set_flag should be 'True' to set the flag, 'False' to clear it.

    """
    state = _display.get_atom('_NET_WM_STATE_SKIP_TASKBAR', 1)
    action = int(set_flag)
    if action == 0:
        print "Clearing flag"
    elif action == 1:
        print "Setting flag"
    _setProperty('_NET_WM_STATE',[action, state, 0, 1], window)
    _display.sync()


def _setProperty(_type, data, win=None, mask=None):
    """ Send a ClientMessage event to a window"""
    if not win: win = _display.screen().root
    if type(data) is str:
        dataSize = 8
    else:
        data = (data+[0]*(5-len(data)))[:5]
        dataSize = 32

    ev = protocol.event.ClientMessage(window=win, client_type=_display.get_atom(_type), data=(dataSize, data))

    if not mask:
        mask = (X.SubstructureRedirectMask|X.SubstructureNotifyMask)
    _display.screen().root.send_event(ev, event_mask=mask)
