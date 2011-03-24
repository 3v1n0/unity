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

#include "PlaceFactory.h"
#include "PlaceEntry.h"

class PlaceEntryHome : public PlaceEntry
{
public:
  PlaceEntryHome (PlaceFactory *factory);
  ~PlaceEntryHome ();

   /* Overrides */
  Place * GetParent () { return NULL; }

  const gchar * GetId          ();
  const gchar * GetName        ();
  const gchar * GetIcon        ();
  const gchar * GetDescription ();
  const gchar * GetSearchHint  ();
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

  void ForeachSection (SectionForeachCallback slot) {};

  void ForeachGroup  (GroupForeachCallback slot);
  void ForeachResult (ResultForeachCallback slot);

  void ForeachGlobalGroup  (GroupForeachCallback slot) {};
  void ForeachGlobalResult (ResultForeachCallback slot) {};

  void GetResult (const void *id, ResultForeachCallback slot);
  void GetGlobalResult (const void *id, ResultForeachCallback slot) {};

  void ActivateResult (const void *id);
  void ActivateGlobalResult (const void *id) {};

private:
  void LoadExistingEntries ();
  void OnPlaceAdded (Place *place);
  void OnPlaceEntryAdded (PlaceEntry *entry);
  void OnPlaceEntryRemoved (PlaceEntry *entry);
  void RefreshEntry (PlaceEntry *entry);

  void OnResultAdded (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result);
  void OnResultRemoved (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result);
  void OnForeachResult (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result);
  void OnSearchFinished (const char                           *search_string,
                         guint32                               section_id,
                         std::map<const char *, const char *>& hints);

public:
  PlaceFactory *_factory;

  std::map<char *, gchar *> _hints;
  std::vector<PlaceEntry *> _entries;
  std::map<const void *, PlaceEntry *> _id_to_entry;

  ResultForeachCallback _foreach_callback;

  guint       _n_searches_done;
  std::string _last_search;
};

#endif // PLACE_ENTRY_HOME_H
