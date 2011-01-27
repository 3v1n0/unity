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

#include <dee.h>

#include "PlaceRemote.h"
#include "PlaceEntry.h"

class PlaceEntryRemote : public PlaceEntry
{
public:
  PlaceEntryRemote (const gchar *dbus_name);
  ~PlaceEntryRemote ();

  void InitFromKeyFile (GKeyFile *key_file, const gchar *group);
  
  /* Overrides */
  const gchar * GetId          ();
  const gchar * GetName        ();
  const gchar * GetIcon        ();
  const gchar * GetDescription ();

  guint32        GetPosition  ();
  const gchar ** GetMimetypes ();

  const std::map<gchar *, gchar *>& GetHints ();

  bool IsSensitive    ();
  bool IsActive       ();
  bool ShowInLauncher ();
  bool ShowInGlobal   ();

  void SetActive        (bool is_active);
  void SetSearch        (const gchar *search, std::map<gchar*, gchar*>& hints);
  void SetActiveSection (guint32 section_id);
  void SetGlobalSearch  (const gchar *search, std::map<gchar*, gchar*>& hints);

  DeeModel * GetSectionsModel ();
  DeeModel * GetGroupsModel ();
  DeeModel * GetResultsModel ();

  /* Other methods */
  bool          IsValid ();
  const gchar * GetPath ();
  void          Connect ();

  void          OnServiceProxyReady (GObject *source, GAsyncResult *res);

  void Update (const gchar  *dbus_path,
               const gchar  *name,
               const gchar  *icon,
               guint32       position,
               GVariantIter *mimetypes,
               gboolean      sensitive,
               const gchar  *sections_model_name,
               GVariantIter *hints,
               const gchar  *entry_renderer,
               const gchar  *entry_groups_model,
               const gchar  *entry_results_model,
               GVariantIter *entry_hints,
               const gchar  *global_renderer,
               const gchar  *global_groups_model,
               const gchar  *global_results_model,
               GVariantIter *global_hints);

public:
  // For our parents use, we don't touch it
  bool dirty;

private:
  gchar   *_dbus_name;
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
  bool     _show_in_launcher;
  bool     _show_in_global;

  GDBusProxy *_proxy;

  DeeModel *_sections_model;
  DeeModel *_groups_model;
  DeeModel *_results_model;
};

#endif // PLACE_ENTRY_REMOTE_H
