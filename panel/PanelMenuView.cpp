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
 *              Marco Trevisan <3v1n0@ubuntu.com>
 */

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

#include "PanelMenuView.h"
#include "unity-shared/AnimationUtils.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/DecorationStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

#include "config.h"
#include <glib/gi18n-lib.h>

namespace unity
{
DECLARE_LOGGER(logger, "unity.panel.menu");

namespace
{
  const int MAIN_LEFT_PADDING = 4;
  const int TITLE_PADDING = 2;
  const int MENUBAR_PADDING = 4;
  const int MENU_ENTRIES_PADDING = 6;
  const int DEFAULT_MENUS_FADEIN = 100;
  const int DEFAULT_MENUS_FADEOUT = 120;
  const int DEFAULT_MENUS_DISCOVERY = 2;
  const int DEFAULT_DISCOVERY_FADEIN = 200;
  const int DEFAULT_DISCOVERY_FADEOUT = 300;

  const std::string NEW_APP_HIDE_TIMEOUT = "new-app-hide-timeout";
  const std::string NEW_APP_SHOW_TIMEOUT = "new-app-show-timeout";
  const std::string WINDOW_MOVED_TIMEOUT = "window-moved-timeout";
  const std::string UPDATE_SHOW_NOW_TIMEOUT = "update-show-now-timeout";
}

PanelMenuView::PanelMenuView()
  : matcher_(bamf_matcher_get_default())
  , is_inside_(false)
  , is_grabbed_(false)
  , is_maximized_(false)
  , last_active_view_(nullptr)
  , new_application_(nullptr)
  , overlay_showing_(false)
  , switcher_showing_(false)
  , launcher_keynav_(false)
  , show_now_activated_(false)
  , we_control_active_(false)
  , new_app_menu_shown_(false)
  , monitor_(0)
  , active_xid_(0)
  , desktop_name_(_("Ubuntu Desktop"))
  , menus_fadein_(DEFAULT_MENUS_FADEIN)
  , menus_fadeout_(DEFAULT_MENUS_FADEOUT)
  , menus_discovery_(DEFAULT_MENUS_DISCOVERY)
  , menus_discovery_fadein_(DEFAULT_DISCOVERY_FADEIN)
  , menus_discovery_fadeout_(DEFAULT_DISCOVERY_FADEOUT)
{

  BamfWindow* active_win = bamf_matcher_get_active_window(matcher_);
  if (BAMF_IS_WINDOW(active_win))
    active_xid_ = bamf_window_get_xid(active_win);

  SetupPanelMenuViewSignals();
  SetupWindowButtons();
  SetupTitlebarGrabArea();
  SetupWindowManagerSignals();
  SetupUBusManagerInterests();

  style_changed_connection_ = panel::Style::Instance().changed.connect([this] {
    window_buttons_->ComputeContentSize();
    layout_->SetLeftAndRightPadding(window_buttons_->GetContentWidth(), 0);

    Refresh(true);
    FullRedraw();
  });

  opacity = 0.0f;

  if (Refresh())
    FullRedraw();
}

PanelMenuView::~PanelMenuView()
{
  window_buttons_->UnParentObject();
  titlebar_grab_area_->UnParentObject();
}

void PanelMenuView::SetupPanelMenuViewSignals()
{
  active_win_changed_signal_.Connect(matcher_, "active-window-changed",
                                     sigc::mem_fun(this, &PanelMenuView::OnActiveWindowChanged));
  active_app_changed_signal_.Connect(matcher_, "active-application-changed",
                                     sigc::mem_fun(this, &PanelMenuView::OnActiveAppChanged));
  view_opened_signal_.Connect(matcher_, "view-opened",
                              sigc::mem_fun(this, &PanelMenuView::OnViewOpened));
  view_closed_signal_.Connect(matcher_, "view-closed",
                              sigc::mem_fun(this, &PanelMenuView::OnViewClosed));

  mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));

  opacity_animator_.updated.connect(sigc::mem_fun(this, &PanelMenuView::OnFadeAnimatorUpdated));
}


void PanelMenuView::SetupWindowButtons()
{
  window_buttons_ = new WindowButtons();
  window_buttons_->SetParentObject(this);
  window_buttons_->monitor = monitor_;
  window_buttons_->controlled_window = active_xid_;
  window_buttons_->opacity = 0.0f;
  window_buttons_->SetLeftAndRightPadding(MAIN_LEFT_PADDING, MENUBAR_PADDING);
  window_buttons_->SetMaximumHeight(panel::Style::Instance().panel_height);
  window_buttons_->ComputeContentSize();

  window_buttons_->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  window_buttons_->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));
  /* This is needed since when buttons are redrawn, the panel is not. See bug #1090439 */
  window_buttons_->opacity.changed.connect(sigc::hide(sigc::mem_fun(this, &PanelMenuView::QueueDraw)));
  AddChild(window_buttons_.GetPointer());

  SetupLayout();
}

void PanelMenuView::SetupLayout()
{
  layout_->SetContentDistribution(nux::MAJOR_POSITION_START);
  layout_->SetLeftAndRightPadding(window_buttons_->GetContentWidth(), 0);
  layout_->SetBaseHeight(panel::Style::Instance().panel_height);
}

void PanelMenuView::SetupTitlebarGrabArea()
{
  titlebar_grab_area_ = new PanelTitlebarGrabArea();
  titlebar_grab_area_->SetParentObject(this);
  titlebar_grab_area_->activate_request.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedActivate));
  titlebar_grab_area_->restore_request.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedRestore));
  titlebar_grab_area_->lower_request.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedLower));
  titlebar_grab_area_->grab_started.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabStart));
  titlebar_grab_area_->grab_move.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabMove));
  titlebar_grab_area_->grab_end.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabEnd));
  titlebar_grab_area_->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  titlebar_grab_area_->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));
  AddChild(titlebar_grab_area_.GetPointer());
}

