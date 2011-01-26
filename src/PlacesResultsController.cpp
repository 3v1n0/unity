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
  std::string *group_name = new std::string (groupname);
  std::string *id = new std::string (_id);

  PlacesGroup *group = NULL;

  if (_groups.find (*group_name) == _groups.end ())
    {
      group = CreateGroup (groupname);
    }
  else
    {
      group = _groups[*group_name];
    }

  group->GetLayout ()->AddView (tile, 1, nux::eLeft, nux::eFull);
  _tiles[*id] = tile;
  _tile_group_relations[*id] = *(new std::string (groupname));

  // Should also catch the onclick signal here on each tile,
  // so we can activate or do whatever it is we need to do

  delete group_name;
  delete id;
}

void
PlacesResultsController::RemoveResultFromGroup (const char *groupname,
                                                const char *_id)
{
  std::string *group_name = new std::string (groupname);
  std::string *id = new std::string (_id);

  PlacesTile *tile = _tiles[*id];
  PlacesGroup *group = _groups[*group_name];

  group->GetLayout ()->RemoveChildObject (tile);

  if (group->GetLayout ()->GetChildren ().empty ())
  {
    _results_view->RemoveGroup (group);
    _groups.erase (*group_name);
    group->UnReference ();
  }

  _tiles.erase (*id);

  delete &(_tile_group_relations[*id]);
  _tile_group_relations.erase (*id);

  delete group_name;
  delete id;
}

void
PlacesResultsController::RemoveResult (const char *_id)
{
  std::string *id = new std::string (_id);
  RemoveResultFromGroup (_tile_group_relations [*id].c_str (), _id);
  delete id;
}



PlacesGroup *
PlacesResultsController::CreateGroup (const char *groupname)
{
  g_debug ("making a group for %s", groupname);
  std::string *group_name = new std::string (groupname);

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

  newgroup->SetLayout (layout);

  _groups[*group_name] = newgroup;
  _results_view->AddGroup (newgroup);

  delete group_name;

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








