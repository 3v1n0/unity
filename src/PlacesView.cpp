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

#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlaceFactory.h"

#include "PlacesView.h"

static void place_entry_activate_request (GVariant *payload, PlacesView *self);

NUX_IMPLEMENT_OBJECT_TYPE (PlacesView);

PlacesView::PlacesView (PlaceFactory *factory)
: nux::View (NUX_TRACKER_LOCATION),
  _factory (factory),
  _entry (NULL)
{
  _home_entry = new PlaceEntryHome (_factory);

  _layout = new nux::VLayout (NUX_TRACKER_LOCATION);
  
  _search_bar = new PlacesSearchBar ();
  _layout->AddView (_search_bar, 0, nux::eCenter, nux::eFull);
  AddChild (_search_bar);
  _search_bar->search_changed.connect (sigc::mem_fun (this, &PlacesView::OnSearchChanged));

  _layered_layout = new nux::LayeredLayout (NUX_TRACKER_LOCATION);
  _layout->AddLayout (_layered_layout, 1, nux::eCenter, nux::eFull);
  
  _home_view = new PlacesHomeView ();
  _layered_layout->AddLayer (_home_view);
  AddChild (_home_view);

  _results_controller = new PlacesResultsController ();
  _results_view = new PlacesResultsView ();
  _results_controller->SetView (_results_view);
  _layered_layout->AddLayer (_results_view);

  _layered_layout->SetActiveLayer (_home_view);

  SetCompositionLayout (_layout);

  // Register for all the events
  UBusServer *ubus = ubus_server_get_default ();
  ubus_server_register_interest (ubus, UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                 (UBusCallback)place_entry_activate_request,
                                 this);
  ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_CLOSE_REQUEST,
                                 (UBusCallback)&PlacesView::CloseRequest,
                                 this);

  _icon_loader = IconLoader::GetDefault ();

  SetActiveEntry (_home_entry, 0, "");
}

PlacesView::~PlacesView ()
{
  delete _home_entry;
}

long
PlacesView::ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  // FIXME: This breaks with multi-monitor
  nux::Geometry homebutton (0.0f, 0.0f, 66.0f, 24.0f);
  long ret = TraverseInfo;

  if (ievent.e_event == nux::NUX_KEYDOWN
      && ievent.GetKeySym () == NUX_VK_ESCAPE)
  {
    SetActiveEntry (NULL, 0, "");
    return TraverseInfo;
  }

  if (ievent.e_event == nux::NUX_MOUSE_RELEASED)
  {
    if (homebutton.IsPointInside (ievent.e_x, ievent.e_y))
      SetActiveEntry (NULL, 0, "");
    return TraverseInfo;
  }

  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);
  return ret;
}

void
PlacesView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  gPainter.PaintBackground (GfxContext, GetGeometry ());

  GfxContext.PopClippingRectangle ();
}


void
PlacesView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );
  GfxContext.GetRenderStates ().SetBlend (true);
  GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);
  
  if (_layout)
    _layout->ProcessDraw (GfxContext, force_draw);
  
  GfxContext.GetRenderStates ().SetBlend (false);

  GfxContext.PopClippingRectangle ();
}

