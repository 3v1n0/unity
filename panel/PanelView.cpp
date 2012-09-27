// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <Nux/Layout.h>
#include <Nux/WindowCompositor.h>

#include <NuxGraphics/CairoGraphics.h>
#include <NuxGraphics/ImageSurface.h>
#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>

#include "unity-shared/PanelStyle.h"
#include "PanelIndicatorsView.h"
#include <UnityCore/Variant.h>

#include "unity-shared/UBusMessages.h"

#include "PanelView.h"

namespace
{
nux::logging::Logger logger("unity.panel.view");
const int refine_gradient_midpoint = 959;
}

namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(PanelView);

PanelView::PanelView(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , _is_dirty(true)
  , _opacity_maximized_toggle(false)
  , _needs_geo_sync(false)
  , _is_primary(false)
  , _overlay_is_open(false)
  , _opacity(1.0f)
  , _monitor(0)
  , _stored_dash_width(0)
  , _launcher_width(0)
{
  panel::Style::Instance().changed.connect(sigc::mem_fun(this, &PanelView::ForceUpdateBackground));

  _bg_layer.reset(new nux::ColorLayer(nux::Color(0xff595853), true));

  nux::ROPConfig rop;
  rop.Blend = true;
  nux::Color darken_colour = nux::Color(0.9f, 0.9f, 0.9f, 1.0f);
  
  if (Settings::Instance().GetLowGfxMode() == false)
  {
    rop.SrcBlend = GL_ZERO;
    rop.DstBlend = GL_SRC_COLOR;
  }
  //If we are in low gfx mode then change our darken_colour to our background colour.
  else
  {
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    darken_colour = _bg_color;
  }

  _bg_darken_layer.reset(new nux::ColorLayer(darken_colour, false, rop));

  _layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  _layout->SetContentDistribution(nux::eStackLeft);

  _menu_view = new PanelMenuView();
  AddPanelView(_menu_view, 1);

  SetCompositionLayout(_layout);

  _tray = new PanelTray();
  _layout->AddView(_tray, 0, nux::eCenter, nux::eFull);
  AddChild(_tray);

  _indicators = new PanelIndicatorsView();
  AddPanelView(_indicators, 0);

  _remote = indicator::DBusIndicators::Ptr(new indicator::DBusIndicators());
  _remote->on_object_added.connect(sigc::mem_fun(this, &PanelView::OnObjectAdded));
  _remote->on_object_removed.connect(sigc::mem_fun(this, &PanelView::OnObjectRemoved));
  _remote->on_entry_activate_request.connect(sigc::mem_fun(this, &PanelView::OnEntryActivateRequest));
  _remote->on_entry_activated.connect(sigc::mem_fun(this, &PanelView::OnEntryActivated));
  _remote->on_entry_show_menu.connect(sigc::mem_fun(this, &PanelView::OnEntryShowMenu));

  _ubus_manager.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED, sigc::mem_fun(this, &PanelView::OnBackgroundUpdate));
  _ubus_manager.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &PanelView::OnOverlayHidden));
  _ubus_manager.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &PanelView::OnOverlayShown));
  _ubus_manager.RegisterInterest(UBUS_DASH_SIZE_CHANGED, [&] (GVariant *data)
  {
    int width, height;
    g_variant_get(data, "(ii)", &width, &height);
    _stored_dash_width = width;
    QueueDraw();
  });
  
  _ubus_manager.RegisterInterest(UBUS_REFINE_STATUS, [this] (GVariant *data) 
  {
    gboolean status;
    g_variant_get(data, UBUS_REFINE_STATUS_FORMAT_STRING, &status);

    _refine_is_open = status;
  
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    nux::TexCoordXForm texxform;
    if (_refine_is_open)
    {
      _bg_refine_layer.reset(new nux::TextureLayer(_bg_refine_tex->GetDeviceTexture(), 
                             texxform, 
                             nux::color::White,
                             false,
                             rop));
    }
    else
    {
      _bg_refine_layer.reset(new nux::TextureLayer(_bg_refine_no_refine_tex->GetDeviceTexture(), 
                             texxform, 
                             nux::color::White,
                             false,
                             rop));
      
    }
    QueueDraw();
  });
  
  // request the latest colour from bghash
  _ubus_manager.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);

  _bg_effect_helper.owner = this;

  //FIXME (gord)- replace with async loading
  glib::Object<GdkPixbuf> pixbuf;
  glib::Error error;
  pixbuf = gdk_pixbuf_new_from_file(PKGDATADIR "/dash_sheen.png", &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << PKGDATADIR << "/dash_sheen.png" << ": " << error;
  }
  else
  {
    _panel_sheen.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
  }

  //FIXME (gord) like 12 months later, still not async loading! 
  pixbuf = gdk_pixbuf_new_from_file(PKGDATADIR "/refine_gradient_panel.png", &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << PKGDATADIR << "/refine_gradient_panel.png";
  }
  else
  {
    _bg_refine_tex.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
  }

  //FIXME (gord) like 12 months later, still not async loading! 
  pixbuf = gdk_pixbuf_new_from_file(PKGDATADIR "/refine_gradient_panel_no_refine.png", &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << PKGDATADIR << "/refine_gradient_panel_no_refine.png";
  }
  else
  {
    _bg_refine_no_refine_tex.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
  }

  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::TexCoordXForm texxform;
  _bg_refine_layer.reset(new nux::TextureLayer(_bg_refine_tex->GetDeviceTexture(), 
                         texxform, 
                         nux::color::White,
                         false,
                         rop));

  //FIXME (gord) like 12 months later, still not async loading! 
  pixbuf = gdk_pixbuf_new_from_file(PKGDATADIR "/refine_gradient_panel_single_column.png", &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << PKGDATADIR << "/refine_gradient_panel_single_column.png";
  }
  else
  {
    _bg_refine_single_column_tex.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
  }

  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  _bg_refine_single_column_layer.reset(new nux::TextureLayer(_bg_refine_single_column_tex->GetDeviceTexture(), 
                         texxform, 
                         nux::color::White,
                         false,
                         rop));

}