void PanelMenuView::SetupWindowManagerSignals()
{
  WindowManager& wm = WindowManager::Default();
  wm.window_minimized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMinimized));
  wm.window_unminimized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUnminimized));
  wm.window_maximized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMaximized));
  wm.window_restored.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowRestored));
  wm.window_unmapped.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUnmapped));
  wm.window_mapped.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMapped));
  wm.window_moved.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMoved));
  wm.window_resized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMoved));
  wm.initiate_spread.connect(sigc::mem_fun(this, &PanelMenuView::OnSpreadInitiate));
  wm.terminate_spread.connect(sigc::mem_fun(this, &PanelMenuView::OnSpreadTerminate));
  wm.initiate_expo.connect(sigc::mem_fun(this, &PanelMenuView::OnExpoInitiate));
  wm.terminate_expo.connect(sigc::mem_fun(this, &PanelMenuView::OnExpoTerminate));
  wm.screen_viewport_switch_ended.connect(sigc::mem_fun(this, &PanelMenuView::OnExpoTerminate));
}

void PanelMenuView::SetupUBusManagerInterests()
{
  ubus_manager_.RegisterInterest(UBUS_SWITCHER_SHOWN, sigc::mem_fun(this, &PanelMenuView::OnSwitcherShown));
  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_NAV, sigc::mem_fun(this, &PanelMenuView::OnLauncherKeyNavStarted));
  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_NAV, sigc::mem_fun(this, &PanelMenuView::OnLauncherKeyNavEnded));
  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_SWITCHER, sigc::mem_fun(this, &PanelMenuView::OnLauncherKeyNavStarted));
  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_SWITCHER, sigc::mem_fun(this, &PanelMenuView::OnLauncherKeyNavEnded));
  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_SELECTION_CHANGED, sigc::mem_fun(this, &PanelMenuView::OnLauncherSelectionChanged));
}

void PanelMenuView::OverlayShown()
{
  overlay_showing_ = true;
  QueueDraw();
}

void PanelMenuView::OverlayHidden()
{
  overlay_showing_ = false;
  QueueDraw();
}

void PanelMenuView::AddIndicator(indicator::Indicator::Ptr const& indicator)
{
  if (!GetIndicators().empty())
  {
    LOG_ERROR(logger) << "PanelMenuView has already an indicator!";
    return;
  }

  PanelIndicatorsView::AddIndicator(indicator);
}

void PanelMenuView::SetMenuShowTimings(int fadein, int fadeout, int discovery,
                                       int discovery_fadein, int discovery_fadeout)
{
  if (fadein > -1)
    menus_fadein_ = fadein;

  if (fadeout > -1)
    menus_fadeout_ = fadeout;

  if (discovery > -1)
    menus_discovery_ = discovery;

  if (discovery_fadein > -1)
    menus_discovery_fadein_ = discovery_fadein;

  if (discovery_fadeout > -1)
    menus_discovery_fadeout_ = discovery_fadeout;
}

void PanelMenuView::FullRedraw()
{
  QueueDraw();
  window_buttons_->QueueDraw();
}

nux::Area* PanelMenuView::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  bool mouse_inside = TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type);

  if (!mouse_inside)
    return nullptr;

  Area* found_area = nullptr;

  if (!we_control_active_)
  {
    /* When the current panel is not active, it all behaves like a grab-area */
    if (GetAbsoluteGeometry().IsInside(mouse_position))
      return titlebar_grab_area_.GetPointer();
  }

  if (is_maximized_)
  {
    if (window_buttons_)
    {
      found_area = window_buttons_->FindAreaUnderMouse(mouse_position, event_type);
      NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
    }
  }

  if (titlebar_grab_area_)
  {
    found_area = titlebar_grab_area_->FindAreaUnderMouse(mouse_position, event_type);
    NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
  }

  return PanelIndicatorsView::FindAreaUnderMouse(mouse_position, event_type);
}

void PanelMenuView::PreLayoutManagement()
{
  PanelIndicatorsView::PreLayoutManagement();
  nux::Geometry const& geo = GetGeometry();

  window_buttons_->ComputeContentSize();
  int buttons_diff = geo.height - window_buttons_->GetContentHeight();
  window_buttons_->SetBaseY(buttons_diff > 0 ? std::ceil(buttons_diff/2.0f) : 0);

  layout_->ComputeContentSize();
  int layout_width = layout_->GetContentWidth();

  titlebar_grab_area_->SetBaseX(layout_width);
  titlebar_grab_area_->SetBaseHeight(geo.height);
  titlebar_grab_area_->SetMinimumWidth(geo.width - layout_width);
  titlebar_grab_area_->SetMaximumWidth(geo.width - layout_width);

  SetMaximumEntriesWidth(geo.width - window_buttons_->GetContentWidth());
}

void PanelMenuView::StartFadeIn(int duration)
{
  opacity_animator_.SetDuration(duration >= 0 ? duration : menus_fadein_);
  animation::StartOrReverse(opacity_animator_, animation::Direction::FORWARD);
}

void PanelMenuView::StartFadeOut(int duration)
{
  opacity_animator_.SetDuration(duration >= 0 ? duration : menus_fadeout_);
  animation::StartOrReverse(opacity_animator_, animation::Direction::BACKWARD);
}

void PanelMenuView::OnFadeAnimatorUpdated(double progress)
{
  if (animation::GetDirection(opacity_animator_) == animation::Direction::FORWARD) /* Fading in... */
  {
    if (ShouldDrawMenus() && opacity() != 1.0f)
      opacity = progress;

    if (ShouldDrawButtons() && window_buttons_->opacity() != 1.0f)
      window_buttons_->opacity = progress;
  }
  else if (animation::GetDirection(opacity_animator_) == animation::Direction::BACKWARD) /* Fading out... */
  {
    if (!ShouldDrawMenus() && opacity() != 0.0f)
      opacity = progress;

    if (!ShouldDrawButtons() && window_buttons_->opacity() != 0.0f)
      window_buttons_->opacity = progress;
  }
}

