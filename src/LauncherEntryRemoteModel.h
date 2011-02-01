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

  static LauncherEntryRemoteModel* GetDefault ();

  guint                Size ();
  LauncherEntryRemote* LookupByUri (const gchar *app_uri);
  GList*               GetUris ();

  void AddEntry (LauncherEntryRemote *entry);
  void RemoveEntry (LauncherEntryRemote *entry);

  sigc::signal<void, LauncherEntryRemote *> entry_added;
  sigc::signal<void, LauncherEntryRemote *> entry_removed;

private:
  LauncherEntryRemoteModel();
  ~LauncherEntryRemoteModel();

  GDBusConnection *_conn;
  guint            _launcher_entry_dbus_signal_id;
  GHashTable      *_entries_by_uri;
};

#endif // LAUNCHER_ENTRY_REMOTE_MODEL_H