PanelView::~PanelView()
{
  for (auto conn : _on_indicator_updated_connections)
    conn.disconnect();

  for (auto conn : _maximized_opacity_toggle_connections)
    conn.disconnect();

  indicator::EntryLocationMap locations;
  _remote->SyncGeometries(GetName() + boost::lexical_cast<std::string>(_monitor), locations);
}

Window PanelView::GetTrayXid() const
{
  if (!_tray)
    return 0;

  return _tray->xid();
}

void PanelView::SetLauncherWidth(int width)
{
  _launcher_width = width;
  QueueDraw();
}

void PanelView::OnBackgroundUpdate(GVariant *data)
{
  gdouble red, green, blue, alpha;
  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);

  _bg_color.red = red;
  _bg_color.green = green;
  _bg_color.blue = blue;
  _bg_color.alpha = alpha;

  ForceUpdateBackground();
}

void PanelView::OnOverlayHidden(GVariant* data)
{
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor);

  if (_monitor == overlay_monitor && overlay_identity.Str() == _active_overlay)
  {
    if (_opacity >= 1.0f)
      _bg_effect_helper.enabled = false;

    _overlay_is_open = false;
    _active_overlay = "";
    _menu_view->OverlayHidden();
    _indicators->OverlayHidden();
    SetAcceptKeyNavFocusOnMouseDown(true);
    ForceUpdateBackground();
  }
}

void PanelView::OnOverlayShown(GVariant* data)
{
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor);

  if (_monitor == overlay_monitor)
  {
    _bg_effect_helper.enabled = true;
    _active_overlay = overlay_identity.Str();
    _overlay_is_open = true;
    _indicators->OverlayShown();
    _menu_view->OverlayShown();
    SetAcceptKeyNavFocusOnMouseDown(false);
    ForceUpdateBackground();
  }
}

void PanelView::AddPanelView(PanelIndicatorsView* child,
                             unsigned int stretchFactor)
{
  _layout->AddView(child, stretchFactor, nux::eCenter, nux::eFull);
  auto conn = child->on_indicator_updated.connect(sigc::mem_fun(this, &PanelView::OnIndicatorViewUpdated));
  _on_indicator_updated_connections.push_back(conn);
  AddChild(child);
}

std::string PanelView::GetName() const
{
  return "UnityPanel";
}

void PanelView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
  .add("backend", "remote")
  .add("monitor", _monitor)
  .add("active", IsActive())
  .add(GetAbsoluteGeometry());
}