bool PanelMenuView::ShouldDrawMenus() const
{
  if (we_control_active_ && !switcher_showing_ && !launcher_keynav_ && !entries_.empty())
  {
    WindowManager& wm = WindowManager::Default();

    if (!wm.IsExpoActive() && !wm.IsScaleActive())
    {
      if (is_inside_ || last_active_view_ || show_now_activated_ || new_application_)
        return true;

      if (is_maximized_)
        return (window_buttons_->IsMouseOwner() || titlebar_grab_area_->IsMouseOwner());
    }
  }

  return false;
}

bool PanelMenuView::ShouldDrawButtons() const
{
  if (we_control_active_ && is_maximized_ && !launcher_keynav_ && !switcher_showing_)
  {
    WindowManager& wm = WindowManager::Default();

    if (!wm.IsExpoActive() && !wm.IsScaleActive())
    {
      if (is_inside_ || show_now_activated_ || new_application_)
        return true;

      if (window_buttons_->IsMouseOwner() || titlebar_grab_area_->IsMouseOwner())
        return true;
    }
  }

  return false;
}

bool PanelMenuView::ShouldDrawFadingTitle() const
{
  return (!ShouldDrawButtons() && we_control_active_ && HasVisibleMenus() &&
         (ShouldDrawMenus() || (opacity() > 0.0f && window_buttons_->opacity() == 0.0f)));
}

bool PanelMenuView::HasVisibleMenus() const
{
  for (auto const& entry : entries_)
    if (entry.second->IsVisible())
      return true;

  return false;
}

double PanelMenuView::GetTitleOpacity() const
{
  double title_opacity = 1.0f;
  bool has_menu = HasVisibleMenus();

  bool is_title_soild = we_control_active_ && (!has_menu || opacity() == 0.0) &&
                        window_buttons_->opacity() == 0.0;

  if (!is_title_soild)
  {
    if (has_menu)
      title_opacity -= std::max<double>(opacity(), window_buttons_->opacity());
    else
      title_opacity -= window_buttons_->opacity();

    if (!ShouldDrawButtons() && !ShouldDrawMenus())
    {
      // If we're fading-out the buttons/menus, let's fade-in quickly the title
      title_opacity = CLAMP(title_opacity + 0.1f, 0.0f, 1.0f);
    }
    else
    {
      // If we're fading-in the buttons/menus, let's fade-out quickly the title
      title_opacity = CLAMP(title_opacity - 0.2f, 0.0f, 1.0f);
    }
  }

  return title_opacity;
}

void PanelMenuView::UpdateLastGeometry(nux::Geometry const& geo)
{
  if (geo != last_geo_)
  {
    last_geo_ = geo;
    QueueRelayout();
    Refresh(true);
  }
}

void PanelMenuView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  if (overlay_showing_)
    return;

  nux::Geometry const& geo = GetGeometry();
  UpdateLastGeometry(geo);

  GfxContext.PushClippingRectangle(geo);

  /* "Clear" out the background */
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::ColorLayer layer(nux::color::Transparent, true, rop);
  nux::GetPainter().PushDrawLayer(GfxContext, geo, &layer);

  if (title_texture_)
  {
    guint blend_alpha = 0, blend_src = 0, blend_dest = 0;

    GfxContext.GetRenderStates().GetBlend(blend_alpha, blend_src, blend_dest);

    if (ShouldDrawFadingTitle())
    {
      UpdateTitleGradientTexture();

      GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      nux::TexCoordXForm texxform0;
      nux::TexCoordXForm texxform1;

      // Modulate the checkboard and the gradient texture
      GfxContext.QRP_2TexMod(geo.x, geo.y,
                             geo.width, geo.height,
                             gradient_texture_, texxform0,
                             nux::color::White,
                             title_texture_->GetDeviceTexture(),
                             texxform1,
                             nux::color::White);
    }
    else
    {
      double title_opacity = GetTitleOpacity();

      if (title_opacity > 0.0f)
      {
        nux::TexCoordXForm texxform;
        GfxContext.QRP_1Tex(geo.x, geo.y, geo.width, geo.height,
                            title_texture_->GetDeviceTexture(), texxform,
                            nux::color::White * title_opacity);
      }
    }

    GfxContext.GetRenderStates().SetBlend(blend_alpha, blend_src, blend_dest);
  }

  nux::GetPainter().PopBackground();

  GfxContext.PopClippingRectangle();
}

