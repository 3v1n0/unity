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
#include "NuxGraphics/GLThread.h"
#include "UBusMessages.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlaceFactory.h"

#include "PlacesView.h"

static void place_entry_activate_request (GVariant *payload, PlacesView *self);

NUX_IMPLEMENT_OBJECT_TYPE (PlacesView);

PlacesView::PlacesView (NUX_FILE_LINE_DECL)
: nux::View (NUX_TRACKER_LOCATION),
  _entry (NULL)
{
  _layout = new nux::VLayout (NUX_TRACKER_LOCATION);

  _search_bar = new PlacesSearchBar ();
  _layout->AddView (_search_bar, 0, nux::eCenter, nux::eFull);
  AddChild (_search_bar);
  
  _home_view = new PlacesHomeView ();
  //_layout->AddView (_home_view, 1, nux::eCenter, nux::eFull);
  AddChild (_home_view);

  _results_controller = new PlacesResultsController ();
  _results_view = new PlacesResultsView ();
  _results_controller->SetView (_results_view);
  _layout->AddView (_results_view, 1, nux::eCenter, nux::eFull);

  SetCompositionLayout (_layout);

  // Register for all the events
  UBusServer *ubus = ubus_server_get_default ();
  ubus_server_register_interest (ubus, UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                 (UBusCallback)place_entry_activate_request,
                                 this);
  ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_CLOSE_REQUEST,
                                 (UBusCallback)&PlacesView::CloseRequest,
                                 this);
}

PlacesView::~PlacesView ()
{

}

long
PlacesView::ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
    
  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);
  return ret;
}

void
PlacesView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{

}


void
PlacesView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  if (_layout)
    _layout->ProcessDraw (GfxContext, force_draw);
}

//
// PlacesView Methods
//
void
PlacesView::SetActiveEntry (PlaceEntry *entry, guint section_id, const char *search_string)
{
  if (_entry)
  {
    _entry->SetActive (false);
    g_signal_handler_disconnect (_entry->GetGroupsModel (), _group_added_id);
    g_signal_handler_disconnect (_entry->GetGroupsModel (), _group_removed_id);
    g_signal_handler_disconnect (_entry->GetResultsModel (), _result_added_id);
    g_signal_handler_disconnect (_entry->GetResultsModel (), _result_removed_id);

    _group_added_id = _group_removed_id = _result_added_id = _result_removed_id = 0;
  }

  _entry = entry;
  
  if (_entry)
  {
    std::map <gchar*, gchar*> hints;

    _entry->SetActive (true);

    _group_added_id = g_signal_connect (_entry->GetGroupsModel (), "row-added",
                                        (GCallback)&PlacesView::OnGroupAdded, this);
    _group_removed_id = g_signal_connect (_entry->GetGroupsModel (), "row-removed",
                                          (GCallback)&PlacesView::OnGroupRemoved, this);
    _result_added_id = g_signal_connect (_entry->GetResultsModel (), "row-added",
                                         (GCallback)&PlacesView::OnResultAdded, this);
    _result_removed_id = g_signal_connect (_entry->GetResultsModel (), "row-removed",
                                           (GCallback)&PlacesView::OnResultRemoved, this);
  }
  _search_bar->SetActiveEntry (_entry, section_id, search_string);
}

PlaceEntry *
PlacesView::GetActiveEntry ()
{
  return _entry;
}

PlacesResultsController *
PlacesView::GetResultsController ()
{
  return _results_controller;
}


//
// Model handlers
//
void
PlacesView::OnGroupAdded (DeeModel *model, DeeModelIter *iter, PlacesView *self)
{
  g_debug ("GroupAdded: %s", dee_model_get_string (model, iter, 1));
}


void
PlacesView::OnGroupRemoved (DeeModel *model, DeeModelIter *iter, PlacesView *self)
{
  g_debug ("GroupAdded: %s", dee_model_get_string (model, iter, 1));
}

void
PlacesView::OnResultAdded (DeeModel *model, DeeModelIter *iter, PlacesView *self)
{
//  g_debug ("ResultAdded: %s", dee_model_get_string (model, iter, 4));
  PlacesSimpleTile *tile = new PlacesSimpleTile (dee_model_get_string (model, iter, 1),
                                                 dee_model_get_string (model, iter, 4));

  PlaceEntry *active = self->GetActiveEntry ();
  DeeModelIter *it = dee_model_get_iter_at_row (active->GetGroupsModel (),
                                                dee_model_get_uint32 (model, iter, 2));
  self->GetResultsController ()->AddResultToGroup (dee_model_get_string (active->GetGroupsModel (),
                                                                         it,
                                                                         1),
                                                   tile,
                                                   dee_model_get_string (model, iter, 0));
}

void
PlacesView::OnResultRemoved (DeeModel *model, DeeModelIter *iter, PlacesView *self)
{
 // g_debug ("ResultRemoved: %s", dee_model_get_string (model, iter, 4));

  self->GetResultsController ()->RemoveResult (dee_model_get_string (model, iter, 0));
}

//
// UBus handlers
//
void
PlacesView::PlaceEntryActivateRequest (const char *entry_id,
                                       guint       section_id,
                                       const char *search_string)
{
  std::vector<Place *> places = PlaceFactory::GetDefault ()->GetPlaces ();
  std::vector<Place *>::iterator it;
  
  for (it = places.begin (); it != places.end (); ++it)
  {
    Place *place = static_cast<Place *> (*it);
    std::vector<PlaceEntry *> entries = place->GetEntries ();
    std::vector<PlaceEntry *>::iterator i;

    for (i = entries.begin (); i != entries.end (); ++i)
    {
      PlaceEntry *entry = static_cast<PlaceEntry *> (*i);

      if (g_strcmp0 (entry_id, entry->GetId ()) == 0)
      {
        SetActiveEntry (entry, section_id, search_string);
        return;
      }
    }
  }

  g_warning ("%s: Unable to find entry: %s for request: %d %s",
             G_STRFUNC,
             entry_id,
             section_id,
             search_string);
}

void
PlacesView::CloseRequest (GVariant *data, PlacesView *self)
{
  self->SetActiveEntry (NULL, 0, "");
}

//
// Introspection
//
const gchar *
PlacesView::GetName ()
{
  return "PlacesView";
}

void
PlacesView::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height)); 
}

//
// C glue code
//
static void
place_entry_activate_request (GVariant *payload, PlacesView *self)
{
  gchar *id = NULL;
  guint  section = 0;
  gchar *search_string = NULL;

  g_return_if_fail (self);

  g_variant_get (payload, "(sus)", &id, &section, &search_string);

  self->PlaceEntryActivateRequest (id, section, search_string);

  g_free (id);
  g_free (search_string);
}