void
PanelView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  UpdateBackground();

  GfxContext.PushClippingRectangle(geo);

  if ((_overlay_is_open || (_opacity != 1.0f && _opacity != 0.0f)))
  {
    nux::Geometry const& geo_absolute = GetAbsoluteGeometry();
    nux::Geometry blur_geo(geo_absolute.x, geo_absolute.y, geo.width, geo.height);

    if (BackgroundEffectHelper::blur_type != BLUR_NONE)
    {
      _bg_blur_texture = _bg_effect_helper.GetBlurRegion(blur_geo);
    }
    else
    {
      _bg_blur_texture = _bg_effect_helper.GetRegion(blur_geo);
    }

    if (_bg_blur_texture.IsValid() && (_overlay_is_open || _opacity != 1.0f))
    {
      nux::TexCoordXForm texxform_blur_bg;
      texxform_blur_bg.flip_v_coord = true;
      texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform_blur_bg.uoffset = geo.x / static_cast<float>(geo_absolute.width);
      texxform_blur_bg.voffset = geo.y / static_cast<float>(geo_absolute.height);

      nux::ROPConfig rop;
      rop.Blend = false;
      rop.SrcBlend = GL_ONE;
      rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

      GfxContext.PushClippingRectangle(geo);

#ifndef NUX_OPENGLES_20
      if (GfxContext.UsingGLSLCodePath())
        gPainter.PushDrawCompositionLayer(GfxContext, geo,
                                          _bg_blur_texture,
                                          texxform_blur_bg,
                                          nux::color::White,
                                          _bg_color,
                                          nux::LAYER_BLEND_MODE_OVERLAY,
                                          true, rop);
      else
        gPainter.PushDrawTextureLayer(GfxContext, geo,
                                      _bg_blur_texture,
                                      texxform_blur_bg,
                                      nux::color::White,
                                      true,
                                      rop);
#else
        gPainter.PushDrawCompositionLayer(GfxContext, geo,
                                          _bg_blur_texture,
                                          texxform_blur_bg,
                                          nux::color::White,
                                          _bg_color,
                                          nux::LAYER_BLEND_MODE_OVERLAY,
                                          true, rop);
#endif

      GfxContext.PopClippingRectangle();
    }

    if (Settings::Instance().GetLowGfxMode() == false)
    {
      if (_overlay_is_open)
      {
        nux::GetPainter().RenderSinglePaintLayer(GfxContext, geo, _bg_darken_layer.get());
      
        GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        nux::TexCoordXForm refine_texxform;
        
        int refine_x_pos = geo.x + (_stored_dash_width - refine_gradient_midpoint);

        refine_x_pos += _launcher_width;
        GfxContext.QRP_1Tex(refine_x_pos, 
                            geo.y,
                            _bg_refine_tex->GetWidth(), 
                            _bg_refine_tex->GetHeight(),
                            _bg_refine_tex->GetDeviceTexture(),
                            refine_texxform,
                            nux::color::White);
        
        GfxContext.QRP_1Tex(refine_x_pos + _bg_refine_tex->GetWidth(),
                            geo.y,
                            geo.width,
                            geo.height,
                            _bg_refine_single_column_tex->GetDeviceTexture(),
                            refine_texxform,
                            nux::color::White);
      }
    }
  }

  if (!_overlay_is_open || GfxContext.UsingGLSLCodePath() == false)
    nux::GetPainter().RenderSinglePaintLayer(GfxContext, geo, _bg_layer.get());

  GfxContext.PopClippingRectangle();

  if (_needs_geo_sync)
  {
    SyncGeometries();
    _needs_geo_sync = false;
  }
}

