#!/usr/bin/python
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2010 Canonical
# Author: Didier Roche
#
# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License version 3, as published 
# by the Free Software Foundation.

import sys

try:
    import datetime
    import gconf
    import glob
    import gobject
    from gobject import GError
    import os
    import subprocess
    from xdg import BaseDirectory
except ImportError, e:
    print "No migration because of a missing python module: %s" % e
    sys.exit (1)

LAST_MIGRATION = '3.2.10'

def get_log_file():
    ''' open the log file and return it '''

    data_path = "%s/unity" % BaseDirectory.xdg_cache_home
    if not os.path.isdir(data_path):
        os.makedirs(data_path)
    try:
        return open("%s/migration_script.log" % data_path, "a")
    except (IOError, OSError), e:
        print "Can't put log in %s, will print those manually. Error is: %s" % (data_path, e)
        return None

def migrating_chapter_log(name, apps_list, migration_list, log_file):
    '''Log migration of new launchers'''
    
    log(" + Migration for %s.\n   Current app list is: %s\n   Candidates are: %s" % (name, apps_list, migration_list), log_file)


def log(message, log_file):
    ''' log if log_file present'''
    if log_file:
        log_file.write("%s\n" % message)
    else:
        print message

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

def register_new_app(launcher_location, apps_list, log_file):
    ''' append a new app with full desktop path if valid, take care of dups '''
    
    # default distribution launcher don't go into that function (as don't have an aboslute path)
    entry = ""
    if os.path.exists(launcher_location):
        log("    == %s: exists" % launcher_location, log_file)
        # try to strip the full path we had in unity mutter if it's part of a xdg path:
        # or try to get that for other desktop file based on name.
        candidate_desktop_filename = launcher_location.split("/")[-1]
        # some desktop file with modified exec key (like in cairo-dock contains 01desktopfilename.desktop, strip that)
        try:
            candidate_cairodock_desktop_filename = candidate_desktop_filename.split("01")[1]
        except IndexError:
            candidate_cairodock_desktop_filename = ""
        for xdg_dir in BaseDirectory.xdg_data_dirs:
            xdg_app_dir = os.path.join(xdg_dir, "applications", "")
            if launcher_location.startswith(xdg_app_dir):
                candidate_desktop_file = launcher_location.split(xdg_app_dir)[1]
                # if really the xdg path is the path to the launcher
                if not '/' in candidate_desktop_file:
                    entry = candidate_desktop_file
                    log("    Direct match found for system desktop file", log_file)
                    break
            # second chance: try to see if the desktop filename is in xdg path and so, assume it's a match
            if not entry and os.path.exists("%s/%s" % (xdg_app_dir, candidate_desktop_filename)):
                entry = candidate_desktop_filename
                log("    Similar desktop file name with system desktop file", log_file)
                break
            # third chance: try to see if a tweaked cairo-dock like deskto file name is in xdg path
            if not entry and os.path.exists("%s/%s" % (xdg_app_dir, candidate_cairodock_desktop_filename)):
                entry = candidate_cairodock_desktop_filename
                log("    Similar Cairo-Dock -like desktop file name with system desktop file", log_file)
                break
        # fourth and last chance: try to find a corresponding Exec key.
        # Wait! scanning /usr/share/applications is heavy !!!
        # Don't panic, we have the bamf.index for that :)
        if not entry:
            exec_arg = ""
            try:
                for line in open(launcher_location):
                    if "Exec=" in line:
                        exec_arg = line.split("Exec=")[1]
                        break
            except IOError:
                log("    Can't open %s for reading Exec" % launcher_location, log_file)     
            if exec_arg:
                try:
                    for line in open("/usr/share/applications/bamf.index"):
                        if exec_arg in line:
                            entry = line.split()[0]
                            log("    Coherent exec key found with system desktop file", log_file)
                            break
                except IOError:
                    log("    No bamf.index file found on the system!", log_file)
            
        if not entry:
            entry = launcher_location
        log("    %s: real entry is %s" % (launcher_location, entry), log_file)
        if entry not in apps_list:
            log("    --- adding %s as not in app_list" % entry, log_file)
            apps_list.append(entry)
        else:
            log("    --- NOT adding %s as already in app_list" % entry, log_file)
    else:
        log("    == %s: doesn't exist" % launcher_location, log_file)

    return apps_list

def save_gsettings_favorites(apps_list, log_file):
    ''' save the app list favorites to gsettings '''
    
    #print apps_list
    return_code = subprocess.call(["gsettings", "set", "com.canonical.Unity.Launcher", "favorites", str(apps_list)])
    
    if return_code != 0:
        print "Settings fail to transition to new unity compiz favorites"
        log("Settings fail to transition to new unity compiz favorites\n\n", log_file)
        if log_file:
            log_file.close()
        sys.exit(1)

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

log_file = get_log_file()
log("Migration script called on %s\n" % str(datetime.datetime.now()), log_file)