void PanelMenuView::UpdateTitleGradientTexture()
{
  float const factor = 4;
  float button_width = window_buttons_->GetContentWidth() / 4;

  nux::Geometry const& geo = GetGeometry();
  nux::SURFACE_LOCKED_RECT lockrect;
  bool build_gradient = false;
  bool locked = false;
  lockrect.pBits = 0;

  if (gradient_texture_.IsNull() || (gradient_texture_->GetWidth() != geo.width))
  {
    build_gradient = true;
  }
  else
  {
    if (gradient_texture_->LockRect(0, &lockrect, nullptr) != OGL_OK)
      build_gradient = true;
    else
      locked = true;

    if (!lockrect.pBits)
    {
      build_gradient = true;

      if (locked)
        gradient_texture_->UnlockRect(0);
    }
  }

  if (build_gradient)
  {
    nux::NTextureData texture_data(nux::BITFMT_R8G8B8A8, geo.width, 1, 1);

    gradient_texture_ = nux::GetGraphicsDisplay()->GetGpuDevice()->
                        CreateSystemCapableDeviceTexture(texture_data.GetWidth(),
                        texture_data.GetHeight(), 1, texture_data.GetFormat());

    locked = (gradient_texture_->LockRect(0, &lockrect, nullptr) == OGL_OK);
  }

  BYTE* dest_buffer = (BYTE*) lockrect.pBits;
  int gradient_opacity = 255.0f * opacity();
  int buttons_opacity = 255.0f * window_buttons_->opacity();

  int first_step = button_width * (factor - 1);
  int second_step = button_width * factor;

  for (int x = 0; x < geo.width && dest_buffer && locked; x++)
  {
    BYTE r, g, b, a;

    r = 223;
    g = 219;
    b = 210;

    if (x < first_step)
    {
      int color_increment = (first_step - x) * 4;

      r = CLAMP(r + color_increment, r, 0xff);
      g = CLAMP(g + color_increment, g, 0xff);
      b = CLAMP(b + color_increment, b, 0xff);
      a = 0xff - buttons_opacity;
    }
    else if (x < second_step)
    {
      a = 0xff - gradient_opacity * (((float)x - (first_step)) / (button_width));
    }
    else
    {
      if (ShouldDrawMenus())
      {
        // If we're fading-out the title, it's better to quickly hide
        // the transparent right-most area
        a = CLAMP(0xff - gradient_opacity - 0x55, 0x00, 0xff);
      }
      else
      {
        a = 0xff - gradient_opacity;
      }
    }

    dest_buffer[4 * x]     = (r * a) / 0xff;
    dest_buffer[4 * x + 1] = (g * a) / 0xff;
    dest_buffer[4 * x + 2] = (b * a) / 0xff;
    dest_buffer[4 * x + 3] = a;
  }

  // FIXME Nux shouldn't make unity to crash if we try to unlock a wrong rect
  if (locked)
    gradient_texture_->UnlockRect(0);
}

void PanelMenuView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  if (overlay_showing_)
    return;

  nux::Geometry const& geo = GetGeometry();
  bool draw_menus = ShouldDrawMenus();
  bool draw_buttons = ShouldDrawButtons();

  GfxContext.PushClippingRectangle(geo);

  if (draw_menus)
  {
    for (auto const& entry : entries_)
      entry.second->SetDisabled(false);

    layout_->ProcessDraw(GfxContext, true);

    if (new_application_ && !is_inside_)
    {
      if (opacity() != 1.0f)
        StartFadeIn(menus_discovery_fadein_);
    }
    else
    {
      if (opacity() != 1.0f)
        StartFadeIn();

      new_app_menu_shown_ = false;
    }
  }
  else
  {
    if (opacity() != 0.0f)
    {
      layout_->ProcessDraw(GfxContext, true);
      StartFadeOut(new_app_menu_shown_ ? menus_discovery_fadeout_ : -1);
    }

    for (auto const& entry : entries_)
      entry.second->SetDisabled(true);
  }

  if (draw_buttons)
  {
    window_buttons_->ProcessDraw(GfxContext, true);

    if (window_buttons_->opacity() != 1.0f)
      StartFadeIn();
  }
  else if (window_buttons_->opacity() != 0.0f)
  {
    window_buttons_->ProcessDraw(GfxContext, true);

    /* If we try to hide only the buttons, then use a faster fadeout */
    if (opacity_animator_.CurrentState() != na::Animation::Running)
    {
      StartFadeOut(menus_fadeout_/3);
    }
  }

  GfxContext.PopClippingRectangle();
}

std::string PanelMenuView::GetActiveViewName(bool use_appname) const
{
  std::string label;
  BamfWindow* window;

  window = bamf_matcher_get_active_window(matcher_);

  if (BAMF_IS_WINDOW(window))
  {
    BamfView *view = reinterpret_cast<BamfView*>(window);
    std::vector<Window> const& our_xids = nux::XInputWindow::NativeHandleList();
    Window window_xid = bamf_window_get_xid(window);

    if (std::find(our_xids.begin(), our_xids.end(), window_xid) != our_xids.end())
    {
      /* If the active window is an unity window, we need to fallback to the
       * top one, anyway we should always avoid to focus unity internal windows */
      BamfWindow* top_win = GetBamfWindowForXid(GetTopWindow());

      if (top_win && top_win != window)
      {
        window = top_win;
      }
      else
      {
        return "";
      }
    }

    if (bamf_window_get_window_type(window) == BAMF_WINDOW_DESKTOP)
    {
      label = desktop_name_;
    }
    else if (!IsValidWindow(window_xid))
    {
       return "";
    }

    if (WindowManager::Default().IsWindowMaximized(window_xid) && !use_appname)
    {
      label = glib::String(bamf_view_get_name(view)).Str();
    }

    if (label.empty())
    {
      BamfApplication* app;
      app = bamf_matcher_get_application_for_window(matcher_, window);

      if (BAMF_IS_APPLICATION(app))
      {
        view = reinterpret_cast<BamfView*>(app);
        label = glib::String(bamf_view_get_name(view)).Str();
      }
    }

    if (label.empty())
    {
      view = reinterpret_cast<BamfView*>(window);
      label = glib::String(bamf_view_get_name(view)).Str();
    }
  }

  return label;
}

void PanelMenuView::UpdateTitleTexture(cairo_t *cr, nux::Geometry const& geo, std::string const& label) const
{
  using namespace decoration;
  auto const& style = Style::Get();
  auto text_size = style->TitleNaturalSize(label);
  int x = MAIN_LEFT_PADDING + TITLE_PADDING + geo.x;

  cairo_save(cr);
  cairo_translate(cr, x, geo.y + (geo.height - text_size.height) / 2);
  style->DrawTitle(label, WidgetState::NORMAL, cr, geo.width - x, geo.height);
  cairo_restore(cr);
}

