# Copyright 2011 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License version 3, as published 
# by the Free Software Foundation.

"Various classes for interacting with BAMF."

import dbus

__all__ = ["Bamf", "BamfApplication"]

class _BamfAccess:
    bamf_bus_name = 'org.ayatana.bamf'
    matcher_path = '/org/ayatana/bamf/matcher'
    matcher_interface = 'org.ayatana.bamf.matcher'
    bus = dbus.SessionBus()
    matcher_proxy_obj = bus.get_object(bamf_bus_name, matcher_path)
    matcher_interface = dbus.Interface(matcher_proxy_obj, matcher_interface)

class Bamf:
    """
    High-level class for interacting with Bamf from within a test. Use this class
    to inspect the state of running applications and open windows.
    """

    def get_running_applications(self):
        """
        Get a list of the currently running applications.
        """
        apps = _BamfAccess.matcher_interface.RunningApplications()
        return [BamfApplication(p) for p in apps]

class BamfApplication:
    """
    Represents an application, with information as returned by Bamf. Don't instantiate 
    this class yourself. instead, use the methods as provided by the Bamf class.
    """
    def __init__(self, bamf_app_path):
        self.bamf_app_path = bamf_app_path
        self.app_proxy = _BamfAccess.bus.get_object(_BamfAccess.bamf_bus_name, bamf_app_path)
        self.view_iface = dbus.Interface(self.app_proxy, 'org.ayatana.bamf.view')

    @property
    def name(self):
        """
        Get the application name.
        """
        return self.view_iface.Name()

    @property
    def is_active(self):
        """
        Is the application active (i.e.- has keyboard focus)?
        """
        return self.view_iface.IsActive()

    @property
    def is_urgent(self):
        """
        Is the application currently signalling urgency?
        """
        return self.view_iface.IsUrgent()

    @property
    def user_visible(self):
        """
        Is this application visible to the user? Some applications (such as the panel) are
        hidden to the user but will still be returned by bamf.
        """
        return self.view_iface.UserVisible()

    def get_windows(self):
        """
        Get a list of the application windows.
        """
        return []

class BamfWindow:
    """
    Represents an application window, as returned by Bamf. Don't instantiate
    this class yourself. Instead, use the appropriate methods in BamfApplication.
    """
    def __init__(self, window_path):
        self.bamf_win_path = window_path
        self.app_proxy = _BamfAccess.bus.get_object(_BamfAccess.bamf_bus_name, window_path)
        self.window_iface = dbus.Interface(self.app_proxy, 'org.ayatana.bamf.window')
        self.view_iface = dbus.Interface(self.app_proxy, 'org.ayatana.bamf.view')

    @property
    def x_id(self):
        """
        Get the X11 Window Id.
        """
        return self.window_iface.GetXid()

    @property
    def window_name(self):
        """
        Get the window name. This may be different from the application name.
        """
        return self.view_iface.Name()

    