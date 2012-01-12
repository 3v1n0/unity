# Copyright 2011 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License version 3, as published 
# by the Free Software Foundation.

"Various classes for interacting with BAMF."

import dbus
import wnck

__all__ = ["Bamf", 
        "BamfApplication", 
        "BamfWindow",
        ]


_BAMF_BUS_NAME = 'org.ayatana.bamf'
_session_bus = dbus.SessionBus()

#
# There's a bug in libwnck - you need to access a screen before any of the
# window methods will work. see lp:914665
__default_screen = wnck.screen_get_default()

class Bamf:
    """
    High-level class for interacting with Bamf from within a test. Use this 
    class to inspect the state of running applications and open windows.
    """

    def __init__(self):
        matcher_path = '/org/ayatana/bamf/matcher'
        matcher_interface = 'org.ayatana.bamf.matcher'
        self.matcher_proxy = _session_bus.get_object(_BAMF_BUS_NAME, matcher_path)
        self.matcher_interface = dbus.Interface(self.matcher_proxy, matcher_interface)

    def get_running_applications(self, user_visible_only=True):
        """
        Get a list of the currently running applications.

        If user_visible_only is True (the default), only applications
        visible to the user in the switcher will be returned.
        """
        apps = [BamfApplication(p) for p in self.matcher_interface.RunningApplications()]
        if user_visible_only:
            return filter(lambda(a): a.user_visible, apps)
        return apps

    def get_running_applications_by_title(self, app_title):
        """
        Return a list of currently running applications that have the title 
        `app_title`. This method may return an empty list, if no applications
        are found with the specified title.
        """
        return [a for a in self.get_running_applications() if a.name == app_title]

    def get_open_windows(self, user_visible_only=True):
        """
        Get a list of currently open windows.

        If user_visible_only is True (the default), only applications
        visible to the user in the switcher will be returned.
        """
        windows = [BamfWindow(w) for w in self.matcher_interface.WindowPaths()]
        if user_visible_only:
            return filter(lambda(a): a.user_visible, windows)
        return windows

    def get_open_windows_by_title(self, win_title):
        """
        Get a list of all open windows with a specific window title. This
        method may return an empty list if no currently open windows have the
        specified title.
        """
        return [w for w in self.get_open_windows() if w.title == win_title]

class BamfApplication:
    """
    Represents an application, with information as returned by Bamf. Don't instantiate 
    this class yourself. instead, use the methods as provided by the Bamf class.
    """
    def __init__(self, bamf_app_path):
        self.bamf_app_path = bamf_app_path
        try:
            self._app_proxy = _session_bus.get_object(_BAMF_BUS_NAME, bamf_app_path)
            self._view_iface = dbus.Interface(self._app_proxy, 'org.ayatana.bamf.view')
        except dbus.DBusException, e:
            e.message += 'bamf_app_path=%r' % (bamf_app_path)
            raise


    @property
    def name(self):
        """
        Get the application name.
        """
        return self._view_iface.Name()

    @property
    def is_active(self):
        """
        Is the application active (i.e.- has keyboard focus)?
        """
        return self._view_iface.IsActive()

    @property
    def is_urgent(self):
        """
        Is the application currently signalling urgency?
        """
        return self._view_iface.IsUrgent()

    @property
    def user_visible(self):
        """
        Is this application visible to the user? Some applications (such as the panel) are
        hidden to the user but will still be returned by bamf.
        """
        return self._view_iface.UserVisible()

    def get_windows(self):
        """
        Get a list of the application windows.
        """
        return [BamfWindow(w) for w in self._view_iface.Children()]

    def __repr__(self):
        return "<BamfApplication '%s'>" % (self.name)

class BamfWindow:
    """
    Represents an application window, as returned by Bamf. Don't instantiate
    this class yourself. Instead, use the appropriate methods in BamfApplication.
    """
    def __init__(self, window_path):
        self._bamf_win_path = window_path
        self._app_proxy = _session_bus.get_object(_BAMF_BUS_NAME, window_path)
        self._window_iface = dbus.Interface(self._app_proxy, 'org.ayatana.bamf.window')
        self._view_iface = dbus.Interface(self._app_proxy, 'org.ayatana.bamf.view')

        self._wnck_window = wnck.window_get(self._window_iface.GetXid())


    @property
    def x_id(self):
        """
        Get the X11 Window Id.
        """
        return self._wnck_window.get_xid()

    @property
    def title(self):
        """
        Get the window title. This may be different from the application name.
        """
        return self._wnck_window.get_name()

    @property
    def geometry(self):
        """
        Get the Xlib geometry object for this window. Returns a tuple
        containing (x, y, width, height)
        """
        
        return self._wnck_window.get_geometry()

    @property
    def is_maximized(self):
        """
        Is the window maximized? Maximized in this case means both maximized 
        vertically and horizontally. If a window is only maximized in one 
        direction it is not considered maximized.
        """
        return self._wnck_window.is_maximized()

    @property
    def is_minimized(self):
        """
        Is the window minimized?
        """
        return self._wnck_window.is_minimized()

    @property
    def application(self):
        """
        Get the application that owns this window. This method may return None
        if the window does not have an associated application. The 'desktop' 
        window is one such example.
        """
        # BAMF returns a list of parents since some windows don't have an
        # associated application. For these windows we return none.
        parents = self._view_iface.Parents()
        if parents:
            return BamfApplication(parents[0])
        else:
            return None

    @property
    def user_visible(self):
        """
        Is this window visible to the user in the switcher?
        """
        return self._view_iface.UserVisible()

    @property
    def is_hidden(self):
        """
        Is this window hidden? Windows are hidden when the 'Show Desktop' 
        mode is activated.
        """
        return bool(wnck.WINDOW_STATE_HIDDEN & self._wnck_window.get_state())

    def close(self):
        """
        Close the window.
        """
        self._wnck_window.close(0)

    def __repr__(self):
        return "<BamfWindow '%s'>" % (self.title)