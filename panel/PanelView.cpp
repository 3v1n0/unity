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
#include <Nux/HLayout.h>

#include <UnityCore/GLibWrapper.h>

#include "unity-shared/PanelStyle.h"
#include "unity-shared/RawPixel.h"
#include "unity-shared/TextureCache.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UScreen.h"

#include "PanelIndicatorsView.h"

#include "PanelView.h"

namespace unity
{
namespace panel
{
namespace
{
const RawPixel TRIANGLE_THRESHOLD = 5_em;
const int refine_gradient_midpoint = 959;
}


NUX_IMPLEMENT_OBJECT_TYPE(PanelView);

PanelView::PanelView(MockableBaseWindow* parent, menu::Manager::Ptr const& menus, NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , parent_(parent)
  , remote_(menus->Indicators())
  , is_dirty_(true)
  , opacity_maximized_toggle_(false)
  , needs_geo_sync_(false)
  , overlay_is_open_(false)
  , opacity_(1.0f)
  , monitor_(0)
  , stored_dash_width_(0)
  , bg_effect_helper_(this)
{
  auto& wm = WindowManager::Default();
  panel::Style::Instance().changed.connect(sigc::mem_fun(this, &PanelView::ForceUpdateBackground));
  Settings::Instance().dpi_changed.connect(sigc::mem_fun(this, &PanelView::Resize));
  Settings::Instance().low_gfx.changed.connect(sigc::hide(sigc::mem_fun(this, &PanelView::OnLowGfxChanged)));

  wm.average_color.changed.connect(sigc::mem_fun(this, &PanelView::OnBackgroundUpdate));
  wm.initiate_spread.connect(sigc::mem_fun(this, &PanelView::OnSpreadInitiate));
  wm.terminate_spread.connect(sigc::mem_fun(this, &PanelView::OnSpreadTerminate));

  bg_layer_.reset(new nux::ColorLayer(nux::Color(0xff595853), true));

  nux::ROPConfig rop;
  rop.Blend = true;
  nux::Color darken_colour = nux::Color(0.9f, 0.9f, 0.9f, 1.0f);

  if (!Settings::Instance().low_gfx())
  {
    rop.SrcBlend = GL_ZERO;
    rop.DstBlend = GL_SRC_COLOR;
  }
  //If we are in low gfx mode then change our darken_colour to our background colour.
  else
  {
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    darken_colour = wm.average_color();
  }

  bg_darken_layer_.reset(new nux::ColorLayer(darken_colour, false, rop));

  layout_ = new nux::HLayout("", NUX_TRACKER_LOCATION);
  layout_->SetContentDistribution(nux::MAJOR_POSITION_START);

  menu_view_ = new PanelMenuView(menus);
  menu_view_->EnableDropdownMenu(true, remote_);
  AddPanelView(menu_view_, 0);

  SetCompositionLayout(layout_);

  tray_ = new PanelTray(monitor_);
  layout_->AddView(tray_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  AddChild(tray_);

  indicators_ = new PanelIndicatorsView();
  indicators_->SetMonitor(monitor_);
  AddPanelView(indicators_, 0);

  for (auto const& object : remote_->GetIndicators())
    OnObjectAdded(object);

  remote_->on_object_added.connect(sigc::mem_fun(this, &PanelView::OnObjectAdded));
  remote_->on_object_removed.connect(sigc::mem_fun(this, &PanelView::OnObjectRemoved));
  remote_->on_entry_activated.connect(sigc::mem_fun(this, &PanelView::OnEntryActivated));
  remote_->on_entry_show_menu.connect(sigc::mem_fun(this, &PanelView::OnEntryShowMenu));
  menus->key_activate_entry.connect(sigc::mem_fun(this, &PanelView::ActivateEntry));
  menus->open_first.connect(sigc::mem_fun(this, &PanelView::ActivateFirstSensitive));

  ubus_manager_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &PanelView::OnOverlayHidden));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &PanelView::OnOverlayShown));
  ubus_manager_.RegisterInterest(UBUS_DASH_SIZE_CHANGED, [this] (GVariant *data) {
    int width, height;
    g_variant_get(data, "(ii)", &width, &height);
    stored_dash_width_ = width;
    QueueDraw();
  });

  LoadTextures();
  TextureCache::GetDefault().themed_invalidated.connect(sigc::mem_fun(this, &PanelView::LoadTextures));
}

