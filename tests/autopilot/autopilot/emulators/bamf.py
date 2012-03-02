# Copyright 2011 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"Various classes for interacting with BAMF."

import dbus
import dbus.glib
import gio
import gobject
from Xlib import display, X, protocol

from autopilot.emulators.dbus_handler import session_bus

__all__ = [
    "Bamf",
    "BamfApplication",
    "BamfWindow",
    ]

_BAMF_BUS_NAME = 'org.ayatana.bamf'
_X_DISPLAY = display.Display()


def _filter_user_visible(win):
    """Filter out non-user-visible objects.

    In some cases the DBus method we need to call hasn't been registered yet,
    in which case we do the safe thing and return False.

    """
    try:
        return win.user_visible
    except dbus.DBusException:
        return False


class Bamf(object):
    """High-level class for interacting with Bamf from within a test.

    Use this class to inspect the state of running applications and open
    windows.

    """

    def __init__(self):
        matcher_path = '/org/ayatana/bamf/matcher'
        self.matcher_interface_name = 'org.ayatana.bamf.matcher'
        self.matcher_proxy = session_bus.get_object(_BAMF_BUS_NAME, matcher_path)
        self.matcher_interface = dbus.Interface(self.matcher_proxy, self.matcher_interface_name)

    def get_running_applications(self, user_visible_only=True):
        """Get a list of the currently running applications.

        If user_visible_only is True (the default), only applications
        visible to the user in the switcher will be returned.

        """
        apps = [BamfApplication(p) for p in self.matcher_interface.RunningApplications()]
        if user_visible_only:
            return filter(_filter_user_visible, apps)
        return apps

    def get_running_applications_by_title(self, app_title):
        """Return a list of applications that have the title `app_title`.

        This method may return an empty list, if no applications
        are found with the specified title.

        """
        return [a for a in self.get_running_applications() if a.name == app_title]

    def get_open_windows(self, user_visible_only=True):
        """Get a list of currently open windows.

        If user_visible_only is True (the default), only applications
        visible to the user in the switcher will be returned.

        """

        windows = [BamfWindow(w) for w in self.matcher_interface.WindowPaths()]
        if user_visible_only:
            return filter(_filter_user_visible, windows)
        return windows

    def get_open_windows_by_title(self, win_title):
        """Get a list of all open windows with a specific window title.

        This method may return an empty list if no currently open windows have
        the specified title.

        """
        return [w for w in self.get_open_windows() if w.title == win_title]

    def application_is_running(self, app_name):
        """Detect if an application with a given name is currently running.

        'app_name' is the name of the application you are looking for.
        """
        try:
            return app_name in [a.name for a in self.get_running_applications()]
        except dbus.DBusException:
            return False
    
    def application_is_focused(self, app_name):
        """Detect if an application with given name is currently focused.

        'app_name' is the name of the application you are looking for.
        """
        try:
            found_focused = False
            for app in self.get_running_applications_by_title(app_name):
                if app.is_active:
                    found_focused = True

            return found_focused
        except dbus.DBusException:
            return False

    def wait_until_application_is_running(self, app_name, timeout):
        """Wait until a given application is running.

        'app_name' is the name of the application.
        'timeout' is the maximum time to wait, in seconds. If set to
        something less than 0, this method will wait forever.

        This method returns true once the application is found, or false
        if the application was not found until the timeout was reached.
        """
        # python workaround since you can't assign to variables in the enclosing scope:
        # see on_timeout_reached below...
        found_app = [True]

        # maybe the app is running already?
        if not self.application_is_running(app_name):
            wait_forever = timeout < 0
            gobject_loop = gobject.MainLoop()

            # No, so define a callback to watch the ViewOpened signal:
            def on_view_added(bamf_path, name):
                if bamf_path.split('/')[-1].startswith('application'):
                    app = BamfApplication(bamf_path)
                    if app.name == app_name:
                        gobject_loop.quit()

            # ...and one for when the user-defined timeout has been reached:
            def on_timeout_reached():
                gobject_loop.quit()
                found_app[0] = False
                return False

            # need a timeout? if so, connect it:
            if not wait_forever:
                gobject.timeout_add(timeout * 1000, on_timeout_reached)
            # connect signal handler:
            session_bus.add_signal_receiver(on_view_added, 'ViewOpened')
            # pump the gobject main loop until either the correct signal is emitted, or the
            # timeout happens.
            gobject_loop.run()

        return found_app[0]

    def launch_application(self, desktop_file, wait=True):
        """Launch an application by specifying a desktop file.

        Returns the Gobject process object. if wait is True (the default),
        this method will not return until an instance of this application
        appears in the BAMF application list.
        """
        proc = gio.unix.DesktopAppInfo(desktop_file)
        proc.launch()
        if wait:
            self.wait_until_application_is_running(proc.get_name(), -1)
        return proc


