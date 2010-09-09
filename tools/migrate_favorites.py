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

def get_desktop_dir():
    ''' no python binding from xdg to get the desktop directory? '''
    
    possible_desktop_folder = None
    try:
        for line in file(os.path.expanduser('~/.config/user-dirs.dirs')):
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

def register_new_app(client, launcher_location, apps_list, priority_position):
    key_name = 'app-%s' % launcher_location.split('/')[-1]
    # default distribution launcher don't begin with / and don't have a desktop file in ~/.gnome2
    if os.path.exists(launcher_location) and key_name not in apps_list:
        apps_list.append(key_name)
        app_gconf_key_path = '/desktop/unity/launcher/favorites/%s' % key_name
        client.set_string('%s/desktop_file' % app_gconf_key_path, launcher_location)
        client.set_float('%s/priority' % app_gconf_key_path, priority_position)
        client.set_string('%s/type' % app_gconf_key_path, 'application')
        priority_position += 1
        #print key_name
        #print launcher_location
        #print priority_position
    return (apps_list, priority_position)

if os.path.exists(os.path.expanduser('~/.gconf/desktop/unity/')):
    print "migration already done or unity used before migration support"
    sys.exit(0)

client = gconf.client_get_default()

# import current defaults
apps_list = client.get_list('/desktop/unity/launcher/favorites/favorites_list', gconf.VALUE_STRING)
priority_position = len(apps_list) + 1

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
            (apps_list, priority_position) = register_new_app(client, launcher_location, apps_list, priority_position)


# get UNE lucid panel favorites and convert them
lucid_favorites_list = client.get_list('/apps/netbook-launcher/favorites/favorites_list', gconf.VALUE_STRING)
for candidate in lucid_favorites_list:
    candidate_path = '/apps/netbook-launcher/favorites/%s' % candidate
    if (client.get_string('%s/type' % candidate_path) == 'application'):
        launcher_location = client.get_string('%s/desktop_file' % candidate_path)
        if launcher_location:     
            (apps_list, priority_position) = register_new_app(client, launcher_location, apps_list, priority_position)
            
# get GNOME desktop launchers
desktop_dir = get_desktop_dir()
for launcher_location in glob.glob('%s/*.desktop' % desktop_dir):
    (apps_list, priority_position) = register_new_app(client, launcher_location, apps_list, priority_position)

# set list of default and new favorites and write everything!
client.set_list('/desktop/unity/launcher/favorites/favorites_list', gconf.VALUE_STRING, apps_list)
#print apps_list
client.clear_cache()
# ugly workaround to force gconf client to actually dump its cache to gconfd
loop = gobject.MainLoop(); gobject.idle_add(loop.quit); loop.run();