PanelView::~PanelView()
{
  indicator::EntryLocationMap locations;
  remote_->SyncGeometries(GetName() + std::to_string(monitor_), locations);
}

void PanelView::LoadTextures()
{
  //FIXME (gord)- replace with async loading
  TextureCache& cache = TextureCache::GetDefault();
  panel_sheen_ = cache.FindTexture("dash_sheen");
  bg_refine_tex_ = cache.FindTexture("refine_gradient_panel");
  bg_refine_single_column_tex_ = cache.FindTexture("refine_gradient_panel_single_column");

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  bg_refine_layer_.reset(new nux::TextureLayer(bg_refine_tex_->GetDeviceTexture(),
                         nux::TexCoordXForm(), nux::color::White, false, rop));

  bg_refine_single_column_layer_.reset(new nux::TextureLayer(bg_refine_single_column_tex_->GetDeviceTexture(),
                                       nux::TexCoordXForm(), nux::color::White, false, rop));
}

Window PanelView::GetTrayXid() const
{
  if (!tray_)
    return 0;

  return tray_->xid();
}

bool PanelView::IsMouseInsideIndicator(nux::Point const& mouse_position) const
{
  return indicators_->GetAbsoluteGeometry().IsInside(mouse_position);
}

void PanelView::OnBackgroundUpdate(nux::Color const&)
{
  if (InOverlayMode())
    ForceUpdateBackground();
}

bool PanelView::InOverlayMode() const
{
  return overlay_is_open_ || WindowManager::Default().IsScaleActive();
}

void PanelView::EnableOverlayMode(bool overlay_mode)
{
  if (overlay_mode)
  {
    bg_effect_helper_.enabled = true;
    indicators_->OverlayShown();
    menu_view_->OverlayShown();
    SetAcceptKeyNavFocusOnMouseDown(false);
  }
  else
  {
    if (opacity_ >= 1.0f)
      bg_effect_helper_.enabled = false;

    menu_view_->OverlayHidden();
    indicators_->OverlayHidden();
    SetAcceptKeyNavFocusOnMouseDown(true);
  }

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
    overlay_is_open_ = false;
    active_overlay_ = "";

    if (!WindowManager::Default().IsScaleActive())
      EnableOverlayMode(false);
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
    overlay_is_open_ = true;
    active_overlay_ = overlay_identity.Str();
    stored_dash_width_ = width;
    EnableOverlayMode(true);
  }
}

void PanelView::OnSpreadInitiate()
{
  if (overlay_is_open_)
    return;

  EnableOverlayMode(true);
}

void PanelView::OnSpreadTerminate()
{
  if (overlay_is_open_)
    return;

  EnableOverlayMode(false);
}

void PanelView::OnLowGfxChanged()
{
  if (!Settings::Instance().low_gfx())
  {
    nux::ROPConfig rop;

    rop.Blend = true;
    rop.SrcBlend = GL_ZERO;
    rop.DstBlend = GL_SRC_COLOR;
    nux::Color darken_colour = nux::Color(0.9f, 0.9f, 0.9f, 1.0f);
    bg_darken_layer_.reset(new nux::ColorLayer(darken_colour, false, rop));
  }

  ForceUpdateBackground();
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

void PanelView::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
  .add("backend", "remote")
  .add("monitor", monitor_)
  .add("active", IsActive())
  .add("in_overlay_mode", InOverlayMode())
  .add(GetAbsoluteGeometry());
}

