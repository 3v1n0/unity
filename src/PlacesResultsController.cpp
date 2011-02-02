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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#include "config.h"

#include "Nux/Nux.h"
#include "Nux/GridHLayout.h"

#include "NuxGraphics/GLThread.h"
#include <glib.h>

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesResultsController.h"
#include "PlacesGroup.h"
#include "PlacesSimpleTile.h"


PlacesResultsController::PlacesResultsController ()
{
  _results_view = NULL;
}

PlacesResultsController::~PlacesResultsController ()
{
  _results_view->UnReference ();
}

void
PlacesResultsController::SetView (PlacesResultsView *view)
{
  if (_results_view != NULL)
    _results_view->UnReference ();

  view->Reference ();
  _results_view = view;
}

PlacesResultsView *
PlacesResultsController::GetView ()
{
  return _results_view;
}

void
PlacesResultsController::AddResultToGroup (const char *groupname,
                                           PlacesTile *tile,
                                           const char *_id)
{
  PlacesGroup *group = _groups[groupname];

  if (!group)
    {
      group = CreateGroup (groupname);
    }

  group->GetLayout ()->AddView (tile, 1, nux::eLeft, nux::eFull);
  _tiles[_id] = tile;
  _tile_group_relations[_id] = groupname;

  // Should also catch the onclick signal here on each tile,
  // so we can activate or do whatever it is we need to do

  if (group->IsVisible () == false)
  {
    group->SetVisible (true);
    _results_view->ReJiggyGroups ();
  }
}

void
PlacesResultsController::RemoveResultFromGroup (const char *groupname,
                                                const char *_id)
{
  PlacesTile *tile = _tiles[_id];
  PlacesGroup *group = _groups[groupname];

  if (group)
  {
    if (tile)
    {
      group->GetLayout ()->RemoveChildObject (tile);

      if (group->GetLayout ()->GetChildren ().empty ())
      {
        group->SetVisible (false);
        _results_view->ReJiggyGroups ();
      }
    }
    else
    {
      g_warning ("Unable to remove '%s' from group '%s': Unable to find tile",
                 _id, groupname);
    }
  }
  else
  {
    g_warning ("Unable to remove '%s' from group '%s': Unable to find group",
               _id, groupname);
  }

  _tiles.erase (_id);
  _tile_group_relations.erase (_id);
}

void
PlacesResultsController::RemoveResult (const char *_id)
{
  RemoveResultFromGroup (_tile_group_relations [_id].c_str (), _id);
}

void
PlacesResultsController::Clear ()
{
  std::map<std::string, PlacesGroup *>::iterator it;

  for (it = _groups.begin (); it != _groups.end (); ++it)
  {
    PlacesGroup *group = static_cast <PlacesGroup *> (it->second);

    _results_view->RemoveGroup (group);
    group->UnReference ();
  }

  _groups.erase (_groups.begin (), _groups.end ());
  _tiles.erase (_tiles.begin (), _tiles.end ());
  _tile_group_relations.erase (_tile_group_relations.begin (), _tile_group_relations.end ());
}

PlacesGroup *
PlacesResultsController::CreateGroup (const char *groupname)
{
  g_debug ("CreateGroup: %s", groupname);

  PlacesGroup *newgroup = new PlacesGroup (NUX_TRACKER_LOCATION);
  newgroup->SinkReference ();
  newgroup->SetTitle (groupname);
  newgroup->SetRowHeight (92);
  newgroup->SetItemDetail (1, 100);
  newgroup->SetExpanded (true);

  nux::GridHLayout *layout = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  layout->ForceChildrenSize (true);
  layout->SetChildrenSize (140, 90);
  layout->EnablePartialVisibility (false);

  layout->SetVerticalExternalMargin (4);
  layout->SetHorizontalExternalMargin (4);
  layout->SetVerticalInternalMargin (4);
  layout->SetHorizontalInternalMargin (4);

  newgroup->AddLayout (layout);
  newgroup->SetVisible (false);

  _groups[groupname] = newgroup;
  _results_view->AddGroup (newgroup);
  _results_view->ReJiggyGroups ();

  return newgroup;
}


/* Introspection */
const gchar *
PlacesResultsController::GetName ()
{
  return "PlacesResultsController";
}

void
PlacesResultsController::AddProperties (GVariantBuilder *builder)
{
}








