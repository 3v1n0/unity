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
#include "PlacesStyle.h"
#include "PlacesSettings.h"
#include "PlacesView.h"

static void place_entry_activate_request (GVariant *payload, PlacesView *self);

NUX_IMPLEMENT_OBJECT_TYPE (PlacesView);

PlacesView::PlacesView (PlaceFactory *factory)
: nux::View (NUX_TRACKER_LOCATION),
  _factory (factory),
  _entry (NULL),
  _size_mode (SIZE_MODE_FULLSCREEN),
  _shrink_mode (SHRINK_MODE_NONE),
  _target_height (1),
  _actual_height (1),
  _resize_id (0)
{
  LoadPlaces ();
  _factory->place_added.connect (sigc::mem_fun (this, &PlacesView::OnPlaceAdded));

  _home_entry = new PlaceEntryHome (_factory);

  _layout = new nux::HLayout (NUX_TRACKER_LOCATION);

  nux::VLayout *vlayout = new nux::VLayout (NUX_TRACKER_LOCATION);
  _layout->AddLayout (vlayout, 1, nux::eCenter, nux::eFull);

  _h_spacer= new nux::SpaceLayout (1, 1, 1, nux::AREA_MAX_HEIGHT);
  _layout->AddLayout (_h_spacer, 0, nux::eCenter, nux::eFull);

  _search_bar = new PlacesSearchBar ();
  vlayout->AddView (_search_bar, 0, nux::eCenter, nux::eFull);
  AddChild (_search_bar);

  _search_bar->search_changed.connect (sigc::mem_fun (this, &PlacesView::OnSearchChanged));
  _search_bar->activated.connect (sigc::mem_fun (this, &PlacesView::OnEntryActivated));

  _layered_layout = new nux::LayeredLayout (NUX_TRACKER_LOCATION);
  vlayout->AddLayout (_layered_layout, 1, nux::eCenter, nux::eFull);

  _v_spacer = new nux::SpaceLayout (1, nux::AREA_MAX_WIDTH, 1, 1);
  vlayout->AddLayout (_v_spacer, 0, nux::eCenter, nux::eFull);

  _home_view = new PlacesHomeView ();
  _home_view->expanded.connect (sigc::mem_fun (this, &PlacesView::ReEvaluateShrinkMode));
  _layered_layout->AddLayer (_home_view);
  AddChild (_home_view);

  _results_controller = new PlacesResultsController ();
  _results_view = new PlacesResultsView ();
  _results_controller->SetView (_results_view);
  _layered_layout->AddLayer (_results_view);
  _results_view->GetLayout ()->OnGeometryChanged.connect (sigc::mem_fun (this, &PlacesView::OnResultsViewGeometryChanged));

  _layered_layout->SetActiveLayer (_home_view);

  SetLayout (_layout);

  {
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    _bg_layer = new nux::ColorLayer (nux::Color (0.0f, 0.0f, 0.0f, 0.9f), true, rop);
  }

  // Register for all the events
  UBusServer *ubus = ubus_server_get_default ();
  ubus_server_register_interest (ubus, UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                 (UBusCallback)place_entry_activate_request,
                                 this);
  ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_CLOSE_REQUEST,
                                 (UBusCallback)&PlacesView::CloseRequest,
                                 this);
  ubus_server_register_interest (ubus, UBUS_PLACE_TILE_ACTIVATE_REQUEST,
                                 (UBusCallback)&PlacesView::OnResultActivated,
                                 this);
  ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_QUEUE_DRAW,
                                 (UBusCallback)&PlacesView::OnPlaceViewQueueDrawNeeded,
                                 this);

  _icon_loader = IconLoader::GetDefault ();

  SetActiveEntry (_home_entry, 0, "");

  //_layout->SetFocused (true);
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

  if ((ievent.e_event == nux::NUX_KEYDOWN) &&
   (ievent.GetKeySym () == NUX_VK_ESCAPE))
  {
    SetActiveEntry (NULL, 0, "");
    return TraverseInfo;
  }

  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    PlacesStyle      *style = PlacesStyle::GetDefault ();
    nux::BaseTexture *corner = style->GetDashCorner ();
    nux::Geometry     geo = GetAbsoluteGeometry ();
    nux::Geometry     fullscreen (geo.x + geo.width - corner->GetWidth (),
                                  geo.y + geo.height - corner->GetHeight (),
                                  corner->GetWidth (),
                                  corner->GetHeight ());

    if (fullscreen.IsPointInside (ievent.e_x, ievent.e_y))
    {
      _bg_blur_texture.Release ();
      fullscreen_request.emit ();

      return TraverseInfo |= nux::eMouseEventSolved;
    }
  }

  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);
  return ret;
}

