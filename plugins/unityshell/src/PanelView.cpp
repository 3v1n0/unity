// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <Nux/Layout.h>
#include <Nux/WindowCompositor.h>

#include <NuxImage/CairoGraphics.h>
#include <NuxImage/ImageSurface.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>

#include "PanelStyle.h"
#include "PanelIndicatorObjectView.h"
#include <UnityCore/Variant.h>

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PanelView.h"


namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(PanelView);

PanelView::PanelView(NUX_FILE_LINE_DECL)
  :   View(NUX_FILE_LINE_PARAM),
      _is_dirty(true),
      _opacity(1.0f),
      _is_primary(false),
      _monitor(0),
      _dash_is_open(false),
      blur_type_(BLUR_ACTIVE)
{
  _needs_geo_sync = false;
  _style = new PanelStyle();
  _style->changed.connect(sigc::mem_fun(this, &PanelView::ForceUpdateBackground));

  _bg_layer = new nux::ColorLayer(nux::Color(0xff595853), true);

  _layout = new nux::HLayout("", NUX_TRACKER_LOCATION);

  // Home button - not an indicator view
  _home_button = new PanelHomeButton();
  //_layout->AddView(_home_button, 0, nux::eCenter, nux::eFull);
  AddChild(_home_button);

  _menu_view = new PanelMenuView();
  AddPanelView(_menu_view, 1);

  SetCompositionLayout(_layout);

  // Pannel tray shouldn't be an indicator view
  _tray = new PanelTray();
  _layout->AddView(_tray, 0, nux::eCenter, nux::eFull);
  AddChild(_tray);

  _remote = indicator::DBusIndicators::Ptr(new indicator::DBusIndicators());
  _remote->on_object_added.connect(sigc::mem_fun(this, &PanelView::OnObjectAdded));
  _remote->on_entry_activate_request.connect(sigc::mem_fun(this, &PanelView::OnEntryActivateRequest));
  _remote->on_entry_activated.connect(sigc::mem_fun(this, &PanelView::OnEntryActivated));
  _remote->on_synced.connect(sigc::mem_fun(this, &PanelView::OnSynced));
  _remote->on_entry_show_menu.connect(sigc::mem_fun(this, &PanelView::OnEntryShowMenu));
  
   UBusServer *ubus = ubus_server_get_default();

   _handle_bg_color_update = ubus_server_register_interest(ubus, UBUS_BACKGROUND_COLOR_CHANGED,
                                                          (UBusCallback)&PanelView::OnBackgroundUpdate,
                                                          this);

   _handle_dash_hidden = ubus_server_register_interest(ubus, UBUS_PLACE_VIEW_HIDDEN,
                                                      (UBusCallback)&PanelView::OnDashHidden,
                                                      this);

   _handle_dash_shown = ubus_server_register_interest(ubus, UBUS_PLACE_VIEW_SHOWN,
                                                     (UBusCallback)&PanelView::OnDashShown,
                                                     this);
   // request the latest colour from bghash
   ubus_server_send_message (ubus, UBUS_BACKGROUND_REQUEST_COLOUR_EMIT, NULL);

  _track_menu_pointer_id = 0;
}

PanelView::~PanelView()
{
  if (_track_menu_pointer_id)
    g_source_remove(_track_menu_pointer_id);
  _style->UnReference();
  UBusServer *ubus = ubus_server_get_default();
  ubus_server_unregister_interest(ubus, _handle_bg_color_update);
  ubus_server_unregister_interest(ubus, _handle_dash_hidden);
  ubus_server_unregister_interest(ubus, _handle_dash_shown);
  
  delete _bg_layer;
}

void PanelView::OnBackgroundUpdate (GVariant *data, PanelView *self)
{
  gdouble red, green, blue, alpha;
  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
  self->_bg_color = nux::Color (red, green, blue, alpha);
  self->ForceUpdateBackground();
}

void PanelView::OnDashHidden(GVariant* data, PanelView* self)
{
  self->_dash_is_open = false;
  self->ForceUpdateBackground();
}

void PanelView::OnDashShown(GVariant* data, PanelView* self)
{
  self->_dash_is_open = true;
  self->ForceUpdateBackground();
}

void PanelView::AddPanelView(PanelIndicatorObjectView* child,
                             unsigned int stretchFactor)
{
  _layout->AddView(child, stretchFactor, nux::eCenter, nux::eFull);
  AddChild(child);
  children_.push_back(child);
}

const gchar* PanelView::GetName()
{
  return "Panel";
}

const gchar*
PanelView::GetChildsName()
{
  return "indicators";
}

void PanelView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
  .add("backend", "remote")
  .add("service-name", _remote->name())
  .add("service-unique-name", _remote->owner_name())
  .add("using-local-service", _remote->using_local_service())
  .add(GetGeometry());
}

long
PanelView::ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  ret = _layout->ProcessEvent(ievent, ret, ProcessEventInfo);
  return ret;
}

