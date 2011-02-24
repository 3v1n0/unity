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

  SetLayout (_layout);

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
  if ((!entry && _entry == _home_entry) && g_strcmp0 (search_string, "") == 0)
    return;

  if (signal)
    entry_changed.emit (entry);

  if (entry == NULL)
    entry = _home_entry;

  if (_entry)
  {
    _entry->SetActive (false);

    g_signal_handler_disconnect (_entry->GetGroupsModel (), _group_added_id);
    g_signal_handler_disconnect (_entry->GetGroupsModel (), _group_removed_id);
    g_signal_handler_disconnect (_entry->GetResultsModel (), _result_added_id);
    g_signal_handler_disconnect (_entry->GetResultsModel (), _result_removed_id);

    _group_added_id = _group_removed_id = _result_added_id = _result_removed_id = 0;

    _results_controller->Clear ();
  }
  
  _entry = entry;

  std::map <gchar*, gchar*> hints;
  DeeModel     *groups, *results;
  DeeModelIter *iter, *last;

  _entry->SetActive (true);
  _search_bar->SetActiveEntry (_entry, section_id, search_string, (_entry == _home_entry));

  groups = _entry->GetGroupsModel ();
  iter = dee_model_get_first_iter (groups);
  last = dee_model_get_last_iter (groups);
  while (iter != last)
  {
    _results_controller->CreateGroup (dee_model_get_string (groups,
                                                            iter,
                                                            PlaceEntry::GROUP_NAME));
    iter = dee_model_next (groups, iter);
  }

  if (_entry != _home_entry)
  {
    results = _entry->GetResultsModel ();
    iter = dee_model_get_first_iter (results);
    last = dee_model_get_last_iter (results);
    while (iter != last)
    {
      OnResultAdded (results, iter, this);
      iter = dee_model_next (results, iter);
    }
  }

  _group_added_id = g_signal_connect (_entry->GetGroupsModel (), "row-added",
                                      (GCallback)&PlacesView::OnGroupAdded, this);
  _group_removed_id = g_signal_connect (_entry->GetGroupsModel (), "row-removed",
                                        (GCallback)&PlacesView::OnGroupRemoved, this);
  _result_added_id = g_signal_connect (_entry->GetResultsModel (), "row-added",
                                       (GCallback)&PlacesView::OnResultAdded, this);
  _result_removed_id = g_signal_connect (_entry->GetResultsModel (), "row-removed",
                                         (GCallback)&PlacesView::OnResultRemoved, this);

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
PlacesView::OnGroupAdded (DeeModel *model, DeeModelIter *iter, PlacesView *self)
{
  g_debug ("GroupAdded: %s", dee_model_get_string (model, iter, 1));
}


void
PlacesView::OnGroupRemoved (DeeModel *model, DeeModelIter *iter, PlacesView *self)
{
  g_debug ("GroupRemoved: %s", dee_model_get_string (model, iter, 1));
}

void
PlacesView::OnResultAdded (DeeModel *model, DeeModelIter *iter, PlacesView *self)
{
  PlaceEntry       *active;
  DeeModel         *groups;
  DeeModelIter     *git;
  const gchar      *group_id;
  gchar            *result_name;
  const gchar      *result_icon;
  PlacesSimpleTile *tile;

  //FIXME: We can't do anything with these do just ignore
  if (g_str_has_prefix (dee_model_get_string (model, iter, PlaceEntry::RESULT_URI),
                        "unity-install"))
    return;
  
  active = self->GetActiveEntry ();
  groups = active->GetGroupsModel ();
  git = dee_model_get_iter_at_row (groups, dee_model_get_uint32 (model,
                                                                 iter,
                                                                 PlaceEntry::RESULT_GROUP_ID));
  group_id = dee_model_get_string (groups, git, PlaceEntry::GROUP_NAME);
  result_name = g_markup_escape_text (dee_model_get_string (model, iter, PlaceEntry::RESULT_NAME),
                                      -1);
  result_icon = dee_model_get_string (model, iter, PlaceEntry::RESULT_ICON);

  tile = new PlacesSimpleTile (result_icon, result_name, 48);
  tile->SetURI (dee_model_get_string (model, iter, PlaceEntry::RESULT_URI));
  tile->sigClick.connect (sigc::mem_fun (self, &PlacesView::OnResultClicked));
  self->GetResultsController ()->AddResultToGroup (group_id, tile, iter);

  g_free (result_name);
}

void
PlacesView::OnResultRemoved (DeeModel *model, DeeModelIter *iter, PlacesView *self)
{
  //FIXME: We can't do anything with these do just ignore
  if (g_str_has_prefix (dee_model_get_string (model, iter, PlaceEntry::RESULT_URI),
                        "unity-install"))
    return;

  self->GetResultsController ()->RemoveResult (iter);
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

nux::TextEntry*
PlacesView::GetTextEntryView ()
{
  return _search_bar->_pango_entry;
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