void
PlacesView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  PlacesStyle  *style = PlacesStyle::GetDefault ();
  nux::Geometry geo = GetGeometry ();
  nux::Geometry geo_absolute = GetAbsoluteGeometry ();
  PlacesSettings::DashBlurType type = PlacesSettings::GetDefault ()->GetDashBlurType ();
  bool paint_blur = type != PlacesSettings::NO_BLUR;

  GfxContext.PushClippingRectangle (geo);

  nux::GetPainter ().PaintBackground (GfxContext, geo);

  GfxContext.GetRenderStates ().SetBlend (true);
  GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

  geo.height = _actual_height;
  nux::Geometry _bg_blur_geo = geo;

  if ((_size_mode == SIZE_MODE_HOVER))
  {
    nux::BaseTexture *bottom = style->GetDashBottomTile ();
    nux::BaseTexture *right = style->GetDashRightTile ();

    _bg_blur_geo.OffsetSize (-right->GetWidth (), -bottom->GetHeight ());
  }

  if (!_bg_blur_texture.IsValid () && paint_blur)
  {
    nux::ObjectPtr<nux::IOpenGLFrameBufferObject> current_fbo = nux::GetGpuDevice ()->GetCurrentFrameBufferObject ();
    nux::GetGpuDevice ()->DeactivateFrameBuffer ();
    
    GfxContext.SetViewport (0, 0, GfxContext.GetWindowWidth (), GfxContext.GetWindowHeight ());
    GfxContext.SetScissor (0, 0, GfxContext.GetWindowWidth (), GfxContext.GetWindowHeight ());
    GfxContext.GetRenderStates ().EnableScissor (false);

    nux::ObjectPtr <nux::IOpenGLBaseTexture> _bg_texture = GfxContext.CreateTextureFromBackBuffer (
    geo_absolute.x, geo_absolute.y, _bg_blur_geo.width, _bg_blur_geo.height);

    nux::TexCoordXForm texxform__bg;
    _bg_blur_texture = GfxContext.QRP_GetBlurTexture (0, 0, _bg_blur_geo.width, _bg_blur_geo.height, _bg_texture, texxform__bg, nux::Color::White, 1.0f, 2);

    if (current_fbo.IsValid ())
    { 
      current_fbo->Activate (true);
      GfxContext.Push2DWindow (current_fbo->GetWidth (), current_fbo->GetHeight ());
    }
    else
    {
      GfxContext.SetViewport (0, 0, GfxContext.GetWindowWidth (), GfxContext.GetWindowHeight ());
      GfxContext.Push2DWindow (GfxContext.GetWindowWidth (), GfxContext.GetWindowHeight ());
      GfxContext.ApplyClippingRectangle ();
    }
  }

  if (_bg_blur_texture.IsValid ()  && paint_blur)
  {
    nux::TexCoordXForm texxform_blur__bg;
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    gPainter.PushDrawTextureLayer (GfxContext, _bg_blur_geo,
                                   _bg_blur_texture,
                                   texxform_blur__bg,
                                   nux::Color::White,
                                   true,
                                   rop);
  }

  if (_size_mode == SIZE_MODE_HOVER)
  {
    nux::BaseTexture *corner = style->GetDashCorner ();
    nux::BaseTexture *bottom = style->GetDashBottomTile ();
    nux::BaseTexture *right = style->GetDashRightTile ();
    nux::BaseTexture *icon = style->GetDashFullscreenIcon ();
    nux::TexCoordXForm texxform;

    { // Background
      nux::Geometry bg = geo;
      bg.width -= corner->GetWidth ();
      bg.height -= corner->GetHeight ();

      _bg_layer->SetGeometry (bg);
      nux::GetPainter ().RenderSinglePaintLayer (GfxContext, bg, _bg_layer);
    }

    { // Corner
      texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap (nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

      GfxContext.QRP_1Tex (geo.x + (geo.width - corner->GetWidth ()),
                           geo.y + (geo.height - corner->GetHeight ()),
                           corner->GetWidth (),
                           corner->GetHeight (),
                           corner->GetDeviceTexture (),
                           texxform,
                           nux::Color::White);
    }

    { // Fullscreen toggle
      GfxContext.QRP_1Tex (geo.x + geo.width - corner->GetWidth (),
                           geo.y + geo.height - corner->GetHeight (),
                           icon->GetWidth (),
                           icon->GetHeight (),
                           icon->GetDeviceTexture (),
                           texxform,
                           nux::Color::White);
    }

    { // Bottom repeated texture
      int real_width = geo.width - corner->GetWidth ();
      int offset = real_width % bottom->GetWidth ();

      texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

      GfxContext.QRP_1Tex (geo.x - offset,
                           geo.y + (geo.height - bottom->GetHeight ()),
                           real_width + offset,
                           bottom->GetHeight (),
                           bottom->GetDeviceTexture (),
                           texxform,
                           nux::Color::White);
    }

    { // Right repeated texture
      int real_height = geo.height - corner->GetHeight ();
      int offset = real_height % right->GetHeight ();

      texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

      GfxContext.QRP_1Tex (geo.x +(geo.width - right->GetWidth ()),
                           geo.y - offset,
                           right->GetWidth (),
                           real_height + offset,
                           right->GetDeviceTexture (),
                           texxform,
                           nux::Color::White);
    }
  }
  else
  {
    _bg_layer->SetGeometry (geo);
    nux::GetPainter ().RenderSinglePaintLayer (GfxContext, geo, _bg_layer);
  }

  if (_bg_blur_texture.IsValid () && paint_blur)
    gPainter.PopBackground ();

  GfxContext.GetRenderStates ().SetBlend (false);

  GfxContext.PopClippingRectangle ();
}


