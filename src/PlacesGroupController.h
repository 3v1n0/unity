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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACES_GROUP_CONTROLLER_H
#define PLACES_GROUP_CONTROLLER_H

#include <Nux/Nux.h>

#include "Introspectable.h"
#include "PlaceEntry.h"

#include "PlacesGroup.h"
#include "PlacesTile.h"

class PlacesGroupController : public nux::Object, public Introspectable
{
public:
  PlacesGroupController (PlaceEntry *entry, PlaceEntryGroup& group);
  ~PlacesGroupController ();

  const void  * GetId ();
  PlacesGroup * GetGroup ();

  void AddResult    (PlaceEntryGroup& group, PlaceEntryResult& result);
  void RemoveResult (PlaceEntryGroup& group, PlaceEntryResult& result);

  void Clear ();

protected:
  const gchar* GetName ();
  void         AddProperties (GVariantBuilder *builder);

private:
  void AddTile (PlaceEntry *ignore, PlaceEntryGroup& group, PlaceEntryResult& result);
  void CheckTiles ();
  static gboolean CheckTilesTimeout (PlacesGroupController *self);

private:
  PlaceEntry  *_entry;
  PlacesGroup *_group;
  const void  *_id;
  std::map<const void *, PlacesTile *>  _id_to_tile;
  guint _check_tiles_id;
  std::vector<const void *> _queue;
};

#endif // PLACES_GROUP_CONTROLLER_H
