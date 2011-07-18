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
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "config.h"

#include <glib.h>

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesSettings.h"

#include "PlacesResultsController.h"

PlacesResultsController::PlacesResultsController()
  : _make_things_look_nice_id(0)
{
  _results_view = NULL;
}

PlacesResultsController::~PlacesResultsController()
{
  Clear();
  if (_make_things_look_nice_id)
    g_source_remove (_make_things_look_nice_id);
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
PlacesResultsController::AddGroup (PlaceEntry *entry, PlaceEntryGroup& group)
{
  PlacesGroupController *controller = new PlacesGroupController (entry, group);

  _id_to_group[group.GetId ()] = controller;
  _groups.push_back (controller);
  _results_view->AddGroup (controller->GetGroup ());
  _results_view->QueueRelayout ();
}

void
PlacesResultsController::AddResult (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlacesGroupController *controller = _id_to_group[group.GetId ()];

  // We don't complain here because there are some shortcuts that the PlacesView takes which
  // mean we sometimes receive requests that we can't process
  if (!controller)
    return;

  controller->AddResult (group, result);
  
  if (!_make_things_look_nice_id)
    _make_things_look_nice_id = g_idle_add ((GSourceFunc)PlacesResultsController::MakeThingsLookNice, this);
}

void
PlacesResultsController::RemoveResult (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlacesGroupController *controller = _id_to_group[group.GetId ()];

  // We don't complain here because there are some shortcuts that the PlacesView takes which
  // mean we sometimes receive requests that we can't process
  if (!controller)
    return;

  controller->RemoveResult (group, result);

  if (!_make_things_look_nice_id)
    _make_things_look_nice_id = g_idle_add ((GSourceFunc)PlacesResultsController::MakeThingsLookNice, this);
}

void PlacesResultsController::Clear()
{
  std::map <const void *, PlacesGroupController *>::iterator it, eit = _id_to_group.end ();

  for (it = _id_to_group.begin (); it != eit; ++it)
  {
    if (it->second)
      (it->second)->UnReference ();
  }

  _id_to_group.clear();
  _groups.clear();

  if (_results_view)
    _results_view->Clear();
}

bool PlacesResultsController::ActivateFirst()
{
  std::vector<PlacesGroupController *>::iterator it, eit = _groups.end ();

  for (it = _groups.begin (); it != eit; ++it)
    if ((*it)->ActivateFirst ())
      return true;

  return false;
}

//
// Introspection
//
const gchar* PlacesResultsController::GetName()
{
  return "PlacesResultsController";
}

void PlacesResultsController::AddProperties(GVariantBuilder *builder)
{
}

gboolean PlacesResultsController::MakeThingsLookNice(PlacesResultsController *self)
{
  PlacesGroup *last_active_group = NULL;
  std::vector<PlacesGroupController *>::iterator it, eit = self->_groups.end ();
  
  for (it = self->_groups.begin (); it != eit; ++it)
  {
    PlacesGroupController *controller = *it;

    if (controller && controller->GetTotalResults ())
    {
      last_active_group = controller->GetGroup ();
      last_active_group->SetDrawSeparator (true);
    }
  }
  if (last_active_group)
    last_active_group->SetDrawSeparator (false);

  self->_make_things_look_nice_id = 0;

  return FALSE;
}