void
PanelView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  int bgs = 1;

  GfxContext.PushClippingRectangle(geo);

  GfxContext.GetRenderStates().SetBlend(true);
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  if (_bg_blur_texture.IsValid() &&
      (_overlay_is_open || (_opacity != 1.0f && _opacity != 0.0f)))
  {
    nux::Geometry const& geo_absolute = GetAbsoluteGeometry();
    nux::TexCoordXForm texxform_blur_bg;
    texxform_blur_bg.flip_v_coord = true;
    texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform_blur_bg.uoffset = geo.x / static_cast<float>(geo_absolute.width);
    texxform_blur_bg.voffset = geo.y / static_cast<float>(geo_absolute.height);

    nux::ROPConfig rop;
    rop.Blend = false;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

#ifndef NUX_OPENGLES_20
    if (GfxContext.UsingGLSLCodePath())
      gPainter.PushCompositionLayer(GfxContext, geo,
                                    _bg_blur_texture,
                                    texxform_blur_bg,
                                    nux::color::White,
                                    _bg_color,
                                    nux::LAYER_BLEND_MODE_OVERLAY,
                                    true,
                                    rop);
    else
      gPainter.PushTextureLayer(GfxContext, geo,
                                _bg_blur_texture,
                                texxform_blur_bg,
                                nux::color::White,
                                true,
                                rop);

#else
      gPainter.PushCompositionLayer(GfxContext, geo,
                                    _bg_blur_texture,
                                    texxform_blur_bg,
                                    nux::color::White,
                                    _bg_color,
                                    nux::LAYER_BLEND_MODE_OVERLAY,
                                    true,
                                    rop);
#endif
    bgs++;

    if (_overlay_is_open)
    {
      nux::GetPainter().PushLayer(GfxContext, geo, _bg_darken_layer.get());
      bgs++;
      
      nux::Geometry refine_geo = geo;
      
      int refine_x_pos = geo.x + (_stored_dash_width - refine_gradient_midpoint);
      refine_x_pos += _launcher_width;
      
      refine_geo.x = refine_x_pos;
      refine_geo.width = _bg_refine_tex->GetWidth();
      refine_geo.height = _bg_refine_tex->GetHeight();

      nux::GetPainter().PushLayer(GfxContext, refine_geo, _bg_refine_layer.get());
      bgs++;

      refine_geo.x += refine_geo.width;
      refine_geo.width = geo.width;
      refine_geo.height = geo.height;
      nux::GetPainter().PushLayer(GfxContext, refine_geo, _bg_refine_single_column_layer.get());
      bgs++;
    }
  }

  if (!_overlay_is_open || GfxContext.UsingGLSLCodePath() == false)
    gPainter.PushLayer(GfxContext, geo, _bg_layer.get());

  if (_overlay_is_open)
  {
    // apply the shine
    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);

    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_DST_COLOR;
    rop.DstBlend = GL_ONE;
    nux::GetPainter().PushTextureLayer(GfxContext, geo,
                                       _panel_sheen->GetDeviceTexture(),
                                       texxform,
                                       nux::color::White,
                                       false,
                                       rop);
  }
  _layout->ProcessDraw(GfxContext, force_draw);

  gPainter.PopBackground(bgs);

  GfxContext.GetRenderStates().SetBlend(false);
  GfxContext.PopClippingRectangle();
}

void
PanelView::UpdateBackground()
{
  nux::Geometry const& geo = GetGeometry();

  if (!_is_dirty && geo == _last_geo)
    return;

  _last_geo = geo;
  _is_dirty = false;

  guint32 maximized_win = _menu_view->GetMaximizedWindow();

  if (_overlay_is_open)
  {
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    _bg_layer.reset(new nux::ColorLayer (_bg_color, true, rop));
  }
  else
  {
    WindowManager* wm = WindowManager::Default();
    double opacity = _opacity;

    if (_opacity_maximized_toggle && (wm->IsExpoActive() ||
        (maximized_win != 0 && !wm->IsWindowObscured(maximized_win))))
    {
      opacity = 1.0f;
    }

    nux::NBitmapData* bitmap = panel::Style::Instance().GetBackground(geo.width, geo.height, opacity);
    nux::BaseTexture* texture2D = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();

    texture2D->Update(bitmap);
    delete bitmap;

    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    nux::Color col = nux::color::White;

    _bg_layer.reset(new nux::TextureLayer(texture2D->GetDeviceTexture(),
                                      texxform,
                                      col,
                                      true,
                                      rop));
    texture2D->UnReference();
  }

  NeedRedraw();
}

void PanelView::ForceUpdateBackground()
{
  _is_dirty = true;
  UpdateBackground();

  _indicators->QueueDraw();
  _menu_view->QueueDraw();
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
  if (proxy->IsAppmenu())
  {
    _menu_view->AddIndicator(proxy);
  }
  else
  {
    _indicators->AddIndicator(proxy);
  }

  ComputeContentSize();
  NeedRedraw();
}

void PanelView::OnObjectRemoved(indicator::Indicator::Ptr const& proxy)
{
  if (proxy->IsAppmenu())
  {
    _menu_view->RemoveIndicator(proxy);
  }
  else
  {
    _indicators->RemoveIndicator(proxy);
  }

  ComputeContentSize();
  NeedRedraw();
}

void PanelView::OnIndicatorViewUpdated(PanelIndicatorEntryView* view)
{
  _needs_geo_sync = true;
  ComputeContentSize();
}

void PanelView::OnMenuPointerMoved(int x, int y)
{
  nux::Geometry const& geo = GetAbsoluteGeometry();

  if (geo.IsPointInside(x, y))
  {
    PanelIndicatorEntryView* view = nullptr;

    if (_menu_view->GetControlsActive())
      view = _menu_view->ActivateEntryAt(x, y);

    if (!view) _indicators->ActivateEntryAt(x, y);

    _menu_view->SetMousePosition(x, y);
  }
  else
  {
    _menu_view->SetMousePosition(-1, -1);
  }
}

