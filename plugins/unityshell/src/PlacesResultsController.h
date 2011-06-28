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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACES_RESULTS_CONTROLLER_H
#define PLACES_RESULTS_CONTROLLER_H

#include <Nux/Nux.h>

#include "Introspectable.h"
#include "PlaceEntry.h"
#include "PlacesResultsView.h"

#include "PlacesGroupController.h"

class PlacesResultsController : public nux::Object, public unity::Introspectable
{
public:
  PlacesResultsController  ();
  ~PlacesResultsController ();

  void                SetView (PlacesResultsView *view);
  PlacesResultsView * GetView ();

  void AddGroup     (PlaceEntry *entry, PlaceEntryGroup& group);
  void AddResult    (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result);
  void RemoveResult (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result);

  // Clears all the current groups and results
  void Clear ();

  bool ActivateFirst ();

protected:
  const gchar* GetName ();
  void         AddProperties (GVariantBuilder *builder);
  static gboolean MakeThingsLookNice (PlacesResultsController *self);

private:
  PlacesResultsView *_results_view;
  std::map<const void *, PlacesGroupController *> _id_to_group;
  std::vector<PlacesGroupController *> _groups;
  guint32 _make_things_look_nice_id;
};

#endif // PLACES_RESULTS_CONTROLLER_H
