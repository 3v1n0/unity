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
#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>

#include "unity-shared/PanelStyle.h"
#include "unity-shared/TextureCache.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"
#include <UnityCore/Variant.h>

#include "PanelIndicatorsView.h"

#include "PanelView.h"

DECLARE_LOGGER(logger, "unity.panel.view");

namespace
{
const int refine_gradient_midpoint = 959;
}

namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(PanelView);

PanelView::PanelView(MockableBaseWindow* parent, indicator::DBusIndicators::Ptr const& remote, NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , parent_(parent)
  , remote_(remote)
  , is_dirty_(true)
  , opacity_maximized_toggle_(false)
  , needs_geo_sync_(false)
  , overlay_is_open_(false)
  , opacity_(1.0f)
  , monitor_(0)
  , stored_dash_width_(0)
  , launcher_width_(64)
{
  panel::Style::Instance().changed.connect(sigc::mem_fun(this, &PanelView::ForceUpdateBackground));
  WindowManager::Default().average_color.changed.connect(sigc::mem_fun(this, &PanelView::OnBackgroundUpdate));

  bg_layer_.reset(new nux::ColorLayer(nux::Color(0xff595853), true));

  nux::ROPConfig rop;
  rop.Blend = true;
  nux::Color darken_colour = nux::Color(0.9f, 0.9f, 0.9f, 1.0f);

  if (!Settings::Instance().GetLowGfxMode())
  {
    rop.SrcBlend = GL_ZERO;
    rop.DstBlend = GL_SRC_COLOR;
  }
  //If we are in low gfx mode then change our darken_colour to our background colour.
  else
  {
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    darken_colour = WindowManager::Default().average_color();
  }

  bg_darken_layer_.reset(new nux::ColorLayer(darken_colour, false, rop));

  layout_ = new nux::HLayout("", NUX_TRACKER_LOCATION);
  layout_->SetContentDistribution(nux::MAJOR_POSITION_START);

  menu_view_ = new PanelMenuView();
  AddPanelView(menu_view_, 1);

  SetCompositionLayout(layout_);

  tray_ = new PanelTray();
  layout_->AddView(tray_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  AddChild(tray_);

  indicators_ = new PanelIndicatorsView();
  AddPanelView(indicators_, 0);

  for (auto const& object : remote_->GetIndicators())
    OnObjectAdded(object);

  remote_->on_object_added.connect(sigc::mem_fun(this, &PanelView::OnObjectAdded));
  remote_->on_object_removed.connect(sigc::mem_fun(this, &PanelView::OnObjectRemoved));
  remote_->on_entry_activate_request.connect(sigc::mem_fun(this, &PanelView::OnEntryActivateRequest));
  remote_->on_entry_activated.connect(sigc::mem_fun(this, &PanelView::OnEntryActivated));
  remote_->on_entry_show_menu.connect(sigc::mem_fun(this, &PanelView::OnEntryShowMenu));

  ubus_manager_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &PanelView::OnOverlayHidden));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &PanelView::OnOverlayShown));
  ubus_manager_.RegisterInterest(UBUS_DASH_SIZE_CHANGED, [&] (GVariant *data) {
    int width, height;
    g_variant_get(data, "(ii)", &width, &height);
    stored_dash_width_ = width;
    QueueDraw();
  });

  bg_effect_helper_.owner = this;

  //FIXME (gord)- replace with async loading
  TextureCache& cache = TextureCache::GetDefault();
  panel_sheen_ = cache.FindTexture("dash_sheen.png");
  bg_refine_tex_ = cache.FindTexture("refine_gradient_panel.png");
  bg_refine_single_column_tex_ = cache.FindTexture("refine_gradient_panel_single_column.png");

  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  bg_refine_layer_.reset(new nux::TextureLayer(bg_refine_tex_->GetDeviceTexture(),
                         nux::TexCoordXForm(), nux::color::White, false, rop));

  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  bg_refine_single_column_layer_.reset(new nux::TextureLayer(bg_refine_single_column_tex_->GetDeviceTexture(),
                                       nux::TexCoordXForm(), nux::color::White, false, rop));

  auto update_blur_geometry = [this](nux::Area*, nux::Geometry const& g) {
    nux::Geometry const& geo = GetGeometry();
    nux::Geometry const& geo_absolute = parent_->GetAbsoluteGeometry();
    nux::Geometry blur_geo (geo_absolute.x, geo_absolute.y, geo.width, geo.height);
    bg_effect_helper_.SetBackbufferRegion(blur_geo);
  };

  parent_->geometry_changed.connect(update_blur_geometry);
  update_blur_geometry(this, parent_->GetGeometry());
}