void
PanelView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();
  nux::Geometry geo_absolute = GetAbsoluteGeometry();
  UpdateBackground();

  GfxContext.PushClippingRectangle(GetGeometry());

  if (!bg_blur_texture_.IsValid() && blur_type_ != BLUR_NONE && (_dash_is_open || _opacity != 1.0f))
  {
    nux::Geometry blur_geo(geo_absolute.x, geo_absolute.y, geo.width, geo.height);
    bg_blur_texture_ = bg_effect_helper_.GetBlurRegion(blur_geo, true);
  }

  if (bg_blur_texture_.IsValid() && blur_type_ != BLUR_NONE && (_dash_is_open || _opacity != 1.0f))
  {
    nux::TexCoordXForm texxform_blur_bg;
    texxform_blur_bg.flip_v_coord = true;
    texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform_blur_bg.uoffset = ((float) geo.x) / geo_absolute.width;
    texxform_blur_bg.voffset = ((float) geo.y) / geo_absolute.height;

    nux::ROPConfig rop;
    rop.Blend = false;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    nux::Geometry bg_clip = geo;
    GfxContext.PushClippingRectangle(bg_clip);

    gPainter.PushDrawTextureLayer(GfxContext, geo,
                                  bg_blur_texture_,
                                  texxform_blur_bg,
                                  nux::color::White,
                                  true,
                                  rop);

    GfxContext.PopClippingRectangle();
  }

  nux::GetPainter().RenderSinglePaintLayer(GfxContext, GetGeometry(), _bg_layer);

  GfxContext.PopClippingRectangle();

  if (_needs_geo_sync && _menu_view->GetControlsActive())
  {
    SyncGeometries();
    _needs_geo_sync = false;
  }
}

void
PanelView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();
  int bgs = 1;

  GfxContext.PushClippingRectangle(GetGeometry());

  GfxContext.GetRenderStates().SetBlend(true);
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  if (bg_blur_texture_.IsValid() && blur_type_ != BLUR_NONE && (_dash_is_open || _opacity != 1.0f))
  {
    nux::Geometry geo_absolute = GetAbsoluteGeometry ();
    nux::TexCoordXForm texxform_blur_bg;
    texxform_blur_bg.flip_v_coord = true;
    texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform_blur_bg.uoffset = ((float) geo.x) / geo_absolute.width;
    texxform_blur_bg.voffset = ((float) geo.y) / geo_absolute.height;

    nux::ROPConfig rop;
    rop.Blend = false;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    gPainter.PushTextureLayer(GfxContext, geo,
                              bg_blur_texture_,
                              texxform_blur_bg,
                              nux::color::White,
                              true,
                              rop);
    bgs++;
  }

  gPainter.PushLayer(GfxContext, GetGeometry(), _bg_layer);

  _layout->ProcessDraw(GfxContext, force_draw);

  gPainter.PopBackground(bgs);

  GfxContext.GetRenderStates().SetBlend(false);
  GfxContext.PopClippingRectangle();

  if (blur_type_ == BLUR_ACTIVE)
    bg_blur_texture_.Release();
}

void
PanelView::PreLayoutManagement()
{
  nux::View::PreLayoutManagement();
}

long
PanelView::PostLayoutManagement(long LayoutResult)
{
  return nux::View::PostLayoutManagement(LayoutResult);
}

void
PanelView::UpdateBackground()
{
  nux::Geometry geo = GetGeometry();

  if (geo.width == _last_width && geo.height == _last_height && !_is_dirty)
    return;

  _last_width = geo.width;
  _last_height = geo.height;
  _is_dirty = false;
  
  if (_dash_is_open)
  {
    if (_bg_layer)
      delete _bg_layer;
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    _bg_layer = new nux::ColorLayer (_bg_color, true, rop);
  }
  else
  {
    nux::NBitmapData* bitmap = _style->GetBackground(geo.width, geo.height);
    nux::BaseTexture* texture2D = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
    texture2D->Update(bitmap);
    delete bitmap;

    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    if (_bg_layer)
      delete _bg_layer;

    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    nux::Color col = nux::color::White;
    col.alpha = _opacity;

    _bg_layer = new nux::TextureLayer(texture2D->GetDeviceTexture(),
                                      texxform,
                                      col,
                                      true,
                                      rop);
    texture2D->UnReference();
  }

  NeedRedraw();
}

void PanelView::ForceUpdateBackground()
{
  _is_dirty = true;
  UpdateBackground();

  for (Children::iterator i = children_.begin(), end = children_.end(); i != end; ++i)
  {
    (*i)->QueueDraw();
  }
  // The home button isn't an indicator view.
  _home_button->QueueDraw();
  _tray->QueueDraw();
  QueueDraw();
}