class BamfApplication(object):
    """Represents an application, with information as returned by Bamf.

    Don't instantiate this class yourself. instead, use the methods as
    provided by the Bamf class.

    """
    def __init__(self, bamf_app_path):
        self.bamf_app_path = bamf_app_path
        try:
            self._app_proxy = session_bus.get_object(_BAMF_BUS_NAME, bamf_app_path)
            self._view_iface = dbus.Interface(self._app_proxy, 'org.ayatana.bamf.view')
        except dbus.DBusException, e:
            e.message += 'bamf_app_path=%r' % (bamf_app_path)
            raise

    @property
    def name(self):
        """Get the application name."""
        return self._view_iface.Name()

    @property
    def is_active(self):
        """Is the application active (i.e.- has keyboard focus)?"""
        return self._view_iface.IsActive()

    @property
    def is_urgent(self):
        """Is the application currently signalling urgency?"""
        return self._view_iface.IsUrgent()

    @property
    def user_visible(self):
        """Is this application visible to the user?

        Some applications (such as the panel) are hidden to the user but will
        still be returned by bamf.

        """
        return self._view_iface.UserVisible()

    def get_windows(self):
        """Get a list of the application windows."""
        return [BamfWindow(w) for w in self._view_iface.Children()]

    def __repr__(self):
        return "<BamfApplication '%s'>" % (self.name)


class BamfWindow(object):
    """Represents an application window, as returned by Bamf.

    Don't instantiate this class yourself. Instead, use the appropriate methods
    in BamfApplication.

    """
    def __init__(self, window_path):
        self._bamf_win_path = window_path
        self._app_proxy = session_bus.get_object(_BAMF_BUS_NAME, window_path)
        self._window_iface = dbus.Interface(self._app_proxy, 'org.ayatana.bamf.window')
        self._view_iface = dbus.Interface(self._app_proxy, 'org.ayatana.bamf.view')

        self._xid = int(self._window_iface.GetXid())
        self._x_root_win = _X_DISPLAY.screen().root
        self._x_win = _X_DISPLAY.create_resource_object('window', self._xid)

    @property
    def x_id(self):
        """Get the X11 Window Id."""
        return self._xid

    @property
    def x_win(self):
        """Get the X11 window object of the underlying window."""
        return self._x_win

    @property
    def title(self):
        """Get the window title.

        This may be different from the application name.

        """
        return self._getProperty('_NET_WM_NAME')

    @property
    def geometry(self):
        """Get the geometry for this window.

        Returns a tuple containing (x, y, width, height).

        """

        geometry = self._x_win.get_geometry()
        return (geometry.x, geometry.y, geometry.width, geometry.height)

    @property
    def is_maximized(self):
        """Is the window maximized?

        Maximized in this case means both maximized
        vertically and horizontally. If a window is only maximized in one
        direction it is not considered maximized.

        """
        win_state = self._get_window_states()
        return '_NET_WM_STATE_MAXIMIZED_VERT' in win_state and \
            '_NET_WM_STATE_MAXIMIZED_HORZ' in win_state

    @property
    def application(self):
        """Get the application that owns this window.

        This method may return None if the window does not have an associated
        application. The 'desktop' window is one such example.

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
        """Is this window visible to the user in the switcher?"""
        return self._view_iface.UserVisible()

    @property
    def is_hidden(self):
        """Is this window hidden?

        Windows are hidden when the 'Show Desktop' mode is activated.

        """
        win_state = self._get_window_states()
        return '_NET_WM_STATE_HIDDEN' in win_state

    @property
    def is_focused(self):
        """Is this window focused?"""
        win_state = self._get_window_states()
        return '_NET_WM_STATE_FOCUSED' in win_state

    @property
    def is_valid(self):
        """Is this window object valid?

        Invalid windows are caused by windows closing during the construction of
        this object instance.

        """
        return not self._x_win is None

    def close(self):
        """Close the window."""

        self._setProperty('_NET_CLOSE_WINDOW', [0, 0])

    def __repr__(self):
        return "<BamfWindow '%s'>" % (self.title if self._x_win else str(self._xid))

    def _getProperty(self, _type):
        """Get an X11 property.

        _type is a string naming the property type. win is the X11 window object.

        """
        atom = self._x_win.get_full_property(_X_DISPLAY.get_atom(_type), X.AnyPropertyType)
        if atom:
            return atom.value

    def _setProperty(self, _type, data, mask=None):
        if type(data) is str:
            dataSize = 8
        else:
            # data length must be 5 - pad with 0's if it's short, truncate otherwise.
            data = (data + [0] * (5 - len(data)))[:5]
            dataSize = 32

        ev = protocol.event.ClientMessage(window=self._x_win, client_type=_X_DISPLAY.get_atom(_type), data=(dataSize, data))

        if not mask:
            mask = (X.SubstructureRedirectMask | X.SubstructureNotifyMask)
        self._x_root_win.send_event(ev, event_mask=mask)
        _X_DISPLAY.sync()

    def _get_window_states(self):
        """Return a list of strings representing the current window state."""

        _X_DISPLAY.sync()
        return map(_X_DISPLAY.get_atom_name, self._getProperty('_NET_WM_STATE'))