PanelView::~PanelView()
{
  indicator::EntryLocationMap locations;
  remote_->SyncGeometries(GetName() + std::to_string(monitor_), locations);
}

Window PanelView::GetTrayXid() const
{
  if (!tray_)
    return 0;

  return tray_->xid();
}

void PanelView::SetLauncherWidth(int width)
{
  launcher_width_ = width;
  QueueDraw();
}

bool PanelView::IsMouseInsideIndicator(nux::Point const& mouse_position) const
{
  return indicators_->GetAbsoluteGeometry().IsInside(mouse_position);
}

void PanelView::OnBackgroundUpdate(nux::Color const&)
{
  if (overlay_is_open_)
    ForceUpdateBackground();
}

void PanelView::OnOverlayHidden(GVariant* data)
{
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  int width, height;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

  if (monitor_ == overlay_monitor && overlay_identity.Str() == active_overlay_)
  {
    if (opacity_ >= 1.0f)
      bg_effect_helper_.enabled = false;

    overlay_is_open_ = false;
    active_overlay_ = "";
    menu_view_->OverlayHidden();
    indicators_->OverlayHidden();
    SetAcceptKeyNavFocusOnMouseDown(true);
    ForceUpdateBackground();
  }
}

void PanelView::OnOverlayShown(GVariant* data)
{
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  int width, height;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

  if (monitor_ == overlay_monitor)
  {
    stored_dash_width_ = width;
    bg_effect_helper_.enabled = true;
    active_overlay_ = overlay_identity.Str();
    overlay_is_open_ = true;
    indicators_->OverlayShown();
    menu_view_->OverlayShown();
    SetAcceptKeyNavFocusOnMouseDown(false);
    ForceUpdateBackground();
  }
}