//
// Signals
//
void PanelView::OnObjectAdded(indicator::Indicator::Ptr const& proxy)
{
  // Appmenu is treated differently as it needs to expand
  // We could do this in a more special way, but who has the time for special?
  if (proxy->name().find("appmenu") != std::string::npos)
  {
    _menu_view->SetProxy(proxy);
  }
  else
  {
    PanelIndicatorObjectView* view = new PanelIndicatorObjectView(proxy);
    AddPanelView(view, 0);
  }

  _layout->SetContentDistribution(nux::eStackLeft);

  ComputeChildLayout();
  NeedRedraw();
}

void PanelView::OnMenuPointerMoved(int x, int y)
{
  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Geometry hgeo = _home_button->GetAbsoluteGeometry();

  if (x <= (hgeo.x + hgeo.width))
    return;

  if (geo.IsPointInside(x, y))
  {
    for (Children::iterator i = children_.begin(), end = children_.end(); i != end; ++i)
    {
      PanelIndicatorObjectView* view = *i;

      if (view == _menu_view && _menu_view->HasOurWindowFocused())
        continue;

      geo = view->GetAbsoluteGeometry();
      if (geo.IsPointInside(x, y))
      {
        view->OnPointerMoved(x, y);
        break;
      }
    }
  }
}

void PanelView::OnEntryActivateRequest(std::string const& entry_id)
{
  if (!_menu_view->GetControlsActive())
    return;

  bool activated = false;
  for (Children::iterator i = children_.begin(), end = children_.end();
       i != end && !activated; ++i)
  {
    PanelIndicatorObjectView* view = *i;
    activated = view->ActivateEntry(entry_id);
  }
}

static gboolean track_menu_pointer(gpointer data)
{
  PanelView *self = (PanelView*)data;
  gint x, y;
  gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
  self->OnMenuPointerMoved(x, y);
  return TRUE;
}

void PanelView::OnEntryActivated(std::string const& entry_id)
{
  bool active = (entry_id.size() > 0);
  if (active && !_track_menu_pointer_id)
  {
    //
    // Track menus being scrubbed at 60Hz (about every 16 millisec)
    // It might sound ugly, but it's far nicer (and more responsive) than the
    // code it replaces which used to capture motion events in another process
    // (unity-panel-service) and send them to us over dbus.
    // NOTE: The reason why we have to use a timer instead of tracking motion
    // events is because the motion events will never be delivered to this
    // process. All the motion events will go to unity-panel-service while
    // scrubbing because the active panel menu has (needs) the pointer grab.
    //
    _track_menu_pointer_id = g_timeout_add(16, track_menu_pointer, this);
  }
  else if (!active)
  {
    if (_track_menu_pointer_id)
    {
      g_source_remove(_track_menu_pointer_id);
      _track_menu_pointer_id = 0;
    }
    _menu_view->AllMenusClosed();
  }
}

void PanelView::OnSynced()
{
  _needs_geo_sync = true;
}

void PanelView::OnEntryShowMenu(std::string const& entry_id,
                                int x, int y, int timestamp, int button)
{
  Display* d = nux::GetGraphicsDisplay()->GetX11Display();
  XUngrabPointer(d, CurrentTime);
  XFlush(d);

  // --------------------------------------------------------------------------
  // FIXME: This is a workaround until the non-paired events issue is fixed in
  // nux
  XButtonEvent ev =
  {
    ButtonRelease,
    0,
    False,
    d,
    0,
    0,
    0,
    CurrentTime,
    x, y,
    x, y,
    0,
    Button1,
    True
  };
  XEvent* e = (XEvent*)&ev;
  nux::GetGraphicsThread()->ProcessForeignEvent(e, NULL);
  // --------------------------------------------------------------------------
}

//
// Useful Public Methods
//
PanelHomeButton* PanelView::GetHomeButton()
{
  return _home_button;
}

void PanelView::StartFirstMenuShow()
{
}

void PanelView::EndFirstMenuShow()
{
  if (!_menu_view->GetControlsActive())
    return;

  bool activated = false;
  for (Children::iterator i = children_.begin(), end = children_.end();
       i != end && !activated; ++i)
  {
    PanelIndicatorObjectView* view = *i;
    activated = view->ActivateIfSensitive();
  }
}

void
PanelView::SetOpacity(float opacity)
{
  if (_opacity == opacity)
    return;

  _opacity = opacity;

  _home_button->SetOpacity(opacity);
  ForceUpdateBackground();
}

bool
PanelView::GetPrimary()
{
  return _is_primary;
}

void
PanelView::SetPrimary(bool primary)
{
  _is_primary = primary;

  _home_button->SetVisible(primary);
}

void PanelView::SyncGeometries()
{
  indicator::EntryLocationMap locations;
  for (Children::iterator i = children_.begin(), end = children_.end(); i != end; ++i)
  {
    (*i)->GetGeometryForSync(locations);
  }
  _remote->SyncGeometries(GetName(), locations);
}

void
PanelView::SetMonitor(int monitor)
{
  _monitor = monitor;
  _menu_view->SetMonitor(monitor);
}

void PanelView::SetBlurType(BlurType type)
{
  blur_type_ = type;
  QueueDraw();
}

} // namespace unity