std::string PanelMenuView::GetCurrentTitle() const
{
  if (!switcher_showing_ && !launcher_keynav_)
  {
    WindowManager& wm = WindowManager::Default();
    std::string new_title;

    if (wm.IsScaleActive())
    {
      if (wm.IsScaleActiveForGroup())
        new_title = GetActiveViewName(true);
      else if (we_control_active_)
        new_title = desktop_name_;
    }
    else if (wm.IsExpoActive())
    {
      new_title = desktop_name_;
    }
    else if (!we_control_active_)
    {
      new_title.clear();
    }
    else
    {
      new_title = GetActiveViewName();
      window_buttons_->controlled_window = active_xid_;
    }

    // panel_title_ needs to be only escaped when computed
    // in this function, if it comes from OnLauncherSelectionChanged
    // it is already escaped
    glib::String escaped(g_markup_escape_text(new_title.c_str(), -1));
    return escaped.Str();
  }
  else
  {
    return panel_title_;
  }
}

bool PanelMenuView::Refresh(bool force)
{
  nux::Geometry const& geo = GetGeometry();

  // We can get into a race that causes the geometry to be wrong as there hasn't been a
  // layout cycle before the first callback. This is to protect from that.
  if (geo.width > monitor_geo_.width)
    return false;

  const std::string& new_title = GetCurrentTitle();
  if (new_title == panel_title_ && !force && last_geo_ == geo && title_texture_)
  {
    // No need to redraw the title, let's save some CPU time!
    return false;
  }

  panel_title_ = new_title;

  if (panel_title_.empty())
  {
    title_texture_ = nullptr;
    return true;
  }

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, geo.width, geo.height);
  cairo_t* cr = cairo_graphics.GetInternalContext();
  UpdateTitleTexture(cr, geo, panel_title_);
  title_texture_ = texture_ptr_from_cairo_graphics(cairo_graphics);

  return true;
}

void PanelMenuView::OnActiveChanged(PanelIndicatorEntryView* view, bool is_active)
{
  if (is_active)
  {
    last_active_view_ = view;
  }
  else
  {
    if (last_active_view_ == view)
    {
      last_active_view_ = nullptr;
    }
  }

  if (Refresh())
    FullRedraw();
}

void PanelMenuView::OnEntryAdded(indicator::Entry::Ptr const& entry)
{
  PanelIndicatorEntryView* view;

  view = new PanelIndicatorEntryView(entry, MENU_ENTRIES_PADDING, IndicatorEntryType::MENU);
  view->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  view->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));

  entry->show_now_changed.connect(sigc::mem_fun(this, &PanelMenuView::UpdateShowNow));
  view->active_changed.connect(sigc::mem_fun(this, &PanelMenuView::OnActiveChanged));

  AddEntryView(view, IndicatorEntryPosition::END);
}

void PanelMenuView::NotifyAllMenusClosed()
{
  last_active_view_ = nullptr;

  auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  is_inside_ = GetAbsoluteGeometry().IsInside(mouse);
  FullRedraw();
}

void PanelMenuView::OnNameChanged(BamfView* bamf_view, gchar* new_name, gchar* old_name)
{
  if (Refresh())
    FullRedraw();
}

bool PanelMenuView::OnNewAppShow()
{
  BamfApplication* active_app = bamf_matcher_get_active_application(matcher_);
  new_application_ = glib::Object<BamfApplication>(active_app, glib::AddRef());
  QueueDraw();

  if (sources_.GetSource(NEW_APP_HIDE_TIMEOUT))
  {
    new_app_menu_shown_ = false;
  }

  auto cb_func = sigc::mem_fun(this, &PanelMenuView::OnNewAppHide);
  sources_.AddTimeoutSeconds(menus_discovery_, cb_func, NEW_APP_HIDE_TIMEOUT);

  return false;
}

bool PanelMenuView::OnNewAppHide()
{
  OnApplicationClosed(new_application_);
  new_app_menu_shown_ = true;
  QueueDraw();

  return false;
}

void PanelMenuView::OnViewOpened(BamfMatcher *matcher, BamfView *view)
{
  /* FIXME: here we should also check for if the view is also user_visible
   * but it seems that BAMF doesn't handle this correctly after some
   * stress tests (repeated launches). */
  if (!BAMF_IS_APPLICATION(view))
    return;

  new_apps_.push_front(glib::Object<BamfApplication>(BAMF_APPLICATION(view), glib::AddRef()));
}

void PanelMenuView::OnApplicationClosed(BamfApplication* app)
{
  if (BAMF_IS_APPLICATION(app))
  {
    if (std::find(new_apps_.begin(), new_apps_.end(), app) != new_apps_.end())
    {
      new_apps_.remove(glib::Object<BamfApplication>(app, glib::AddRef()));
    }
    else if (new_apps_.empty())
    {
      new_application_ = nullptr;
    }
  }

  if (app == new_application_)
  {
    new_application_ = nullptr;
  }
}

void PanelMenuView::OnViewClosed(BamfMatcher *matcher, BamfView *view)
{
  if (reinterpret_cast<BamfView*>(view_name_changed_signal_.object()) == view)
  {
    view_name_changed_signal_.Disconnect();
  }

  if (BAMF_IS_APPLICATION(view))
  {
    OnApplicationClosed(reinterpret_cast<BamfApplication*>(view));
  }
  else if (reinterpret_cast<BamfApplication*>(view) == new_application_)
  {
    new_application_ = nullptr;
  }
  else if (BAMF_IS_WINDOW(view))
  {
    /* FIXME, this can be removed when window_unmapped WindowManager signal
     * will emit the proper xid */
    Window xid = bamf_window_get_xid(reinterpret_cast<BamfWindow*>(view));
    OnWindowUnmapped(xid);
  }
}