void PanelView::AddPanelView(PanelIndicatorsView* child,
                             unsigned int stretchFactor)
{
  layout_->AddView(child, stretchFactor, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  auto conn = child->on_indicator_updated.connect(sigc::mem_fun(this, &PanelView::OnIndicatorViewUpdated));
  on_indicator_updated_connections_.Add(conn);
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
  .add("monitor", monitor_)
  .add("active", IsActive())
  .add(GetAbsoluteGeometry());
}

void
PanelView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  UpdateBackground();

  GfxContext.PushClippingRectangle(geo);

  if (IsTransparent())
  {
    nux::Geometry const& geo_absolute = GetAbsoluteGeometry();

    if (BackgroundEffectHelper::blur_type != BLUR_NONE)
    {
      bg_blur_texture_ = bg_effect_helper_.GetBlurRegion();
    }
    else
    {
      bg_blur_texture_ = bg_effect_helper_.GetRegion();
    }

    if (bg_blur_texture_.IsValid())
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
                                          bg_blur_texture_,
                                          texxform_blur_bg,
                                          nux::color::White,
                                          WindowManager::Default().average_color(),
                                          nux::LAYER_BLEND_MODE_OVERLAY,
                                          true, rop);
      else
        gPainter.PushDrawTextureLayer(GfxContext, geo,
                                      bg_blur_texture_,
                                      texxform_blur_bg,
                                      nux::color::White,
                                      true,
                                      rop);
#else
        gPainter.PushDrawCompositionLayer(GfxContext, geo,
                                          bg_blur_texture_,
                                          texxform_blur_bg,
                                          nux::color::White,
                                          WindowManager::Default().average_color(),
                                          nux::LAYER_BLEND_MODE_OVERLAY,
                                          true, rop);
#endif

      GfxContext.PopClippingRectangle();
    }

    if (overlay_is_open_ && !Settings::Instance().GetLowGfxMode())
    {
      nux::GetPainter().RenderSinglePaintLayer(GfxContext, geo, bg_darken_layer_.get());

      GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      nux::TexCoordXForm refine_texxform;

      int refine_x_pos = geo.x + (stored_dash_width_ - refine_gradient_midpoint);

      refine_x_pos += launcher_width_;
      GfxContext.QRP_1Tex(refine_x_pos, geo.y,
                          bg_refine_tex_->GetWidth(),
                          bg_refine_tex_->GetHeight(),
                          bg_refine_tex_->GetDeviceTexture(),
                          refine_texxform, nux::color::White);

      GfxContext.QRP_1Tex(refine_x_pos + bg_refine_tex_->GetWidth(),
                          geo.y, geo.width, geo.height,
                          bg_refine_single_column_tex_->GetDeviceTexture(),
                          refine_texxform, nux::color::White);
    }
  }

  if (!overlay_is_open_ || !GfxContext.UsingGLSLCodePath())
    nux::GetPainter().RenderSinglePaintLayer(GfxContext, geo, bg_layer_.get());

  GfxContext.PopClippingRectangle();

  if (needs_geo_sync_)
  {
    SyncGeometries();
    needs_geo_sync_ = false;
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

  if (bg_blur_texture_.IsValid() && IsTransparent())
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
                                    bg_blur_texture_,
                                    texxform_blur_bg,
                                    nux::color::White,
                                    WindowManager::Default().average_color(),
                                    nux::LAYER_BLEND_MODE_OVERLAY,
                                    true,
                                    rop);
    else
      gPainter.PushTextureLayer(GfxContext, geo,
                                bg_blur_texture_,
                                texxform_blur_bg,
                                nux::color::White,
                                true,
                                rop);

#else
      gPainter.PushCompositionLayer(GfxContext, geo,
                                    bg_blur_texture_,
                                    texxform_blur_bg,
                                    nux::color::White,
                                    WindowManager::Default().average_color(),
                                    nux::LAYER_BLEND_MODE_OVERLAY,
                                    true,
                                    rop);
#endif
    bgs++;

    if (overlay_is_open_)
    {
      if (Settings::Instance().GetLowGfxMode())
      {
        rop.Blend = false;
        auto const& bg_color = WindowManager::Default().average_color();
        bg_darken_layer_.reset(new nux::ColorLayer(bg_color, false, rop));
      }

      nux::GetPainter().PushLayer(GfxContext, geo, bg_darken_layer_.get());
      bgs++;

      nux::Geometry refine_geo = geo;

      int refine_x_pos = geo.x + (stored_dash_width_ - refine_gradient_midpoint);
      refine_x_pos += launcher_width_;

      refine_geo.x = refine_x_pos;
      refine_geo.width = bg_refine_tex_->GetWidth();
      refine_geo.height = bg_refine_tex_->GetHeight();

      if (Settings::Instance().GetLowGfxMode() == false)
      {
        nux::GetPainter().PushLayer(GfxContext, refine_geo, bg_refine_layer_.get());
        bgs++;

        refine_geo.x += refine_geo.width;
        refine_geo.width = geo.width;
        refine_geo.height = geo.height;
        nux::GetPainter().PushLayer(GfxContext, refine_geo, bg_refine_single_column_layer_.get());
        bgs++;
      }
    }
  }

  if (!overlay_is_open_ || GfxContext.UsingGLSLCodePath() == false)
    gPainter.PushLayer(GfxContext, geo, bg_layer_.get());

  if (overlay_is_open_ && Settings::Instance().GetLowGfxMode() == false)
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
                                       panel_sheen_->GetDeviceTexture(),
                                       texxform,
                                       nux::color::White,
                                       false,
                                       rop);
  }
  layout_->ProcessDraw(GfxContext, force_draw);

  gPainter.PopBackground(bgs);

  GfxContext.GetRenderStates().SetBlend(false);
  GfxContext.PopClippingRectangle();
}