# first migration to unity compiz
if migration_level < '3.2.0':
    log("======= Migration to 3.2.0 =======\n", log_file)

    unity_mutter_favorites_list = client.get_list('/desktop/unity/launcher/favorites/favorites_list', gconf.VALUE_STRING)
    unity_mutter_launcher_ordered = {}
    migrating_chapter_log("unity mutter", apps_list, unity_mutter_favorites_list, log_file)
    for candidate in unity_mutter_favorites_list:
        candidate_path = '/desktop/unity/launcher/favorites/%s' % candidate
        try:
            if (candidate and client.get_string('%s/type' % candidate_path) == 'application'):
                launcher_location = client.get_string('%s/desktop_file' % candidate_path)
                position = client.get_string('%s/desktop_file' % candidate_path)
                if launcher_location:
                    # try to preserve the order, will be done in a second loop
                    unity_mutter_launcher_ordered[position] = launcher_location
        except GError, e:
            log("Dont migrate %s: %s" % (candidate_path, e), log_file)
            continue
    for launcher_location in unity_mutter_launcher_ordered:    
        apps_list = register_new_app(launcher_location, apps_list, log_file)


    # import netbook-launcher favorites and convert them
    lucid_favorites_list = client.get_list('/apps/netbook-launcher/favorites/favorites_list', gconf.VALUE_STRING)
    migrating_chapter_log("netbook-launcher favorites", apps_list, lucid_favorites_list, log_file)
    for candidate in lucid_favorites_list:
        candidate_path = '/apps/netbook-launcher/favorites/%s' % candidate
        try:
            if (candidate and client.get_string('%s/type' % candidate_path) == 'application'):
                try:
                    launcher_location = client.get_string('%s/desktop_file' % candidate_path)
                except GError, e:
                    log("Dont migrate %s: %s" % (candidate_path, e), log_file)
                    continue
                if launcher_location:
                    apps_list = register_new_app(launcher_location, apps_list, log_file)
        except GError, e:
            log("Dont migrate %s: %s" % (candidate_path, e), log_file)
            continue

    # get GNOME panel favorites and convert them
    panel_list = client.get_list('/apps/panel/general/toplevel_id_list', gconf.VALUE_STRING)
    candidate_objects = client.get_list('/apps/panel/general/object_id_list', gconf.VALUE_STRING)
    migrating_chapter_log("gnome-panel items", apps_list, candidate_objects, log_file)
    for candidate in candidate_objects:
        candidate_path = '/apps/panel/objects/%s' % candidate
        try:
            if (candidate and client.get_string('%s/object_type' % candidate_path) == 'launcher-object'
               and client.get_string('%s/toplevel_id' % candidate_path) in panel_list):
                launcher_location = client.get_string('%s/launcher_location' % candidate_path)
                if launcher_location:
                    if not launcher_location.startswith('/'):
                        launcher_location = os.path.expanduser('~/.gnome2/panel2.d/default/launchers/%s' % launcher_location)
                    apps_list = register_new_app(launcher_location, apps_list, log_file)
        except GError, e:
            log("Dont migrate %s: %s" % (candidate_path, e), log_file)
            continue
                
    # get GNOME desktop launchers
    desktop_dir = get_desktop_dir()
    desktop_items = glob.glob('%s/*.desktop' % desktop_dir)
    migrating_chapter_log("desktop items in %s" % desktop_dir, apps_list, desktop_items, log_file)
    for launcher_location in glob.glob('%s/*.desktop' % desktop_dir):
        # blacklist ubiquity as will have two ubiquity in the netbook live session then
        if not "ubiquity" in launcher_location:
            apps_list = register_new_app(launcher_location, apps_list, log_file)

    # Now write to gsettings!
    save_gsettings_favorites(apps_list, log_file)
        
    # some autumn cleanage (gconf binding for recursive_unset seems broken)
    subprocess.call(["gconftool-2", "--recursive-unset", "/apps/netbook-launcher"])
    subprocess.call(["gconftool-2", "--recursive-unset", "/desktop/unity"])

# second migration: transition popular docks entry as well
if migration_level < '3.2.10':
    log("======= Migration to 3.2.10 =======\n", log_file)
    
    # import awn favorites
    awn_favorites_list = client.get_list('/apps/awn-applet-taskmanager/launcher_paths', gconf.VALUE_STRING)
    migrating_chapter_log("awn favorites", apps_list, awn_favorites_list, log_file)
    for launcher_location in awn_favorites_list:
        apps_list = register_new_app(launcher_location, apps_list, log_file)
    
    # import Docky favorites
    dock_list = client.get_list('/apps/docky-2/Docky/DockController/ActiveDocks', gconf.VALUE_STRING)
    for dock in dock_list:
        docky_favorites_list = client.get_list('/apps/docky-2/Docky/Interface/DockPreferences/%s/Launchers' % dock, gconf.VALUE_STRING)
        migrating_chapter_log("Docky favorites for %s" % dock, apps_list, docky_favorites_list, log_file)
        for launcher_location in docky_favorites_list:
            try:
                launcher_location = launcher_location.split("file://")[1]
            except IndexError:
                pass
            apps_list = register_new_app(launcher_location, apps_list, log_file)

    # import Cairo-Dock favorites
    try:
        cairodock_path = "%s/cairo-dock/current_theme/launchers" % BaseDirectory.xdg_config_home
        cairodock_favorites_list = os.listdir(cairodock_path)
        migrating_chapter_log("Cairo-Dock favorites (in %s)" % cairodock_path, apps_list, cairodock_favorites_list, log_file)
        for launcher in cairodock_favorites_list:
            launcher_location = "%s/%s" % (cairodock_path, launcher)
            apps_list = register_new_app(launcher_location, apps_list, log_file)
    except OSError:
        log(" + Can't migrate Cairo-Dock as %s doesn't exist" % cairodock_path, log_file)
        
    # Now write to gsettings!
    save_gsettings_favorites(apps_list, log_file)

log("Migration script ended successfully\n\n", log_file)
if log_file:
    log_file.close()

# stamp that all went well
subprocess.call(["gsettings", "set", "com.canonical.Unity.Launcher", "favorite-migration", "\'%s\'" % LAST_MIGRATION])
sys.exit(0)

