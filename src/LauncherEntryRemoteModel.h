// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 */

#ifndef LAUNCHER_ENTRY_REMOTE_MODEL_H
#define LAUNCHER_ENTRY_REMOTE_MODEL_H

#include <glib.h>
#include <sigc++/sigc++.h>

#include "LauncherEntryRemote.h"

class LauncherEntryRemoteModel : public sigc::trackable
{

public:
    LauncherEntryRemoteModel();
    ~LauncherEntryRemoteModel();

    void OnUpdateReceived (const gchar *app_uri, GHashTable *props);

    int Size ();
    std::list<LauncherEntryRemote*>::iterator begin ();
    std::list<LauncherEntryRemote*>::iterator end ();
    std::list<LauncherEntryRemote*>::reverse_iterator rbegin ();
    std::list<LauncherEntryRemote*>::reverse_iterator rend ();

    sigc::signal<void, LauncherEntryRemote *> entry_added;
    sigc::signal<void, LauncherEntryRemote *> entry_removed;

private:
    GDBusConnection *conn;
    guint            launcher_entry_dbus_signal_id;

    std::list<LauncherEntryRemote*> _entries;

    void AddEntry (LauncherEntryRemote *entry);
    void RemoveEntry (LauncherEntryRemote *entry);
};

#endif // LAUNCHER_ENTRY_REMOTE_MODEL_H