void
PanelView::UpdateBackground()
{
  if (!is_dirty_)
    return;

  is_dirty_ = false;

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  if (overlay_is_open_)
  {
    auto const& bg_color = WindowManager::Default().average_color();
    bg_layer_.reset(new nux::ColorLayer(bg_color, true, rop));
  }
  else
  {
    double opacity = opacity_;

    if (opacity_maximized_toggle_)
    {
      WindowManager& wm = WindowManager::Default();
      Window maximized_win = menu_view_->GetMaximizedWindow();

      if (wm.IsExpoActive() || (maximized_win != 0 && !wm.IsWindowObscured(maximized_win)))
        opacity = 1.0f;
    }

    auto const& tex = panel::Style::Instance().GetBackground();
    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_CLAMP);

    bg_layer_.reset(new nux::TextureLayer(tex->GetDeviceTexture(), texxform,
                                          nux::color::White * opacity, true, rop));
  }
}

void PanelView::ForceUpdateBackground()
{
  is_dirty_ = true;
  UpdateBackground();

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
    menu_view_->AddIndicator(proxy);
  }
  else
  {
    indicators_->AddIndicator(proxy);
  }

  ComputeContentSize();
  NeedRedraw();
}

void PanelView::OnObjectRemoved(indicator::Indicator::Ptr const& proxy)
{
  if (proxy->IsAppmenu())
  {
    menu_view_->RemoveIndicator(proxy);
  }
  else
  {
    indicators_->RemoveIndicator(proxy);
  }

  ComputeContentSize();
  NeedRedraw();
}

void PanelView::OnIndicatorViewUpdated(PanelIndicatorEntryView* view)
{
  needs_geo_sync_ = true;
  ComputeContentSize();
}

void PanelView::OnMenuPointerMoved(int x, int y)
{
  nux::Geometry const& geo = GetAbsoluteGeometry();

  if (geo.IsPointInside(x, y))
  {
    PanelIndicatorEntryView* view = nullptr;

    if (menu_view_->GetControlsActive())
      view = menu_view_->ActivateEntryAt(x, y);

    if (!view) indicators_->ActivateEntryAt(x, y);

    menu_view_->SetMousePosition(x, y);
  }
  else
  {
    menu_view_->SetMousePosition(-1, -1);
  }
}

void PanelView::OnEntryActivateRequest(std::string const& entry_id)
{
  if (!IsActive())
    return;

  bool ret;

  ret = menu_view_->ActivateEntry(entry_id, 0);
  if (!ret) indicators_->ActivateEntry(entry_id, 0);
}

bool PanelView::TrackMenuPointer()
{
  nux::Point const& mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  if (tracked_pointer_pos_ != mouse)
  {
    OnMenuPointerMoved(mouse.x, mouse.y);
    tracked_pointer_pos_ = mouse;
  }

  return true;
}

void PanelView::OnEntryActivated(std::string const& entry_id, nux::Rect const& geo)
{
  bool active = (entry_id.size() > 0);
  if (active && !track_menu_pointer_timeout_)
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
    track_menu_pointer_timeout_.reset(new glib::Timeout(16));
    track_menu_pointer_timeout_->Run(sigc::mem_fun(this, &PanelView::TrackMenuPointer));
  }
  else if (!active)
  {
    track_menu_pointer_timeout_.reset();
    menu_view_->NotifyAllMenusClosed();
    tracked_pointer_pos_ = {-1, -1};
  }

  if (overlay_is_open_)
    ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
}

