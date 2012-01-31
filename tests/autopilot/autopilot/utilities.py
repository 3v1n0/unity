# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
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
from Xlib.error import ConnectionClosedError
import threading


class WindowThread(threading.Thread):
    """A separate thread of control that creates a window."""

    def __init__(self, skip_tskbar=False):
        """Construct the window thread object.

        if skip_taskbar is set (the default), the window will not appear in the
        task bar or switcher.

        Call start() to create the window.
        """
        super(WindowThread, self).__init__()
        self._skip_taskbar = skip_tskbar
        self._msg = "Skip-Taskbar flag %s set" % ('' if skip_tskbar else 'not')
        self._quit_event = threading.Event()

    def run(self):

        self._display = display.Display()
        self._screen = self._display.screen()
        self._window = self._screen.root.create_window(
            10, 10, 200, 100, 1,
            self._screen.root_depth,
            background_pixel=self._screen.white_pixel,
            event_mask=X.ExposureMask | X.KeyPressMask,
            )
        self._gc = self._window.create_gc(
            foreground=self._screen.black_pixel,
            background=self._screen.white_pixel,
            )

        self._window.map()

        # set the SKIP_TASKBAR flag maybe:
        if self._skip_taskbar:
            state = self._display.get_atom('_NET_WM_STATE_SKIP_TASKBAR', 1)
            # first param: 0=remove,1=set,2=toggle
            # 2nd, 3rd param: states you want to manipulate, or 0
            data = [1, state, 0, 1]
            data = (data+[0]*(5-len(data)))[:5]
            dataSize = 32
            ev = protocol.event.ClientMessage(window=self._window,
                                            client_type=self._display.get_atom('_NET_WM_STATE'),
                                            data=(dataSize, data)
                                            )
            self._screen.root.send_event(ev, event_mask=(X.SubstructureRedirectMask|X.SubstructureNotifyMask))

        try:
            while not self.quit_event.wait(0.1):
                while self._display.pending_events() > 0:
                    e = self._display.next_event()

                    if e.type == X.Expose:
                        self._window.draw_text(self._gc, 10, 50, self._msg)
                    elif e.type == X.DestroyNotify:
                        return
        except ConnectionClosedError:
            return
        self._window.unmap()
        self._display.sync()

    def close(self):
        """Close the window."""
        self._quit_event.set()