void
PanelView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  nux::Geometry const& geo_absolute = GetAbsoluteGeometry();
  nux::Geometry const& mgeo = UScreen::GetDefault()->GetMonitorGeometry(monitor_);
  nux::Geometry isect = mgeo.Intersect(geo_absolute);

  if(!isect.width || !isect.height)
      return;

  UpdateBackground();

  bool overlay_mode = InOverlayMode();
  GfxContext.PushClippingRectangle(isect);

  if (IsTransparent())
  {
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

      GfxContext.PushClippingRectangle(isect);

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

    if (overlay_mode && !Settings::Instance().low_gfx())
    {
      nux::GetPainter().RenderSinglePaintLayer(GfxContext, geo, bg_darken_layer_.get());

      GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      nux::TexCoordXForm refine_texxform;

      int refine_x_pos = geo.x + (stored_dash_width_ - refine_gradient_midpoint);

      if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
        refine_x_pos += unity::Settings::Instance().LauncherSize(monitor_);
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

  if (!overlay_mode || !GfxContext.UsingGLSLCodePath())
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
  bool overlay_mode = InOverlayMode();
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

    if (overlay_mode)
    {
      if (Settings::Instance().low_gfx())
      {
        rop.Blend = false;
        auto const& bg_color = WindowManager::Default().average_color();
        bg_darken_layer_.reset(new nux::ColorLayer(bg_color, false, rop));
      }

      nux::GetPainter().PushLayer(GfxContext, geo, bg_darken_layer_.get());
      bgs++;

      nux::Geometry refine_geo = geo;

      int refine_x_pos = geo.x + (stored_dash_width_ - refine_gradient_midpoint);
      if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
        refine_x_pos += unity::Settings::Instance().LauncherSize(monitor_);

      refine_geo.x = refine_x_pos;
      refine_geo.width = bg_refine_tex_->GetWidth();

      if (!Settings::Instance().low_gfx())
      {
        nux::GetPainter().PushLayer(GfxContext, refine_geo, bg_refine_layer_.get());
        bgs++;

        refine_geo.x += refine_geo.width;
        refine_geo.width = geo.width;
        nux::GetPainter().PushLayer(GfxContext, refine_geo, bg_refine_single_column_layer_.get());
        bgs++;
      }
    }
  }

  if (!overlay_mode || !GfxContext.UsingGLSLCodePath())
    gPainter.PushLayer(GfxContext, geo, bg_layer_.get());

  if (overlay_mode && !Settings::Instance().low_gfx())
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

  WindowManager& wm = WindowManager::Default();
  is_dirty_ = false;

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  if (overlay_is_open_ || wm.IsScaleActive())
  {
    bg_layer_.reset(new nux::ColorLayer(wm.average_color(), true, rop));
  }
  else
  {
    double opacity = opacity_;

    if (opacity_maximized_toggle_)
    {
      Window maximized_win = menu_view_->maximized_window();

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

  QueueRelayout();
  QueueDraw();
}

void PanelView::PreLayoutManagement()
{
  View::PreLayoutManagement();

  int tray_width = 0;
  if (tray_)
    tray_width = tray_->GetBaseWidth();

  int menu_width = GetMaximumWidth() - indicators_->GetBaseWidth() - tray_width;

  menu_view_->SetMinimumWidth(menu_width);
  menu_view_->SetMaximumWidth(menu_width);
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

  QueueRelayout();
  QueueDraw();
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

  QueueRelayout();
  QueueDraw();
}

void PanelView::OnIndicatorViewUpdated()
{
  needs_geo_sync_ = true;
  QueueRelayout();
  QueueDraw();
}

void PanelView::OnMenuPointerMoved(int x, int y)
{
  nux::Geometry const& geo = GetAbsoluteGeometry();

  if (geo.IsPointInside(x, y))
  {
    PanelIndicatorEntryView* view = nullptr;

    if (menu_view_->HasMenus())
      view = menu_view_->ActivateEntryAt(x, y);

    if (!view) indicators_->ActivateEntryAt(x, y);

    menu_view_->SetMousePosition(x, y);
  }
  else
  {
    menu_view_->SetMousePosition(-1, -1);
  }
}

static bool PointInTriangle(nux::Point const& p, nux::Point const& t0, nux::Point const& t1, nux::Point const& t2)
{
  int s = t0.y * t2.x - t0.x * t2.y + (t2.y - t0.y) * p.x + (t0.x - t2.x) * p.y;
  int t = t0.x * t1.y - t0.y * t1.x + (t0.y - t1.y) * p.x + (t1.x - t0.x) * p.y;

  if ((s < 0) != (t < 0))
    return false;

  int A = -t1.y * t2.x + t0.y * (t2.x - t1.x) + t0.x * (t1.y - t2.y) + t1.x * t2.y;
  if (A < 0)
  {
    s = -s;
    t = -t;
    A = -A;
  }

  return s > 0 && t > 0 && (s + t) < A;
}

static double GetMouseVelocity(nux::Point const& p0, nux::Point const& p1, util::Timer &timer)
{
  int dx, dy;
  double speed;
  auto millis = timer.ElapsedMicroSeconds();

  if (millis == 0)
    return 1;

  dx = p0.x - p1.x;
  dy = p0.y - p1.y;

  speed = sqrt(dx * dx + dy * dy) / millis * 1000;

  return speed;
}

bool PanelView::TrackMenuPointer()
{
  nux::Point const& mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  double speed = GetMouseVelocity(mouse, tracked_pointer_pos_, mouse_tracker_timer_);

  mouse_tracker_timer_.Reset();
  tracked_pointer_pos_ = mouse;

  double scale = Settings::Instance().em(monitor_)->DPIScale();
  if (speed > 0 && PointInTriangle(mouse,
                                   nux::Point(triangle_top_corner_.x, std::max(triangle_top_corner_.y - TRIANGLE_THRESHOLD.CP(scale), 0)),
                                   nux::Point(menu_geo_.x, menu_geo_.y),
                                   nux::Point(menu_geo_.x + menu_geo_.width, menu_geo_.y)))
  {
    return true;
  }

  if (mouse != triangle_top_corner_)
  {
    triangle_top_corner_ = mouse;
    OnMenuPointerMoved(mouse.x, mouse.y);
  }

  return true;
}

void PanelView::OnEntryActivated(std::string const& panel, std::string const& entry_id, nux::Rect const& menu_geo)
{
  if (!panel.empty() && panel != GetPanelName())
    return;

  menu_geo_ = menu_geo;

  bool active = !entry_id.empty();
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
    mouse_tracker_timer_.Reset();
    triangle_top_corner_ = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
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
  if (!track_menu_pointer_timeout_)
  {
    // This is ugly... But Nux fault!
    menu_view_->IgnoreLeaveEvents(true);
    WindowManager::Default().UnGrabMousePointer(CurrentTime, button, x, y);
    menu_view_->IgnoreLeaveEvents(false);
  }
}

bool PanelView::ActivateFirstSensitive()
{
  if (!IsActive())
    return false;

  if ((menu_view_->HasKeyActivableMenus() && menu_view_->ActivateIfSensitive()) ||
      indicators_->ActivateIfSensitive())
  {
    // Since this only happens on keyboard events, we need to prevent that the
    // pointer tracker would select another entry.
    tracked_pointer_pos_ = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
    return true;
  }

  return false;
}

bool PanelView::ActivateEntry(std::string const& entry_id)
{
  if (!IsActive())
    return false;

  if ((menu_view_->HasKeyActivableMenus() && menu_view_->ActivateEntry(entry_id, 0)) ||
      indicators_->ActivateEntry(entry_id, 0))
  {
    // Since this only happens on keyboard events, we need to prevent that the
    // pointer tracker would select another entry.
    tracked_pointer_pos_ = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
    return true;
  }

  return false;
}

//
// Useful Public Methods
//

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
  return (opacity_ < 1.0f || InOverlayMode());
}

void PanelView::SetOpacityMaximizedToggle(bool enabled)
{
  if (opacity_maximized_toggle_ != enabled)
  {
    if (enabled)
    {
      WindowManager& win_manager = WindowManager::Default();
      auto update_bg_lambda = [this](guint32) { ForceUpdateBackground(); };
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

std::string PanelView::GetPanelName() const
{
  return GetName() + std::to_string(monitor_);
}

void PanelView::SyncGeometries()
{
  indicator::EntryLocationMap locations;

   if (menu_view_->HasMenus())
    menu_view_->GetGeometryForSync(locations);

  indicators_->GetGeometryForSync(locations);
  remote_->SyncGeometries(GetPanelName(), locations);
}

void PanelView::SetMonitor(int monitor)
{
  monitor_ = monitor;
  menu_view_->SetMonitor(monitor);
  indicators_->SetMonitor(monitor);
  Resize();

  if (WindowManager::Default().IsScaleActive())
    EnableOverlayMode(true);
}

void PanelView::Resize()
{
  int height = Style::Instance().PanelHeight(monitor_);
  auto const& monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor_);

  SetMinMaxSize(monitor_geo.width, height);
  parent_->SetGeometry({monitor_geo.x, monitor_geo.y, monitor_geo.width, height});

  for (auto* child : layout_->GetChildren())
  {
    child->SetMinimumHeight(height);
    child->SetMaximumHeight(height);
  }

  QueueRelayout();
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

ui::EdgeBarrierSubscriber::Result PanelView::HandleBarrierEvent(ui::PointerBarrierWrapper::Ptr const& owner, ui::BarrierEvent::Ptr event)
{
  if (WindowManager::Default().IsAnyWindowMoving())
    return ui::EdgeBarrierSubscriber::Result::IGNORED;

  return ui::EdgeBarrierSubscriber::Result::NEEDS_RELEASE;
}

} // namespace panel
} // namespace unity