void
PlacesView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  PlacesSettings::DashBlurType type = PlacesSettings::GetDefault ()->GetDashBlurType ();
  nux::Geometry clip_geo = GetGeometry ();
  bool paint_blur = type != PlacesSettings::NO_BLUR;
  int bgs = 1;

  clip_geo.height = _bg_layer->GetGeometry ().height;
  GfxContext.PushClippingRectangle (clip_geo);

  GfxContext.GetRenderStates ().SetBlend (true);
  GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

  if (_bg_blur_texture.IsValid () && paint_blur)
  {

    nux::TexCoordXForm texxform_blur__bg;
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    gPainter.PushTextureLayer (GfxContext, _bg_blur_geo,
                               _bg_blur_texture,
                               texxform_blur__bg,
                               nux::Color::White,
                               true,
                               rop);
    bgs++;
  }
 
  nux::GetPainter ().PushLayer (GfxContext, _bg_layer->GetGeometry (), _bg_layer);

  if (_layout)
    _layout->ProcessDraw (GfxContext, force_draw);

  nux::GetPainter ().PopBackground (bgs);

  GfxContext.GetRenderStates ().SetBlend (false);

  GfxContext.PopClippingRectangle ();
}

//
// PlacesView Methods
//
void
PlacesView::AboutToShow ()
{
  _bg_blur_texture.Release ();
}

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
  _search_bar->SetActiveEntry (_entry, section_id, search_string);

  _entry->ForeachGroup (sigc::mem_fun (this, &PlacesView::OnGroupAdded));
  _entry->ForeachResult (sigc::mem_fun (this, &PlacesView::OnResultAdded));

  _group_added_conn = _entry->group_added.connect (sigc::mem_fun (this, &PlacesView::OnGroupAdded));
  _result_added_conn = _entry->result_added.connect (sigc::mem_fun (this, &PlacesView::OnResultAdded));
  _result_removed_conn = _entry->result_removed.connect (sigc::mem_fun (this, &PlacesView::OnResultRemoved));
  
  if (_entry == _home_entry && (g_strcmp0 (search_string, "") == 0))
    _layered_layout->SetActiveLayer (_home_view);
  else
    _layered_layout->SetActiveLayer (_results_view);

  ReEvaluateShrinkMode ();  
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

gboolean
PlacesView::OnResizeFrame (PlacesView *self)
{
#define _LENGTH_ 200000
  gint64 diff;
  float  progress;
  float  last_height;

  diff = g_get_monotonic_time () - self->_resize_start_time;

  progress = diff/(float)_LENGTH_;
  
  last_height = self->_last_height;

  if (self->_target_height > self->_last_height)
  {
    self->_actual_height = last_height + ((self->_target_height - last_height) * progress);
  }
  else
  {
    self->_actual_height = last_height - ((last_height - self->_target_height) * progress);
  }

  if (diff > _LENGTH_)
  {
    self->_resize_id = 0;

    // Make sure the state is right
    self->_actual_height = self->_target_height;
  
    self->QueueDraw ();
    return FALSE;
  }

  self->QueueDraw ();
  return TRUE;
}

