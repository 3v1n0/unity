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

#ifndef PLACE_ENTRY_HOME_H
#define PLACE_ENTRY_HOME_H

#include <glib.h>
#include <gio/gio.h>

#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include <dee.h>

#include "PlaceFactory.h"
#include "PlaceEntry.h"

class PlaceEntryHome : public PlaceEntry
{
public:
  PlaceEntryHome (PlaceFactory *factory);
  ~PlaceEntryHome ();

   /* Overrides */
  const gchar * GetId          ();
  const gchar * GetName        ();
  const gchar * GetIcon        ();
  const gchar * GetDescription ();
  guint64       GetShortcut    ();

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

  DeeModel * GetGlobalResultsModel () { return NULL; };
  DeeModel * GetGlobalGroupsModel () { return NULL; };

private:
  void LoadExistingEntries ();
  void OnPlaceAdded (Place *place);
  void OnPlaceEntryAdded (PlaceEntry *entry);
  void RefreshEntry (PlaceEntry *entry);

  static void OnResultAdded (DeeModel *model, DeeModelIter *iter, PlaceEntryHome *self);
  static void OnResultRemoved (DeeModel *model, DeeModelIter *iter, PlaceEntryHome *self);

  // FIXME: I know this is horrible but I can't fix it this week, have a much better plan for next
public:
  PlaceFactory *_factory;
  DeeModel     *_sections_model;
  DeeModel     *_groups_model;
  DeeModel     *_results_model;

  std::map<char *, gchar *> _hints;

  std::map<DeeModel *, int> _model_to_group;
  std::map<DeeModel *, DeeModel *> _model_to_group_model;
  std::vector<PlaceEntry *> _entries;
};

#endif // PLACE_ENTRY_HOME_H