void PanelMenuView::OnActiveAppChanged(BamfMatcher *matcher,
                                       BamfApplication* old_app,
                                       BamfApplication* new_app)
{
  if (BAMF_IS_APPLICATION(new_app))
  {
    app_name_changed_signal_.Disconnect();
    app_name_changed_signal_.Connect(BAMF_VIEW(new_app), "name-changed",
                                     sigc::mem_fun(this, &PanelMenuView::OnNameChanged));

    if (std::find(new_apps_.begin(), new_apps_.end(), new_app) != new_apps_.end())
    {
      if (new_application_ != new_app)
      {
        /* Add a small delay before showing the menus, this is done both
         * to fix the issues with applications that takes some time to loads
         * menus and to show the menus only when an application has been
         * kept active for some time */

        auto cb_func = sigc::mem_fun(this, &PanelMenuView::OnNewAppShow);
        sources_.AddTimeout(300, cb_func, NEW_APP_SHOW_TIMEOUT);
      }
    }
    else
    {
      sources_.Remove(NEW_APP_SHOW_TIMEOUT);

      if (sources_.GetSource(NEW_APP_HIDE_TIMEOUT))
      {
        sources_.Remove(NEW_APP_HIDE_TIMEOUT);
        new_app_menu_shown_ = false;
      }

      if (new_application_)
        OnApplicationClosed(new_application_);
    }
  }
}

void PanelMenuView::OnActiveWindowChanged(BamfMatcher *matcher, BamfView* old_view, BamfView* new_view)
{
  show_now_activated_ = false;
  is_maximized_ = false;
  active_xid_ = 0;

  sources_.Remove(WINDOW_MOVED_TIMEOUT);

  if (BAMF_IS_WINDOW(new_view))
  {
    BamfWindow* window = reinterpret_cast<BamfWindow*>(new_view);
    active_xid_ = bamf_window_get_xid(window);
    is_maximized_ = (bamf_window_maximized(window) == BAMF_WINDOW_MAXIMIZED);

    if (bamf_window_get_window_type(window) == BAMF_WINDOW_DESKTOP)
      we_control_active_ = true;
    else
      we_control_active_ = IsWindowUnderOurControl(active_xid_);

    if (is_maximized_)
      maximized_set_.insert(active_xid_);

    // first see if we need to remove and old callback
    view_name_changed_signal_.Disconnect();
    // register callback for new view
    view_name_changed_signal_.Connect(new_view, "name-changed",
                                      sigc::mem_fun(this, &PanelMenuView::OnNameChanged));

    window_buttons_->controlled_window = (is_maximized_) ? active_xid_ : 0;
  }

  if (Refresh())
    FullRedraw();
}

void PanelMenuView::OnSpreadInitiate()
{
  if (Refresh())
    QueueDraw();
}

void PanelMenuView::OnSpreadTerminate()
{
  if (Refresh())
    QueueDraw();
}

void PanelMenuView::OnExpoInitiate()
{
  if (Refresh())
    QueueDraw();
}

void PanelMenuView::OnExpoTerminate()
{
  if (Refresh())
    QueueDraw();
}

void PanelMenuView::OnWindowMinimized(Window xid)
{
  maximized_set_.erase(xid);

  if (xid == active_xid_)
  {
    if (Refresh())
      QueueDraw();
  }
}

void PanelMenuView::OnWindowUnminimized(Window xid)
{
  if (WindowManager::Default().IsWindowMaximized(xid))
    maximized_set_.insert(xid);

  if (xid == active_xid_)
  {
    if (Refresh())
      QueueDraw();
  }
}

void PanelMenuView::OnWindowUnmapped(Window xid)
{
  // FIXME: compiz doesn't give us a valid xid (is always 0 on unmap)
  // we need to do this again on BamfView closed signal.
  maximized_set_.erase(xid);

  if (xid == active_xid_)
  {
    if (Refresh())
      QueueDraw();
  }
}

void PanelMenuView::OnWindowMapped(Window xid)
{
  if (WindowManager::Default().IsWindowMaximized(xid))
  {
    maximized_set_.insert(xid);

    if (xid == active_xid_)
    {
      if (Refresh())
        QueueDraw();
    }
  }
}

void PanelMenuView::OnWindowMaximized(Window xid)
{
  maximized_set_.insert(xid);

  if (xid == active_xid_)
  {
    // We need to update the is_inside_ state in the case of maximization by grab
    auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
    is_inside_ = GetAbsoluteGeometry().IsInside(mouse);
    is_maximized_ = true;

    if (Refresh())
      FullRedraw();
  }
}

void PanelMenuView::OnWindowRestored(Window xid)
{
  maximized_set_.erase(xid);

  if (active_xid_ == xid)
  {
    is_maximized_ = false;
    is_grabbed_ = false;

    if (Refresh())
      FullRedraw();
  }
}

bool PanelMenuView::UpdateActiveWindowPosition()
{
  bool we_control_window = IsWindowUnderOurControl(active_xid_);

  if (we_control_window != we_control_active_)
  {
    we_control_active_ = we_control_window;

    if (Refresh())
      QueueDraw();
  }

  return false;
}

void PanelMenuView::OnWindowMoved(Window xid)
{
  if (active_xid_ == xid)
  {
    /* When moving the active window, if the current panel is controlling
     * the active window, then we postpone the timeout function every movement
     * that we have, setting a longer timeout.
     * Otherwise, if the moved window is not controlled by the current panel
     * every few millisecond we check the new window position */

    unsigned int timeout_length = 250;

    if (we_control_active_)
    {
      sources_.Remove(WINDOW_MOVED_TIMEOUT);
    }
    else
    {
      if (sources_.GetSource(WINDOW_MOVED_TIMEOUT))
        return;

      timeout_length = 60;
    }

    auto cb_func = sigc::mem_fun(this, &PanelMenuView::UpdateActiveWindowPosition);
    sources_.AddTimeout(timeout_length, cb_func, WINDOW_MOVED_TIMEOUT);
  }
}

