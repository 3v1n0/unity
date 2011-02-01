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

#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include "Nux/Layout.h"
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "PlacesResultsView.h"
#include "PlacesTile.h"
#include "Introspectable.h"

class PlacesResultsController : public nux::Object, public Introspectable
{
public:
  PlacesResultsController ();
  ~PlacesResultsController ();

  void SetView (PlacesResultsView *_results_view);
  PlacesResultsView *GetView ();


  void AddResultToGroup      (const char *groupname,
                              PlacesTile *tile,
                              const char *id);
  void RemoveResult          (const char *id);
  void RemoveResultFromGroup (const char *groupname,
                              const char *_id);
  void Clear                 ();
  PlacesGroup  *CreateGroup  (const char *groupname);
protected:
  const gchar* GetName ();
  void AddProperties (GVariantBuilder *builder);

private:
  PlacesResultsView *_results_view;
  std::map<std::string, PlacesGroup *>   _groups;
  std::map<std::string, PlacesTile *>    _tiles;
  std::map<std::string, std::string>     _tile_group_relations;

};

#endif // PLACES_RESULTS_CONTROLLER_H