void PanelView::OnEntryActivateRequest(std::string const& entry_id)
{
  if (!IsActive())
    return;

  bool ret;

  ret = _menu_view->ActivateEntry(entry_id, 0);
  if (!ret) _indicators->ActivateEntry(entry_id, 0);
}

bool PanelView::TrackMenuPointer()
{
  auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  if (_tracked_pointer_pos != mouse)
  {
    OnMenuPointerMoved(mouse.x, mouse.y);
    _tracked_pointer_pos = mouse;
  }

  return true;
}

void PanelView::OnEntryActivated(std::string const& entry_id, nux::Rect const& geo)
{
  bool active = (entry_id.size() > 0);
  if (active && !_track_menu_pointer_timeout)
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
    _track_menu_pointer_timeout.reset(new glib::Timeout(16));
    _track_menu_pointer_timeout->Run(sigc::mem_fun(this, &PanelView::TrackMenuPointer));
  }
  else if (!active)
  {
    _track_menu_pointer_timeout.reset();
    _menu_view->NotifyAllMenusClosed();
    _tracked_pointer_pos = {-1, -1};
  }

  _ubus_manager.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
}

void PanelView::OnEntryShowMenu(std::string const& entry_id, unsigned int xid,
                                int x, int y, unsigned int button,
                                unsigned int timestamp)
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
  nux::GetWindowThread()->ProcessForeignEvent(e, NULL);
  // --------------------------------------------------------------------------
}

//
// Useful Public Methods
//

bool PanelView::FirstMenuShow() const
{
  bool ret = false;

  if (!IsActive())
    return ret;

  ret = _menu_view->ActivateIfSensitive();
  if (!ret) _indicators->ActivateIfSensitive();

  return ret;
}

void PanelView::SetOpacity(float opacity)
{
  if (_opacity == opacity)
    return;

  _opacity = opacity;
  _bg_effect_helper.enabled = (_opacity < 1.0f || _overlay_is_open);

  ForceUpdateBackground();
}

void PanelView::SetMenuShowTimings(int fadein, int fadeout, int discovery,
                                   int discovery_fadein, int discovery_fadeout)
{
  _menu_view->SetMenuShowTimings(fadein, fadeout, discovery, discovery_fadein, discovery_fadeout);
}

void PanelView::SetOpacityMaximizedToggle(bool enabled)
{
  if (_opacity_maximized_toggle != enabled)
  {
    if (enabled)
    {
      auto win_manager = WindowManager::Default();
      auto update_bg_lambda = [&](guint32) { ForceUpdateBackground(); };
      auto conn = &_maximized_opacity_toggle_connections;

      conn->push_back(win_manager->window_minimized.connect(update_bg_lambda));
      conn->push_back(win_manager->window_unminimized.connect(update_bg_lambda));
      conn->push_back(win_manager->window_maximized.connect(update_bg_lambda));
      conn->push_back(win_manager->window_restored.connect(update_bg_lambda));
      conn->push_back(win_manager->window_mapped.connect(update_bg_lambda));
      conn->push_back(win_manager->window_unmapped.connect(update_bg_lambda));
      conn->push_back(win_manager->initiate_expo.connect(
        sigc::mem_fun(this, &PanelView::ForceUpdateBackground)));
      conn->push_back(win_manager->terminate_expo.connect(
        sigc::mem_fun(this, &PanelView::ForceUpdateBackground)));
      conn->push_back(win_manager->compiz_screen_viewport_switch_ended.connect(
        sigc::mem_fun(this, &PanelView::ForceUpdateBackground)));
    }
    else
    {
      for (auto conn : _maximized_opacity_toggle_connections)
        conn.disconnect();

      _maximized_opacity_toggle_connections.clear();
    }

    _opacity_maximized_toggle = enabled;
    ForceUpdateBackground();
  }
}

bool PanelView::GetPrimary() const
{
  return _is_primary;
}

void PanelView::SetPrimary(bool primary)
{
  _is_primary = primary;
}

void PanelView::SyncGeometries()
{
  indicator::EntryLocationMap locations;
  std::string panel_id = GetName() + boost::lexical_cast<std::string>(_monitor);

  if (_menu_view->GetControlsActive())
    _menu_view->GetGeometryForSync(locations);

  _indicators->GetGeometryForSync(locations);
  _remote->SyncGeometries(panel_id, locations);
}

void PanelView::SetMonitor(int monitor)
{
  _monitor = monitor;
  _menu_view->SetMonitor(monitor);
}

int PanelView::GetMonitor() const
{
  return _monitor;
}

bool PanelView::IsActive() const
{
  return _menu_view->GetControlsActive();
}

} // namespace unity
