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
  if (_results_view)
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
  char *group_name = g_strdup (groupname);
  char *id = g_strdup (_id);
  if (_groups.find (group_name) == _groups.end ())
  {
    CreateGroup (group_name);
  }

  _groups[group_name]->GetLayout ()->AddView (tile);
  _tiles[id] = tile;
  _tile_group_relations[id] = g_strdup (group_name);

  // Should also catch the onclick signal here on each tile,
  // so we can activate or do whatever it is we need to do

  g_free (group_name);
  g_free (id);
}

void
PlacesResultsController::RemoveResultFromGroup (const char *groupname,
                                                const char *_id)
{
  char *group_name = g_strdup (groupname);
  char *id = g_strdup (_id);

  PlacesTile *tile = _tiles[id];
  PlacesGroup *group = _groups[group_name];

  group->GetLayout ()->RemoveChildObject (tile);

  if (group->GetLayout ()->GetChildren ().empty ())
  {
    _results_view->RemoveGroup (group);
    _groups.erase (group_name);
    group->UnReference ();
  }

  _tiles.erase (id);

  g_free (_tile_group_relations[id]);
  _tile_group_relations.erase (id);

  g_free (group_name);
  g_free (id);
}

void
PlacesResultsController::RemoveResult (const char *_id)
{
  char *id = g_strdup (_id);
  RemoveResultFromGroup (_tile_group_relations [id], id);
  g_free (id);
}



void
PlacesResultsController::CreateGroup (const char *groupname)
{
  char *group_name = g_strdup (groupname);
  PlacesGroup *newgroup = new PlacesGroup (NUX_TRACKER_LOCATION);
  newgroup->SinkReference ();
  newgroup->SetTitle (group_name);
  newgroup->SetRowHeight (92);
  newgroup->SetItemDetail (1, 100);
  newgroup->SetExpanded (true);

  nux::GridHLayout *layout = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  newgroup->SetLayout (layout);

  _groups[group_name] = newgroup;
  _results_view->AddGroup (newgroup);

  g_free (group_name);
}