bool PanelMenuView::IsWindowUnderOurControl(Window xid) const
{
  if (UScreen::GetDefault()->GetMonitors().size() > 1)
  {
    WindowManager& wm = WindowManager::Default();
    nux::Geometry const& window_geo = wm.GetWindowGeometry(xid);
    nux::Geometry const& intersect = monitor_geo_.Intersect(window_geo);

    /* We only care of the horizontal window portion */
    return (intersect.width > window_geo.width/2 && intersect.height > 0);
  }

  return true;
}

bool PanelMenuView::IsValidWindow(Window xid) const
{
  WindowManager& wm = WindowManager::Default();
  std::vector<Window> const& our_xids = nux::XInputWindow::NativeHandleList();

  if (wm.IsWindowOnCurrentDesktop(xid) && !wm.IsWindowObscured(xid) &&
      wm.IsWindowVisible(xid) && IsWindowUnderOurControl(xid) &&
      std::find(our_xids.begin(), our_xids.end(), xid) == our_xids.end())
  {
    return true;
  }

  return false;
}

Window PanelMenuView::GetMaximizedWindow() const
{
  Window window_xid = 0;

  // Find the front-most of the maximized windows we are controlling
  for (auto xid : maximized_set_)
  {
    // We can safely assume only the front-most is visible
    if (IsValidWindow(xid))
    {
      window_xid = xid;
      break;
    }
  }

  return window_xid;
}

Window PanelMenuView::GetTopWindow() const
{
  Window window_xid = 0;
  GList* windows = bamf_matcher_get_window_stack_for_monitor(matcher_, monitor_);

  for (GList* l = windows; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    Window xid = bamf_window_get_xid(static_cast<BamfWindow*>(l->data));
    bool visible = bamf_view_is_user_visible(static_cast<BamfView*>(l->data));

    if (visible && IsValidWindow(xid))
    {
      window_xid = xid;
    }
  }

  g_list_free(windows);

  return window_xid;
}

BamfWindow* PanelMenuView::GetBamfWindowForXid(Window xid) const
{
  BamfWindow* window = nullptr;

  if (xid != 0)
  {
    GList* windows = bamf_matcher_get_windows(matcher_);

    for (GList* l = windows; l; l = l->next)
    {
      if (!BAMF_IS_WINDOW(l->data))
        continue;

      auto win = static_cast<BamfWindow*>(l->data);

      if (bamf_window_get_xid(win) == xid)
      {
        window = win;
        break;
      }
    }

    g_list_free(windows);
  }

  return window;
}

void PanelMenuView::OnMaximizedActivate(int x, int y)
{
  Window maximized = GetMaximizedWindow();

  if (maximized != 0)
  {
    WindowManager::Default().Activate(maximized);
  }
}

void PanelMenuView::OnMaximizedRestore(int x, int y)
{
  Window maximized = GetMaximizedWindow();

  if (maximized != 0)
  {
    WindowManager::Default().Restore(maximized);
    is_inside_ = true;
  }
}

void PanelMenuView::OnMaximizedLower(int x, int y)
{
  Window maximized = GetMaximizedWindow();

  if (maximized != 0)
  {
    WindowManager::Default().Lower(maximized);
  }
}

void PanelMenuView::OnMaximizedGrabStart(int x, int y)
{
  /* When Start dragging the panelmenu of a maximized window, change cursor
   * to simulate the dragging, waiting to go out of the panel area.
   *
   * This is a workaround to avoid that the grid plugin would be fired
   * showing the window shape preview effect. See bug #838923 */

  Window maximized = GetMaximizedWindow();

  if (maximized != 0)
  {
    /* Always activate the window in case it is on another monitor */
    WindowManager::Default().Activate(maximized);
    titlebar_grab_area_->SetGrabbed(true);
  }
}

void PanelMenuView::OnMaximizedGrabMove(int x, int y)
{
  auto panel = GetTopLevelViewWindow();

  if (!panel)
    return;

  /* Adjusting the x, y coordinates to get the absolute values */
  x += titlebar_grab_area_->GetAbsoluteX();
  y += titlebar_grab_area_->GetAbsoluteY();

  Window maximized = GetMaximizedWindow();

  /* When the drag goes out from the Panel, start the real movement.
   *
   * This is a workaround to avoid that the grid plugin would be fired
   * showing the window shape preview effect. See bug #838923 */
  if (maximized != 0)
  {
    nux::Geometry const& panel_geo = panel->GetAbsoluteGeometry();

    if (!panel_geo.IsPointInside(x, y))
    {
      WindowManager& wm = WindowManager::Default();
      nux::Geometry const& restored_geo = wm.GetWindowSavedGeometry(maximized);
      nux::Geometry const& workarea_geo = wm.GetWorkAreaGeometry(maximized);

      /* By default try to restore the window horizontally-centered respect to the
       * pointer position, if it doesn't fit on that area try to keep it into the
       * current workarea as much as possible, but giving priority to the left border
       * that shouldn't be never put out of the workarea */
      int restore_x = x - (restored_geo.width * (x - panel_geo.x) / panel_geo.width);
      int restore_y = y;

      if (restore_x + restored_geo.width > workarea_geo.x + workarea_geo.width)
      {
        restore_x = workarea_geo.x + workarea_geo.width - restored_geo.width;
      }

      if (restore_x < workarea_geo.x)
      {
        restore_x = workarea_geo.x;
      }

      wm.Activate(maximized);
      wm.RestoreAt(maximized, restore_x, restore_y);

      is_inside_ = true;
      is_grabbed_ = true;
      if (Refresh())
        FullRedraw();

      /* Ungrab the pointer and start the X move, to make the decorator handle it */
      titlebar_grab_area_->SetGrabbed(false);
      wm.StartMove(maximized, x, y);
    }
  }
}

