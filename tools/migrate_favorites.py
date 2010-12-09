#!/usr/bin/python
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Didier Roche
#
# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License version 3, as published 
# by the Free Software Foundation.

import gconf
import glob
import gobject
import os
import sys
import subprocess
from xdg import BaseDirectory

LAST_MIGRATION = '3.2.0'

def get_desktop_dir():
    ''' no python binding from xdg to get the desktop directory? '''
    
    possible_desktop_folder = None
    try:
        for line in file('%s/user-dirs.dirs' % BaseDirectory.xdg_config_home):
            values = line.split('=')
            if values[0] == 'XDG_DESKTOP_DIR':
                try:
                    possible_desktop_folder = values[1][1:-2].replace('$HOME', os.path.expanduser('~'))
                    if os.path.isdir(possible_desktop_folder):
                        return possible_desktop_folder
                    else:
                        possible_desktop_folder = None
                        break
                except IndexError:
                    continue
    except IOError:
        pass
    return os.path.expanduser('~/Desktop')

def register_new_app(launcher_location, apps_list):
    ''' append a new app with full desktop path if valid, take care of dups '''
    
    # default distribution launcher don't go into that function (as don't have an aboslute path)
    entry = ""
    if os.path.exists(launcher_location):
        # try to strip the full path we had in unity mutter if it's part of a xdg path:
        for xdg_dir in BaseDirectory.xdg_data_dirs:
            xdg_app_dir = os.path.join(xdg_dir, "applications", "")
            if launcher_location.startswith(xdg_app_dir):
                candidate_desktop_file = launcher_location.split(xdg_app_dir)[1]
                # if really the xdg path is the path to the launcher
                if not '/' in candidate_desktop_file:
                    entry = candidate_desktop_file
                    break
        if not entry:
            entry = launcher_location
        if entry not in apps_list:
            apps_list.append(entry)

    return apps_list

try:
    migration_level = subprocess.Popen(["gsettings", "get", "com.canonical.Unity.Launcher", "favorite-migration"], stdout=subprocess.PIPE).communicate()[0].strip()[1:-1]
except OSError, e:
    print "Gsettings not executable or not installed, postponing migration. The error was: %s" % e
    sys.exit(1)

if migration_level >= LAST_MIGRATION:
    print "Migration already done"
    sys.exit(0)

client = gconf.client_get_default()

# get current gsettings defaults into a list
defaults_call = subprocess.Popen(["gsettings", "get", "com.canonical.Unity.Launcher", "favorites"], stdout=subprocess.PIPE)
apps_list = [elem.strip()[1:-1] for elem in defaults_call.communicate()[0].strip()[1:-1].split(',')]

# first migration to unity compiz
if migration_level < '3.2.0':

    unity_mutter_favorites_list = client.get_list('/desktop/unity/launcher/favorites/favorites_list', gconf.VALUE_STRING)
    unity_mutter_launcher_ordered = {}
    for candidate in unity_mutter_favorites_list:
        candidate_path = '/desktop/unity/launcher/favorites/%s' % candidate
        if (client.get_string('%s/type' % candidate_path) == 'application'):
            launcher_location = client.get_string('%s/desktop_file' % candidate_path)
            position = client.get_string('%s/desktop_file' % candidate_path)
            if launcher_location:
                # try to preserve the order, will be done in a second loop
                unity_mutter_launcher_ordered[position] = launcher_location
    for launcher_location in unity_mutter_launcher_ordered:    
        apps_list = register_new_app(launcher_location, apps_list)


    # import netbook-launcher favorites and convert them
    lucid_favorites_list = client.get_list('/apps/netbook-launcher/favorites/favorites_list', gconf.VALUE_STRING)
    for candidate in lucid_favorites_list:
        candidate_path = '/apps/netbook-launcher/favorites/%s' % candidate
        if (client.get_string('%s/type' % candidate_path) == 'application'):
            launcher_location = client.get_string('%s/desktop_file' % candidate_path)
            if launcher_location:     
                apps_list = register_new_app(launcher_location, apps_list)

    # get GNOME panel favorites and convert them
    panel_list = client.get_list('/apps/panel/general/toplevel_id_list', gconf.VALUE_STRING)
    candidate_objects = client.get_list('/apps/panel/general/object_id_list', gconf.VALUE_STRING)
    for candidate in candidate_objects:
        candidate_path = '/apps/panel/objects/%s' % candidate
        if (client.get_string('%s/object_type' % candidate_path) == 'launcher-object'
           and client.get_string('%s/toplevel_id' % candidate_path) in panel_list):
            launcher_location = client.get_string('%s/launcher_location' % candidate_path)
            if launcher_location:
                if not launcher_location.startswith('/'):
                    launcher_location = os.path.expanduser('~/.gnome2/panel2.d/default/launchers/%s' % launcher_location)
                apps_list = register_new_app(launcher_location, apps_list)
                
    # get GNOME desktop launchers
    desktop_dir = get_desktop_dir()
    for launcher_location in glob.glob('%s/*.desktop' % desktop_dir):
        # blacklist ubiquity as will have two ubiquity in the netbook live session then
        if not "ubiquity" in launcher_location:
            apps_list = register_new_app(launcher_location, apps_list)

    # Now write to gsettings!
    #print apps_list
    return_code = subprocess.call(["gsettings", "set", "com.canonical.Unity.Launcher", "favorites", str(apps_list)])
    
    if return_code != 0:
        print "Settings fail to transition to new unity compiz"
        sys.exit(1)
        
    # some autumn cleanage (gconf binding for recursive_unset seems broken)
    subprocess.call(["gconftool-2", "--recursive-unset", "/apps/netbook-launcher"])
    subprocess.call(["gconftool-2", "--recursive-unset", "/desktop/unity"])
    print "Settings successfully transitionned to new unity compiz"

# stamp that all went well
subprocess.call(["gsettings", "set", "com.canonical.Unity.Launcher", "favorite-migration", "\'%s\'" % LAST_MIGRATION])
sys.exit(0)