//
// PlacesView Methods
//
void
PlacesView::SetActiveEntry (PlaceEntry *entry, guint section_id, const char *search_string, bool signal)
{
  if (signal)
    entry_changed.emit (entry);

  if (entry == NULL)
    entry = _home_entry;

  if (_entry)
  {
    _entry->SetActive (false);

    _group_added_conn.disconnect ();
    _result_added_conn.disconnect ();
    _result_removed_conn.disconnect ();

    _results_controller->Clear ();
  }
  
  _entry = entry;

  _entry->SetActive (true);
  _search_bar->SetActiveEntry (_entry, section_id, search_string, (_entry == _home_entry));

  _entry->ForeachGroup (sigc::mem_fun (this, &PlacesView::OnGroupAdded));
  
  if (_entry != _home_entry)
    _entry->ForeachResult (sigc::mem_fun (this, &PlacesView::OnResultAdded));

  _group_added_conn = _entry->group_added.connect (sigc::mem_fun (this, &PlacesView::OnGroupAdded));
  _result_added_conn = _entry->result_added.connect (sigc::mem_fun (this, &PlacesView::OnResultAdded));
  _result_removed_conn = _entry->result_removed.connect (sigc::mem_fun (this, &PlacesView::OnResultRemoved));

  if (_entry == _home_entry && (g_strcmp0 (search_string, "") == 0))
    _layered_layout->SetActiveLayer (_home_view);
  else
    _layered_layout->SetActiveLayer (_results_view);
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
PlacesView::OnGroupAdded (PlaceEntryGroup& group)
{
  _results_controller->CreateGroup (group.GetName (), group.GetIcon ());
}

void
PlacesView::OnResultAdded (PlaceEntryGroup& group, PlaceEntryResult& result)
{
  gchar            *result_name;
  const gchar      *result_icon;
  PlacesSimpleTile *tile;

  //FIXME: We can't do anything with these do just ignore
  if (g_str_has_prefix (result.GetURI (), "unity-install"))
    return;
  
  result_name = g_markup_escape_text (result.GetName (), -1);
  result_icon = result.GetIcon ();

  tile = new PlacesSimpleTile (result_icon, result_name, 48);
  tile->SetURI (result.GetURI ());
  tile->sigClick.connect (sigc::mem_fun (this, &PlacesView::OnResultClicked));
  _results_controller->AddResultToGroup (group.GetName (), tile,
                                         const_cast<void*> (result.GetId ()));

  g_free (result_name);
}

void
PlacesView::OnResultRemoved (PlaceEntryGroup& group, PlaceEntryResult& result)
{
  //FIXME: We can't do anything with these do just ignore
  if (g_str_has_prefix (result.GetURI (), "unity-install"))
    return;

  _results_controller->RemoveResult (const_cast<void*> (result.GetId ()));
}

void
PlacesView::OnResultClicked (PlacesTile *tile)
{
  PlacesSimpleTile *simple_tile = static_cast<PlacesSimpleTile *> (tile);
  const char *uri;

  if (!(uri = simple_tile->GetURI ()))
  {
    g_warning ("Unable to launch %s: does not have a URI", simple_tile->GetLabel ());
    return;
  }

  if (g_str_has_prefix (uri, "application://"))
  {
    const char      *id = &uri[14];
    GDesktopAppInfo *info;

    info = g_desktop_app_info_new (id);
    if (G_IS_DESKTOP_APP_INFO (info))
    {
      GError *error = NULL;

      g_app_info_launch (G_APP_INFO (info), NULL, NULL, &error);
      if (error)
      {
        g_warning ("Unable to launch %s: %s", id,  error->message);
        g_error_free (error);
      }
      g_object_unref (info);
   }
  }
  else
  {
    GError *error = NULL;
    gtk_show_uri (NULL, uri, time (NULL), &error);

    if (error)
    {
      g_warning ("Unable to show %s: %s", uri, error->message);
      g_error_free (error);
    }
  }

  ubus_server_send_message (ubus_server_get_default (),
                            UBUS_PLACE_VIEW_CLOSE_REQUEST,
                            NULL);
}

void
PlacesView::OnSearchChanged (const char *search_string)
{
  if (_entry == _home_entry)
  {
    if (g_strcmp0 (search_string, "") == 0)
    {
      _layered_layout->SetActiveLayer (_home_view);
      _home_view->QueueDraw ();
    }
    else
    {
      _layered_layout->SetActiveLayer (_results_view);
      _results_view->QueueDraw ();
    }

    _results_view->QueueDraw ();
    _home_view->QueueDraw ();
    _layered_layout->QueueDraw ();
    QueueDraw ();
  }
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

  if (g_strcmp0 (entry_id, "PlaceEntryHome") == 0)
  {
    SetActiveEntry (_home_entry, section_id, search_string);
    return;
  }
  
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