void PanelMenuView::OnMaximizedGrabEnd(int x, int y)
{
  titlebar_grab_area_->SetGrabbed(false);

  x += titlebar_grab_area_->GetAbsoluteX();
  y += titlebar_grab_area_->GetAbsoluteY();
  is_inside_ = GetAbsoluteGeometry().IsPointInside(x, y);

  if (!is_inside_)
    is_grabbed_ = false;

  if (Refresh())
    FullRedraw();
}

// Introspectable
std::string
PanelMenuView::GetName() const
{
  return "MenuView";
}

void PanelMenuView::AddProperties(debug::IntrospectionData& introspection)
{
  PanelIndicatorsView::AddProperties(introspection);

  introspection
  .add("mouse_inside", is_inside_)
  .add("grabbed", is_grabbed_)
  .add("active_win_maximized", is_maximized_)
  .add("panel_title", panel_title_)
  .add("desktop_active", (panel_title_ == desktop_name_))
  .add("monitor", monitor_)
  .add("active_window", (uint64_t)active_xid_)
  .add("draw_menus", ShouldDrawMenus())
  .add("draw_window_buttons", ShouldDrawButtons())
  .add("controls_active_window", we_control_active_)
  .add("fadein_duration", menus_fadein_)
  .add("fadeout_duration", menus_fadeout_)
  .add("discovery_duration", menus_discovery_)
  .add("discovery_fadein_duration", menus_discovery_fadein_)
  .add("discovery_fadeout_duration", menus_discovery_fadeout_);
}

void PanelMenuView::OnSwitcherShown(GVariant* data)
{
  if (!data)
    return;

  gboolean switcher_shown;
  gint monitor;
  g_variant_get(data, "(bi)", &switcher_shown, &monitor);

  if (switcher_shown == switcher_showing_ || monitor != monitor_)
    return;

  switcher_showing_ = switcher_shown;

  if (!switcher_showing_)
  {
    auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
    is_inside_ = GetAbsoluteGeometry().IsInside(mouse);
  }
  else
  {
    show_now_activated_ = false;
  }

  if (Refresh())
    QueueDraw();
}

void PanelMenuView::OnLauncherKeyNavStarted(GVariant* data)
{
  if (launcher_keynav_)
    return;


  if (!data || (data && g_variant_get_int32(data) == monitor_))
  {
    launcher_keynav_ = true;
  }
}

void PanelMenuView::OnLauncherKeyNavEnded(GVariant* data)
{
  if (!launcher_keynav_)
    return;

  launcher_keynav_ = false;

  auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  is_inside_ = GetAbsoluteGeometry().IsInside(mouse);

  if (Refresh())
    QueueDraw();
}

void PanelMenuView::OnLauncherSelectionChanged(GVariant* data)
{
  if (!data || !launcher_keynav_)
    return;

  const gchar *title = g_variant_get_string(data, 0);
  panel_title_ = (title ? title : "");

  Refresh(true);
  QueueDraw();
}

bool PanelMenuView::UpdateShowNowWithDelay()
{
  bool active = false;

  for (auto const& entry : entries_)
  {
    if (entry.second->GetShowNow())
    {
      active = true;
      break;
    }
  }

  if (active)
  {
    show_now_activated_ = true;
    QueueDraw();
  }

  return false;
}

void PanelMenuView::UpdateShowNow(bool status)
{
  /* When we get a show now event, if we are requested to show the menus,
   * we take the last incoming event and we wait for small delay (to avoid the
   * Alt+Tab conflict) then we check if any menuitem has requested to show.
   * If the status is false, we just check that the menus entries are hidden
   * and we remove any eventual delayed request */

   sources_.Remove(UPDATE_SHOW_NOW_TIMEOUT);

  if (!status && show_now_activated_)
  {
    show_now_activated_ = false;
    QueueDraw();
    return;
  }

  if (status && !show_now_activated_)
  {
    auto cb_func = sigc::mem_fun(this, &PanelMenuView::UpdateShowNowWithDelay);
    sources_.AddTimeout(180, cb_func, UPDATE_SHOW_NOW_TIMEOUT);
  }
}

void PanelMenuView::SetMonitor(int monitor)
{
  monitor_ = monitor;
  monitor_geo_ = UScreen::GetDefault()->GetMonitorGeometry(monitor_);

  maximized_set_.clear();
  GList* windows = bamf_matcher_get_window_stack_for_monitor(matcher_, monitor_);

  for (GList* l = windows; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    auto window = static_cast<BamfWindow*>(l->data);
    auto view = static_cast<BamfView*>(l->data);

    if (bamf_view_is_active(view))
      active_xid_ = bamf_window_get_xid(window);

    if (bamf_window_maximized(window) == BAMF_WINDOW_MAXIMIZED)
      maximized_set_.insert(bamf_window_get_xid(window));
  }

  Window maximized = GetMaximizedWindow();
  Window buttons_win = (maximized == active_xid_) ? maximized : 0;

  window_buttons_->monitor = monitor_;
  window_buttons_->controlled_window = buttons_win;

  g_list_free(windows);
}

bool PanelMenuView::GetControlsActive() const
{
  return we_control_active_;
}

void PanelMenuView::OnPanelViewMouseEnter(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state)
{
  if (!is_inside_)
  {
    if (is_grabbed_)
      is_grabbed_ = false;
    else
      is_inside_ = true;

    FullRedraw();
  }
}

void PanelMenuView::OnPanelViewMouseLeave(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state)
{
  if (is_inside_)
  {
    is_inside_ = false;
    FullRedraw();
  }
}

void PanelMenuView::SetMousePosition(int x, int y)
{
  if (last_active_view_ ||
      (x >= 0 && y >= 0 && GetAbsoluteGeometry().IsPointInside(x, y)))
  {
    if (!is_inside_)
    {
      is_inside_ = true;
      FullRedraw();
    }
  }
  else
  {
    if (is_inside_)
    {
      is_inside_ = false;
      FullRedraw();
    }
  }
}
} // namespace unity
