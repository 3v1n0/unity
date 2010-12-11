/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACE_ENTRY_REMOTE_H
#define PLACE_ENTRY_REMOTE_H

#include <glib.h>
#include <gio/gio.h>

#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include "Place.h"
#include "PlaceEntry.h"

class PlaceEntryRemote : public PlaceEntry
{
public:
  PlaceEntryRemote (Place *parent);
  ~PlaceEntryRemote ();

  void InitFromKeyFile (GKeyFile *key_file, const gchar *group);
  
  /* Overrides */
  const gchar * GetName ();
  const gchar * GetIcon        ();
  const gchar * GetDescription ();

  guint32        GetPosition  ();
  const gchar ** GetMimetypes ();

  const std::map<gchar *, gchar *>& GetHints ();

  bool IsSensitive  ();
  bool IsActive     ();
  bool ShowInGlobal ();
  bool IsVisible    ();

  /* Other methods */
  Place * GetParent ();
  bool IsValid ();

private:
  Place   *_parent;
  gchar   *_dbus_path;
  gchar   *_name;
  gchar   *_icon;
  gchar   *_description;
  guint32  _position;
  gchar  **_mimetypes;
  std::map<gchar *, gchar *> _hints;
  bool     _sensitive;
  bool     _active;
  bool     _valid;
  bool     _show_in_global;
  bool     _visible;
};

#endif // PLACE_REMOTE_H