void
PlacesView::OnResultsViewGeometryChanged (nux::Area *view, nux::Geometry& view_geo)
{
#define UNEXPANDED_HOME_PADDING 12
  nux::BaseTexture *corner = PlacesStyle::GetDefault ()->GetDashCorner ();

  if (_shrink_mode == SHRINK_MODE_NONE)
  {
    _target_height = GetGeometry ().height;
    _actual_height = _target_height;
  }
  else
  {
    gint target_height = _search_bar->GetGeometry ().height;

    if (_layered_layout->GetActiveLayer () == _home_view)
    {
      if (_home_view->GetExpanded ())
        target_height += _home_view->GetLayout ()->GetContentHeight ();
      else
        target_height += _home_view->GetHeaderHeight () + UNEXPANDED_HOME_PADDING;
    }
    else
    {
      target_height += _results_view->GetLayout ()->GetContentHeight ();
    }

    target_height += corner->GetHeight ();
    if (target_height >= GetGeometry ().height)
      target_height = GetGeometry ().height;

    if (_target_height != target_height)
    {
      _target_height = target_height;
      _last_height = _actual_height;
      _resize_start_time = g_get_monotonic_time ();

      if (_resize_id)
        g_source_remove (_resize_id);
      _resize_id = g_timeout_add (15, (GSourceFunc)PlacesView::OnResizeFrame, this);
    }
   
    QueueDraw ();
  }
}

PlacesView::SizeMode
PlacesView::GetSizeMode ()
{
  return _size_mode;
}

void
PlacesView::SetSizeMode (SizeMode size_mode)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  if (_size_mode == size_mode)
    return;

  _size_mode = size_mode;

  if (_size_mode == SIZE_MODE_FULLSCREEN)
  {
    _h_spacer->SetMinimumWidth (1);
    _h_spacer->SetMaximumWidth (1);
    _v_spacer->SetMinimumHeight (1);
    _v_spacer->SetMaximumHeight (1);
  }
  else
  {
    nux::BaseTexture *corner = style->GetDashCorner ();
    _h_spacer->SetMinimumWidth (corner->GetWidth ());
    _h_spacer->SetMaximumWidth (corner->GetWidth ());
    _v_spacer->SetMinimumHeight (corner->GetHeight ());
    _v_spacer->SetMaximumHeight (corner->GetHeight ());
  }

  QueueDraw ();
}

//
// Model handlers
//
void
PlacesView::OnGroupAdded (PlaceEntry *entry, PlaceEntryGroup& group)
{
  _results_controller->AddGroup (entry, group);
}

void
PlacesView::OnResultAdded (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result)
{
  _results_controller->AddResult (entry, group, result);
}

void
PlacesView::OnResultRemoved (PlaceEntry *entry, PlaceEntryGroup& group, PlaceEntryResult& result)
{
  _results_controller->RemoveResult (entry, group, result);
}

void
PlacesView::OnResultActivated (GVariant *data, PlacesView *self)
{
  const char *uri;

  uri = g_variant_get_string (data, NULL);

  if (!uri || g_strcmp0 (uri, "") == 0)
  {
    g_warning ("Unable to launch tile does not have a URI");
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

void
PlacesView::OnPlaceViewQueueDrawNeeded (GVariant *data, PlacesView *self)
{
  self->QueueDraw ();
}

void
PlacesView::OnEntryActivated ()
{
  if (!_results_controller->ActivateFirst ())
    g_debug ("Cannot activate anything");
}

void
PlacesView::LoadPlaces ()
{
  std::vector<Place *>::iterator it, eit = _factory->GetPlaces ().end ();

  for (it = _factory->GetPlaces ().begin (); it != eit; ++it)
  {
    OnPlaceAdded (*it);
  }
}

void
PlacesView::OnPlaceAdded (Place *place)
{
  place->result_activated.connect (sigc::mem_fun (this, &PlacesView::OnPlaceResultActivated));
}

void
PlacesView::OnPlaceResultActivated (const char *uri, ActivationResult res)
{
  switch (res)
  {
    case FALLBACK:
      OnResultActivated (g_variant_new_string (uri), this);
      break;
    case SHOW_DASH:
      break;
    case HIDE_DASH:
      ubus_server_send_message (ubus_server_get_default (),
                                UBUS_PLACE_VIEW_CLOSE_REQUEST,
                                NULL);
      break;
    default:
      g_warning ("Activation result %d not supported", res);
      break;
  };
}

void
PlacesView::ReEvaluateShrinkMode ()
{
  if (_size_mode == SIZE_MODE_FULLSCREEN)
    _shrink_mode = SHRINK_MODE_NONE;

  if (_entry == _home_entry)
    _shrink_mode = SHRINK_MODE_CONTENTS;
  else
    _shrink_mode = SHRINK_MODE_NONE;
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

  if (self->GetFocused ())
  {
    // reset the focus
    self->SetFocused (false);
    self->SetFocused (true);
  }
  else
  {
    // Not focused but we really should be
    self->SetFocused (true);
  }

  g_free (id);
  g_free (search_string);
}

