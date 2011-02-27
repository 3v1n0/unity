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
 */

#ifndef PLACES_RESULTS_CONTROLLER_H
#define PLACES_RESULTS_CONTROLLER_H

#include <Nux/Nux.h>

#include "Introspectable.h"
#include "PlaceEntry.h"
#include "PlacesResultsView.h"

//FIXME
#include "PlacesTile.h"
#include "PlacesGroup.h"

class PlacesResultsController : public nux::Object, public Introspectable
{
public:
  PlacesResultsController  ();
  ~PlacesResultsController ();

  void                SetView (PlacesResultsView *view);
  PlacesResultsView * GetView ();

  void AddGroup     (PlaceEntryGroup& group);
  void AddResult    (PlaceEntryGroup& group, PlaceEntryResult& result);
  void RemoveResult (PlaceEntryGroup& group, PlaceEntryResult& result);

  // Clears all the current groups and results
  void Clear ();

protected:
  const gchar* GetName ();
  void         AddProperties (GVariantBuilder *builder);

private:
  PlacesResultsView *_results_view;
  std::map<const void *, PlacesGroup *> _id_to_group;
  std::map<const void *, PlacesTile *> _id_to_tile;
};

#endif // PLACES_RESULTS_CONTROLLER_H