void PanelView::OnEntryShowMenu(std::string const& entry_id, unsigned xid,
                                int x, int y, unsigned button)
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

  ret = menu_view_->ActivateIfSensitive();
  if (!ret) indicators_->ActivateIfSensitive();

  return ret;
}

void PanelView::SetOpacity(float opacity)
{
  if (opacity_ == opacity)
    return;

  opacity_ = (opacity <= 0.0f ? 0.0001f : opacity); // Not to get a black menu area
  bg_effect_helper_.enabled = IsTransparent();

  ForceUpdateBackground();
}

bool PanelView::IsTransparent()
{
  return (opacity_ < 1.0f || overlay_is_open_);
}

void PanelView::SetMenuShowTimings(int fadein, int fadeout, int discovery,
                                   int discovery_fadein, int discovery_fadeout)
{
  menu_view_->SetMenuShowTimings(fadein, fadeout, discovery, discovery_fadein, discovery_fadeout);
}

void PanelView::SetOpacityMaximizedToggle(bool enabled)
{
  if (opacity_maximized_toggle_ != enabled)
  {
    if (enabled)
    {
      WindowManager& win_manager = WindowManager::Default();
      auto update_bg_lambda = [&](guint32) { ForceUpdateBackground(); };
      auto conn = &maximized_opacity_toggle_connections_;

      conn->Add(win_manager.window_minimized.connect(update_bg_lambda));
      conn->Add(win_manager.window_unminimized.connect(update_bg_lambda));
      conn->Add(win_manager.window_maximized.connect(update_bg_lambda));
      conn->Add(win_manager.window_restored.connect(update_bg_lambda));
      conn->Add(win_manager.window_mapped.connect(update_bg_lambda));
      conn->Add(win_manager.window_unmapped.connect(update_bg_lambda));
      conn->Add(win_manager.initiate_expo.connect(
        sigc::mem_fun(this, &PanelView::ForceUpdateBackground)));
      conn->Add(win_manager.terminate_expo.connect(
        sigc::mem_fun(this, &PanelView::ForceUpdateBackground)));
      conn->Add(win_manager.screen_viewport_switch_ended.connect(
        sigc::mem_fun(this, &PanelView::ForceUpdateBackground)));
    }
    else
    {
      maximized_opacity_toggle_connections_.Clear();
    }

    opacity_maximized_toggle_ = enabled;
    ForceUpdateBackground();
  }
}

void PanelView::SyncGeometries()
{
  indicator::EntryLocationMap locations;
  std::string panel_id = GetName() + std::to_string(monitor_);

  if (menu_view_->GetControlsActive())
    menu_view_->GetGeometryForSync(locations);

  indicators_->GetGeometryForSync(locations);
  remote_->SyncGeometries(panel_id, locations);
}

void PanelView::SetMonitor(int monitor)
{
  monitor_ = monitor;
  menu_view_->SetMonitor(monitor);

  UScreen* uscreen = UScreen::GetDefault();
  auto monitor_geo = uscreen->GetMonitorGeometry(monitor);
  Resize(nux::Point(monitor_geo.x, monitor_geo.y), monitor_geo.width);
}

void PanelView::Resize(nux::Point const& offset, int width)
{
  unity::panel::Style &panel_style = panel::Style::Instance();
  SetMaximumWidth(width);
  SetGeometry(nux::Geometry(0, 0, width, panel_style.panel_height));
  parent_->SetGeometry(nux::Geometry(offset.x, offset.y, width, panel_style.panel_height));
}

int PanelView::GetMonitor() const
{
  return monitor_;
}

bool PanelView::IsActive() const
{
  return menu_view_->GetControlsActive();
}

int PanelView::GetStoredDashWidth() const
{
  return stored_dash_width_;
}

ui::EdgeBarrierSubscriber::Result PanelView::HandleBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event)
{
  if (WindowManager::Default().IsAnyWindowMoving())
    return ui::EdgeBarrierSubscriber::Result::IGNORED;

  return ui::EdgeBarrierSubscriber::Result::NEEDS_RELEASE;
}

} // namespace unity
