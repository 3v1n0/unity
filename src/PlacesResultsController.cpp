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

#include "PlacesSettings.h"

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
PlacesResultsController::AddGroup (PlaceEntryGroup& group)
{
  PlacesSettings *settings = PlacesSettings::GetDefault ();

  PlacesGroup *new_group = new PlacesGroup (NUX_TRACKER_LOCATION);
  new_group->SetTitle (group.GetName ());
  new_group->SetEmblem (group.GetIcon ());

  nux::GridHLayout *layout = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  layout->ForceChildrenSize (true);
  layout->SetChildrenSize (settings->GetDefaultTileWidth (), 100);
  layout->EnablePartialVisibility (false);

  layout->SetVerticalExternalMargin (4);
  layout->SetHorizontalExternalMargin (4);
  layout->SetVerticalInternalMargin (4);
  layout->SetHorizontalInternalMargin (4);
  layout->SetHeightMatchContent (true);

  new_group->SetChildLayout (layout);
  new_group->SetVisible (false);

  _id_to_group[group.GetId ()] = new_group;
  _results_view->AddGroup (new_group);
  _results_view->QueueRelayout ();
}

void
PlacesResultsController::AddResult (PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlacesGroup      *pgroup;
  gchar            *result_name;
  const gchar      *result_icon;
  PlacesSimpleTile *tile;

  pgroup = _id_to_group[group.GetId ()];
  if (!pgroup)
  {
    g_warning ("Unable find group %s for result %s", group.GetName (), result.GetName ());
    return;
  }

  result_name = g_markup_escape_text (result.GetName (), -1);
  result_icon = result.GetIcon ();

  tile = new PlacesSimpleTile (result_icon, result_name, 48);
  tile->SetURI (result.GetURI ());

  _id_to_tile[result.GetId ()] = tile;
  
  pgroup->GetChildLayout ()->AddView (tile);
  pgroup->Relayout ();
  tile->QueueRelayout ();

  pgroup->SetVisible (pgroup->GetChildLayout ()->GetChildren ().size ());
  g_free (result_name);
}

void
PlacesResultsController::RemoveResult (PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlacesTile  *tile;
  PlacesGroup *pgroup;

  pgroup = _id_to_group[group.GetId ()];
  if (!pgroup)
  {
    g_warning ("Unable find group %s for result %s", group.GetName (), result.GetName ());
    return;
  }
  
  tile = _id_to_tile[result.GetId ()];
  if (!tile)
  {
    g_warning ("Unable to find result %s for group %s", result.GetName (), group.GetName ());
    return;
  }

  pgroup->GetChildLayout ()->RemoveChildObject (tile);
  pgroup->SetVisible (pgroup->GetChildLayout ()->GetChildren ().size ());
  pgroup->Relayout ();
}

void
PlacesResultsController::Clear ()
{
  _results_view->Clear ();
  _id_to_group.erase (_id_to_group.begin (), _id_to_group.end ());
  _id_to_tile.erase (_id_to_tile.begin (), _id_to_tile.end ());
}


//
// Introspection
//
const gchar*
PlacesResultsController::GetName ()
{
  return "PlacesResultsController";
}

void
PlacesResultsController::AddProperties (GVariantBuilder *builder)
{

}
