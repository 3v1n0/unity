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
#include <UnityCore/GLibWrapper.h>
#include <boost/algorithm/string/erase.hpp>

#include "PanelMenuView.h"
#include "unity-shared/AnimationUtils.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/DecorationStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/RawPixel.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

#include "config.h"
#include <glib/gi18n-lib.h>

namespace unity
{
namespace panel
{
DECLARE_LOGGER(logger, "unity.panel.menu");

namespace
{
  const RawPixel MAIN_LEFT_PADDING = 4_em;
  const RawPixel TITLE_PADDING = 2_em;
  const RawPixel MENUBAR_PADDING = 4_em;
  const int MENU_ENTRIES_PADDING = 6;

  const std::string NEW_APP_HIDE_TIMEOUT = "new-app-hide-timeout";
  const std::string NEW_APP_SHOW_TIMEOUT = "new-app-show-timeout";
  const std::string WINDOW_MOVED_TIMEOUT = "window-moved-timeout";
  const std::string UPDATE_SHOW_NOW_TIMEOUT = "update-show-now-timeout";
  const std::string INTEGRATED_MENUS_DOUBLE_CLICK_TIMEOUT = "integrated-menus-double-click-timeout";

std::string get_current_desktop()
{
  std::ifstream fin("/etc/os-release");
  std::string temp;
  std::string os_release_name("Ubuntu");

  if (fin.is_open())
  {
    while (getline(fin, temp))
    {
      if (temp.substr(0, 4) == "NAME")
      {
        os_release_name = boost::erase_all_copy(temp.substr(temp.find_last_of('=')+1), "\"");
        break;
      }
    }
    fin.close();
  }

  //this is done to avoid breaking translation before 14.10.
  if (os_release_name.empty() || os_release_name == "Ubuntu")
    return _("Ubuntu Desktop");
  else
    return glib::String(g_strdup_printf(_("%s Desktop"), os_release_name.c_str())).Str();
}
}

PanelMenuView::PanelMenuView(menu::Manager::Ptr const& menus)
  : active_window(0)
  , maximized_window(0)
  , focused(true)
  , menu_manager_(menus)
  , is_inside_(false)
  , is_grabbed_(false)
  , is_maximized_(false)
  , is_desktop_focused_(false)
  , last_active_view_(nullptr)
  , switcher_showing_(false)
  , spread_showing_(false)
  , launcher_keynav_(false)
  , show_now_activated_(false)
  , we_control_active_(false)
  , new_app_menu_shown_(false)
  , ignore_menu_visibility_(false)
  , integrated_menus_(menu_manager_->integrated_menus())
  , always_show_menus_(menu_manager_->always_show_menus())
  , ignore_leave_events_(false)
  , desktop_name_(get_current_desktop())
{
  if (ApplicationWindowPtr const& win = ApplicationManager::Default().GetActiveWindow())
    active_window = win->window_id();

  SetupWindowButtons();
  SetupTitlebarGrabArea();
  SetupPanelMenuViewSignals();
  SetupWindowManagerSignals();
  SetupUBusManagerInterests();

  opacity = 0.0f;

  RefreshAndRedraw();
}

PanelMenuView::~PanelMenuView()
{
  window_buttons_->UnParentObject();
  titlebar_grab_area_->UnParentObject();
}

void PanelMenuView::OnStyleChanged()
{
  int height = panel::Style::Instance().PanelHeight(monitor_);
  double scale = Settings::Instance().em(monitor_)->DPIScale();
  window_buttons_->SetMinimumHeight(height);
  window_buttons_->SetMaximumHeight(height);
  window_buttons_->SetLeftAndRightPadding(MAIN_LEFT_PADDING.CP(scale), MENUBAR_PADDING.CP(scale));
  window_buttons_->UpdateDPIChanged();

  layout_->SetLeftAndRightPadding(window_buttons_->GetContentWidth(), 0);

  Refresh(true);
  FullRedraw();
}

void PanelMenuView::SetupPanelMenuViewSignals()
{
  auto& am = ApplicationManager::Default();
  am.active_window_changed.connect(sigc::mem_fun(this, &PanelMenuView::OnActiveWindowChanged));
  am.active_application_changed.connect(sigc::mem_fun(this, &PanelMenuView::OnActiveAppChanged));
  am.application_started.connect(sigc::mem_fun(this, &PanelMenuView::OnApplicationStarted));
  am.application_stopped.connect(sigc::mem_fun(this, &PanelMenuView::OnApplicationClosed));
  am.window_opened.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowOpened));
  am.window_closed.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowClosed));

  mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));
  opacity_animator_.updated.connect(sigc::mem_fun(this, &PanelMenuView::OnFadeAnimatorUpdated));
  entry_added.connect(sigc::mem_fun(this, &PanelMenuView::OnEntryViewAdded));
  Style::Instance().changed.connect(sigc::mem_fun(this, &PanelMenuView::OnStyleChanged));

  menu_manager_->integrated_menus.changed.connect(sigc::mem_fun(this, &PanelMenuView::OnLIMChanged));
  menu_manager_->always_show_menus.changed.connect(sigc::mem_fun(this, &PanelMenuView::OnAlwaysShowMenusChanged));

  auto update_target_cb = sigc::hide(sigc::mem_fun(this, &PanelMenuView::UpdateTargetWindowItems));
  maximized_window.changed.connect(update_target_cb);
  active_window.changed.connect(update_target_cb);

  focused.changed.connect([this] (bool focused) {
    Refresh(true);
    window_buttons_->focused = focused;

    for (auto const& entry : entries_)
      entry.second->SetFocusedState(focused);

    FullRedraw();
  });
}

void PanelMenuView::SetupWindowButtons()
{
  window_buttons_ = new WindowButtons();
  window_buttons_->SetParentObject(this);
  window_buttons_->monitor = monitor_;
  window_buttons_->opacity = 0.0f;
  window_buttons_->SetLeftAndRightPadding(MAIN_LEFT_PADDING, MENUBAR_PADDING);
  window_buttons_->SetMaximumHeight(panel::Style::Instance().PanelHeight(monitor_));
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
}

void PanelMenuView::SetupTitlebarGrabArea()
{
  titlebar_grab_area_ = new PanelTitlebarGrabArea();
  titlebar_grab_area_->SetParentObject(this);
  titlebar_grab_area_->clicked.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedActivate));
  titlebar_grab_area_->double_clicked.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedDoubleClicked));
  titlebar_grab_area_->middle_clicked.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedMiddleClicked));
  titlebar_grab_area_->right_clicked.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedRightClicked));
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
  wm.window_fullscreen.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMaximized));
  wm.window_unfullscreen.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUnFullscreen));
  wm.window_unmapped.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUnmapped));
  wm.window_mapped.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMapped));
  wm.window_moved.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMoved));
  wm.window_resized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMoved));
  wm.initiate_spread.connect(sigc::mem_fun(this, &PanelMenuView::OnSpreadInitiate));
  wm.terminate_spread.connect(sigc::mem_fun(this, &PanelMenuView::OnSpreadTerminate));
  wm.initiate_expo.connect(sigc::mem_fun(this, &PanelMenuView::RefreshAndRedraw));
  wm.terminate_expo.connect(sigc::mem_fun(this, &PanelMenuView::RefreshAndRedraw));
  wm.screen_viewport_switch_ended.connect(sigc::mem_fun(this, &PanelMenuView::RefreshAndRedraw));
  wm.show_desktop_changed.connect(sigc::mem_fun(this, &PanelMenuView::OnShowDesktopChanged));
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

void PanelMenuView::UpdateTargetWindowItems()
{
  Window old_target = window_buttons_->controlled_window;
  Window target_window = integrated_menus_ ? maximized_window() : active_window();

  if (old_target != target_window)
  {
    window_buttons_->controlled_window = target_window;

    if (ApplicationWindowPtr const& win = ApplicationManager::Default().GetWindowForId(target_window))
    {
      auto refresh_cb = sigc::hide(sigc::mem_fun(this, &PanelMenuView::RefreshAndRedraw));
      win_name_changed_conn_ = win->title.changed.connect(refresh_cb);

      if (ApplicationPtr const& app = win->application())
        app_name_changed_conn_ = app->title.changed.connect(refresh_cb);
    }

    ClearEntries();

    if (indicator::AppmenuIndicator::Ptr appmenu = menu_manager_->AppMenu())
    {
      for (auto const& entry : appmenu->GetEntriesForWindow(target_window))
        OnEntryAdded(entry);
    }
  }

  if (integrated_menus_)
    focused = (target_window == active_window());
}

void PanelMenuView::FullRedraw()
{
  QueueDraw();
  window_buttons_->QueueDraw();
}

void PanelMenuView::OnLIMChanged(bool lim)
{
  integrated_menus_ = lim;
  new_application_.reset();

  if (!integrated_menus_)
  {
    CheckMouseInside();
    focused = true;
  }

  UpdateTargetWindowItems();

  Refresh(true);
  FullRedraw();
}

void PanelMenuView::OnAlwaysShowMenusChanged(bool always_show_menus)
{
  always_show_menus_ = always_show_menus;

  if (!always_show_menus_)
    CheckMouseInside();

  QueueDraw();
}

nux::Area* PanelMenuView::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  bool mouse_inside = TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type);

  if (!mouse_inside)
    return nullptr;

  if (WindowManager::Default().IsExpoActive())
    return nullptr;

  Area* found_area = nullptr;

  if (!integrated_menus_ && !we_control_active_ && !spread_showing_)
  {
    /* When the current panel is not active, it all behaves like a grab-area */
    if (GetAbsoluteGeometry().IsInside(mouse_position))
      return titlebar_grab_area_.GetPointer();
  }

  if (is_maximized_ || spread_showing_ || (integrated_menus_ && maximized_window() != 0))
  {
    found_area = window_buttons_->FindAreaUnderMouse(mouse_position, event_type);
    NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
  }

  if (titlebar_grab_area_)
  {
    found_area = titlebar_grab_area_->FindAreaUnderMouse(mouse_position, event_type);
    NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);

    if (integrated_menus_ && maximized_window() != 0)
    {
      /* When the integrated menus are enabled, that area must act both like an
       * indicator-entry view and like a panel-grab-area, so not to re-implement
       * what we have done into PanelTitleGrabAreaView inside PanelIndicatorEntryView
       * we can just redirect the input events directed to the integrated menus
       * to the grab_area, then we just have to use few tricks when we get the
       * signals back from the grab-area to check where they really were */
      return titlebar_grab_area_.GetPointer();
    }
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

  SetMaximumEntriesWidth(geo.width - window_buttons_->GetContentWidth());

  layout_->ComputeContentSize();
  int layout_width = layout_->GetContentWidth();

  titlebar_grab_area_->SetBaseX(layout_width);
  titlebar_grab_area_->SetBaseHeight(geo.height);
  titlebar_grab_area_->SetMinimumWidth(geo.width - layout_width);
  titlebar_grab_area_->SetMaximumWidth(geo.width - layout_width);
}

void PanelMenuView::StartFadeIn(int duration)
{
  opacity_animator_.SetDuration(duration >= 0 ? duration : menu_manager_->fadein());
  animation::StartOrReverse(opacity_animator_, animation::Direction::FORWARD);
}

void PanelMenuView::StartFadeOut(int duration)
{
  opacity_animator_.SetDuration(duration >= 0 ? duration : menu_manager_->fadeout());
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
  if ((we_control_active_ || integrated_menus_) && !switcher_showing_ && !launcher_keynav_ && !ignore_menu_visibility_ && HasVisibleMenus())
  {
    WindowManager& wm = WindowManager::Default();

    if (!wm.IsExpoActive() && !wm.IsScaleActive())
    {
      if (is_inside_ || last_active_view_ || show_now_activated_ || new_application_ || always_show_menus_)
        return true;

      if (is_maximized_)
        return (window_buttons_->IsMouseOwner() || titlebar_grab_area_->IsMouseOwner());
    }
  }

  return false;
}

bool PanelMenuView::ShouldDrawButtons() const
{
  if (spread_showing_)
    return true;

  if (integrated_menus_)
  {
    if (!WindowManager::Default().IsExpoActive())
      return (maximized_window() != 0);

    return false;
  }

  if (we_control_active_ && is_maximized_ && !launcher_keynav_ && !switcher_showing_)
  {
    if (!WindowManager::Default().IsExpoActive())
    {
      if (is_inside_ || show_now_activated_ || new_application_ || always_show_menus_)
        return true;

      if (window_buttons_->IsMouseOwner() || titlebar_grab_area_->IsMouseOwner())
        return true;
    }
  }

  return false;
}

bool PanelMenuView::ShouldDrawFadingTitle() const
{
  if (integrated_menus_)
    return false;

  bool draw_buttons = ShouldDrawButtons();

  return (!draw_buttons && we_control_active_ && HasVisibleMenus() &&
          (draw_buttons || (opacity() > 0.0f && window_buttons_->opacity() == 0.0f)));
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
  bool is_title_soild;
  bool has_menu = HasVisibleMenus();

  if (!integrated_menus_)
  {
    is_title_soild = we_control_active_ && (!has_menu || opacity() == 0.0) &&
                     window_buttons_->opacity() == 0.0;
  }
  else
  {
    is_title_soild = is_maximized_ && (!has_menu || opacity() == 0.0);
  }

  if (!is_title_soild)
  {
    if (integrated_menus_)
      title_opacity -= has_menu ? opacity() : 0;
    else if (has_menu)
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
    Refresh(true);
  }
}

void PanelMenuView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  if (overlay_showing_ || spread_showing_)
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
      GfxContext.QRP_2TexMod(title_geo_.x, title_geo_.y,
                             title_geo_.width, title_geo_.height,
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
        GfxContext.QRP_1Tex(title_geo_.x, title_geo_.y, title_geo_.width, title_geo_.height,
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

  nux::SURFACE_LOCKED_RECT lockrect;
  bool build_gradient = false;
  bool locked = false;
  lockrect.pBits = 0;

  if (gradient_texture_.IsNull() || (gradient_texture_->GetWidth() != title_geo_.width))
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
    nux::NTextureData texture_data(nux::BITFMT_R8G8B8A8, title_geo_.width, 1, 1);

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

  for (int x = 0; x < title_geo_.width && dest_buffer && locked; x++)
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
  if (overlay_showing_ && !spread_showing_)
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
      if (opacity() != 1.0f && menu_manager_->discovery() > 0)
        StartFadeIn(menu_manager_->discovery_fadein());
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
      StartFadeOut(new_app_menu_shown_ ? menu_manager_->discovery_fadeout() : -1);
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
      StartFadeOut(menu_manager_->fadeout()/3);
    }
  }

  GfxContext.PopClippingRectangle();
}

std::string PanelMenuView::GetActiveViewName(bool use_appname) const
{
  std::string label;
  auto& am = ApplicationManager::Default();

  if (ApplicationWindowPtr const& window = am.GetActiveWindow())
  {
    Window window_xid = window->window_id();

    if (window->type() == WindowType::DESKTOP)
    {
      label = desktop_name_;
    }
    else if (!IsValidWindow(window_xid))
    {
      return "";
    }

    if (window_xid == maximized_window() && !use_appname)
      label = window->title();

    if (label.empty())
    {
      if (ApplicationPtr const& app = am.GetActiveApplication())
        label = app->title();
    }

    if (label.empty())
      label = window->title();
  }

  return label;
}

std::string PanelMenuView::GetMaximizedViewName(bool use_appname) const
{
  Window maximized = maximized_window();
  std::string label;

  if (ApplicationWindowPtr const& window = ApplicationManager::Default().GetWindowForId(maximized))
  {
    label = window->title();

    if (use_appname || label.empty())
    {
      if (ApplicationPtr const& app = window->application())
        label = app->title();
    }
  }

  if (label.empty() && is_desktop_focused_)
    label = desktop_name_;

  return label;
}

void PanelMenuView::UpdateTitleTexture(nux::Geometry const& geo, std::string const& label)
{
  using namespace decoration;
  auto const& style = decoration::Style::Get();
  auto text_size = style->TitleNaturalSize(label);
  auto state = WidgetState::NORMAL;
  double dpi_scale = Settings::Instance().em(monitor_)->DPIScale();

  if (integrated_menus_ && !is_desktop_focused_ && !WindowManager::Default().IsExpoActive())
  {
    title_geo_.x = geo.x + window_buttons_->GetBaseWidth() + (style->TitleIndent() * dpi_scale);

    if (!focused())
      state = WidgetState::BACKDROP;
  }
  else
  {
    title_geo_.x = geo.x + MAIN_LEFT_PADDING.CP(dpi_scale) + TITLE_PADDING.CP(dpi_scale);
  }

  title_geo_.y = geo.y + (geo.height - (text_size.height * dpi_scale)) / 2;
  title_geo_.width = std::min<int>(std::ceil(text_size.width * dpi_scale), geo.width - title_geo_.x);
  title_geo_.height = std::ceil(text_size.height * dpi_scale);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, title_geo_.width, title_geo_.height);
  cairo_surface_set_device_scale(cairo_graphics.GetSurface(), dpi_scale, dpi_scale);
  cairo_t* cr = cairo_graphics.GetInternalContext();

  auto* style_ctx = panel::Style::Instance().GetStyleContext(panel::PanelItem::TITLE);
  gtk_style_context_save(style_ctx);
  gtk_style_context_add_class(style_ctx, "panel-title");
  nux::Geometry text_bg(-title_geo_.x, -title_geo_.y, geo.width, geo.height);
  style->DrawTitle(label, state, cr, title_geo_.width / dpi_scale, title_geo_.height / dpi_scale, text_bg * (1.0/dpi_scale), style_ctx);
  title_texture_ = texture_ptr_from_cairo_graphics(cairo_graphics);
  gtk_style_context_restore(style_ctx);
}

std::string PanelMenuView::GetCurrentTitle() const
{
  if (always_show_menus_ && is_maximized_ && we_control_active_)
    return std::string();

  if (integrated_menus_ || (!switcher_showing_ && !launcher_keynav_))
  {
    std::string new_title;

    if (WindowManager::Default().IsExpoActive())
    {
      new_title = desktop_name_;
    }
    else if (integrated_menus_)
    {
      new_title = GetMaximizedViewName();
    }
    else if (!we_control_active_)
    {
      new_title.clear();
    }
    else
    {
      new_title = GetActiveViewName();
    }

    return new_title;
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

  std::string const& new_title = GetCurrentTitle();

  if (!force && new_title == panel_title_ && last_geo_ == geo && title_texture_)
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

  UpdateTitleTexture(geo, panel_title_);

  return true;
}

void PanelMenuView::RefreshAndRedraw()
{
  if (Refresh())
    QueueDraw();
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

  RefreshAndRedraw();
}

void PanelMenuView::OnEntryAdded(indicator::Entry::Ptr const& entry)
{
  auto parent_window = entry->parent_window();
  Window target = integrated_menus_ ? maximized_window() : active_window();

  if (!parent_window || parent_window == target)
    AddEntryView(new PanelIndicatorEntryView(entry, MENU_ENTRIES_PADDING, IndicatorEntryType::MENU));
}

void PanelMenuView::OnEntryViewAdded(PanelIndicatorEntryView* view)
{
  view->SetFocusedState(focused());
  view->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  view->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));
  view->active_changed.connect(sigc::mem_fun(this, &PanelMenuView::OnActiveChanged));
  view->show_now_changed.connect(sigc::mem_fun(this, &PanelMenuView::UpdateShowNow));
}

void PanelMenuView::NotifyAllMenusClosed()
{
  last_active_view_ = nullptr;

  if (!integrated_menus_ || is_maximized_)
  {
    bool was_inside = is_inside_;

    if (CheckMouseInside() != was_inside)
      QueueDraw();
  }
}

bool PanelMenuView::OnNewAppShow()
{
  new_application_ = ApplicationManager::Default().GetActiveApplication();
  QueueDraw();

  if (sources_.GetSource(NEW_APP_HIDE_TIMEOUT))
    new_app_menu_shown_ = false;

  auto cb_func = sigc::mem_fun(this, &PanelMenuView::OnNewAppHide);
  sources_.AddTimeoutSeconds(menu_manager_->discovery(), cb_func, NEW_APP_HIDE_TIMEOUT);

  return false;
}

bool PanelMenuView::OnNewAppHide()
{
  OnApplicationClosed(new_application_);
  new_app_menu_shown_ = true;
  QueueDraw();

  return false;
}

void PanelMenuView::OnApplicationStarted(ApplicationPtr const& app)
{
  /* FIXME: here we should also check for if the view is also user_visible
   * but it seems that BAMF doesn't handle this correctly after some
   * stress tests (repeated launches). */
  if (integrated_menus_)
    return;

  new_apps_.push_front(app);
}

void PanelMenuView::OnApplicationClosed(ApplicationPtr const& app)
{
  if (app && !integrated_menus_)
  {
    if (std::find(new_apps_.begin(), new_apps_.end(), app) != new_apps_.end())
    {
      new_apps_.remove(app);
    }
    else if (new_apps_.empty())
    {
      new_application_.reset();
    }
  }

  if (app == new_application_)
  {
    new_application_.reset();
  }
}

void PanelMenuView::OnWindowOpened(ApplicationWindowPtr const& win)
{
  if (win->window_id() == window_buttons_->controlled_window() &&
      win->title.changed.empty())
  {
    /* This is a not so nice workaround that we need to include here, since
     * BAMF might be late in informing us about a new window, and thus we
     * can't connect to it's signals (as not available in the App Manager). */
    window_buttons_->controlled_window = 0;
    UpdateTargetWindowItems();
  }
}

void PanelMenuView::OnWindowClosed(ApplicationWindowPtr const& win)
{
  /* FIXME, this can be removed when window_unmapped WindowManager signal
   * will emit the proper xid */
  OnWindowUnmapped(win->window_id());
}

void PanelMenuView::OnActiveAppChanged(ApplicationPtr const& new_app)
{
  if (new_app)
  {
    if (integrated_menus_ || always_show_menus_)
      return;

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

void PanelMenuView::OnActiveWindowChanged(ApplicationWindowPtr const& new_win)
{
  show_now_activated_ = false;
  is_maximized_ = false;
  is_desktop_focused_ = false;
  Window active_xid = 0;

  sources_.Remove(WINDOW_MOVED_TIMEOUT);

  if (new_win)
  {
    active_xid = new_win->window_id();
    is_maximized_ = new_win->maximized() || WindowManager::Default().IsWindowFullscreen(active_xid);

    if (new_win->type() == WindowType::DESKTOP)
    {
      is_desktop_focused_ = !maximized_window();
      we_control_active_ = true;
    }
    else
    {
      we_control_active_ = IsWindowUnderOurControl(active_xid);
    }

    if (is_maximized_)
    {
      maximized_wins_.erase(std::remove(maximized_wins_.begin(), maximized_wins_.end(), active_xid), maximized_wins_.end());
      maximized_wins_.push_front(active_xid);
      UpdateMaximizedWindow();
    }
  }

  active_window = active_xid;
  RefreshAndRedraw();
}

void PanelMenuView::OnSpreadInitiate()
{
  spread_showing_ = true;
  QueueDraw();
}

void PanelMenuView::OnSpreadTerminate()
{
  spread_showing_ = false;
  QueueDraw();
}

void PanelMenuView::OnWindowMinimized(Window xid)
{
  maximized_wins_.erase(std::remove(maximized_wins_.begin(), maximized_wins_.end(), xid), maximized_wins_.end());
  UpdateMaximizedWindow();

  if (xid == active_window())
  {
    RefreshAndRedraw();
  }
  else if (integrated_menus_ && window_buttons_->controlled_window == xid)
  {
    RefreshAndRedraw();
  }
}

void PanelMenuView::OnWindowUnminimized(Window xid)
{
  if (xid == active_window())
  {
    if (WindowManager::Default().IsWindowMaximized(xid))
    {
      maximized_wins_.push_front(xid);
      UpdateMaximizedWindow();
    }

    RefreshAndRedraw();
  }
  else
  {
    if (WindowManager::Default().IsWindowMaximized(xid))
    {
      maximized_wins_.push_back(xid);
      UpdateMaximizedWindow();
    }

    if (integrated_menus_ && IsWindowUnderOurControl(xid))
    {
      RefreshAndRedraw();
    }
  }
}

void PanelMenuView::OnWindowUnmapped(Window xid)
{
  // FIXME: compiz doesn't give us a valid xid (is always 0 on unmap)
  // we need to do this again on BamfView closed signal.
  maximized_wins_.erase(std::remove(maximized_wins_.begin(), maximized_wins_.end(), xid), maximized_wins_.end());
  UpdateMaximizedWindow();

  if (xid == active_window())
  {
    RefreshAndRedraw();
  }
  else if (integrated_menus_ && window_buttons_->controlled_window == xid)
  {
    RefreshAndRedraw();
  }
}

void PanelMenuView::OnWindowMapped(Window xid)
{
  if (WindowManager::Default().IsWindowMaximized(xid))
  {
    if (xid == active_window())
    {
      maximized_wins_.push_front(xid);
      UpdateMaximizedWindow();

      RefreshAndRedraw();
    }
    else
    {
      maximized_wins_.push_back(xid);
      UpdateMaximizedWindow();
    }
  }
}

void PanelMenuView::OnWindowMaximized(Window xid)
{
  if (xid == active_window())
  {
    maximized_wins_.push_front(xid);
    UpdateMaximizedWindow();

    // We need to update the is_inside_ state in the case of maximization by grab
    CheckMouseInside();
    is_maximized_ = true;

    RefreshAndRedraw();
  }
  else
  {
    maximized_wins_.push_back(xid);
    UpdateMaximizedWindow();

    if (integrated_menus_ && IsWindowUnderOurControl(xid))
    {
      RefreshAndRedraw();
    }
  }
}

void PanelMenuView::OnWindowUnFullscreen(Window xid)
{
  if (!WindowManager::Default().IsWindowMaximized(xid))
    OnWindowRestored(xid);
}

void PanelMenuView::OnWindowRestored(Window xid)
{
  maximized_wins_.erase(std::remove(maximized_wins_.begin(), maximized_wins_.end(), xid), maximized_wins_.end());
  UpdateMaximizedWindow();

  if (active_window() == xid)
  {
    is_maximized_ = false;
    is_grabbed_ = false;
    RefreshAndRedraw();
  }
  else if (integrated_menus_ && window_buttons_->controlled_window == xid)
  {
    RefreshAndRedraw();
  }
}

void PanelMenuView::OnShowDesktopChanged()
{
  UpdateMaximizedWindow();
}


bool PanelMenuView::UpdateActiveWindowPosition()
{
  bool we_control_window = IsWindowUnderOurControl(active_window);

  if (we_control_window != we_control_active_)
  {
    we_control_active_ = we_control_window;

    if (HasVisibleMenus())
      on_indicator_updated.emit();

    RefreshAndRedraw();
  }

  return false;
}

void PanelMenuView::OnWindowMoved(Window xid)
{
  if (!integrated_menus_ && active_window() == xid && UScreen::GetDefault()->GetMonitors().size() > 1)
  {
    /* When moving the active window, if the current panel is controlling
     * the active window, then we postpone the timeout function every movement
     * that we have, setting a longer timeout.
     * Otherwise, if the movedPanelMenuView::OnWindowMovedPanelMenuView::OnWindowMoved window is not controlled by the current panel
     * every few millisecond we check the new window position */

    unsigned int timeout_length = 250;

    if (!we_control_active_)
    {
      if (sources_.GetSource(WINDOW_MOVED_TIMEOUT))
        return;

      timeout_length = 60;
    }

    auto cb_func = sigc::mem_fun(this, &PanelMenuView::UpdateActiveWindowPosition);
    sources_.AddTimeout(timeout_length, cb_func, WINDOW_MOVED_TIMEOUT);
  }

  if (std::find(maximized_wins_.begin(), maximized_wins_.end(), xid) != maximized_wins_.end())
    UpdateMaximizedWindow();
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

void PanelMenuView::UpdateMaximizedWindow()
{
  Window window_xid = 0;

  // Find the front-most of the maximized windows we are controlling
  for (auto xid : maximized_wins_)
  {
    // We can safely assume only the front-most is visible
    if (IsValidWindow(xid))
    {
      window_xid = xid;
      break;
    }
  }

  maximized_window = window_xid;
}

Window PanelMenuView::GetTopWindow() const
{
  Window window_xid = 0;

  for (auto const& win : ApplicationManager::Default().GetWindowsForMonitor(monitor_))
  {
    Window xid = win->window_id();

    if (win->visible() && IsValidWindow(xid))
    {
      window_xid = xid;
    }
  }

  return window_xid;
}

void PanelMenuView::ActivateIntegratedMenus(nux::Point const& click)
{
  if (!layout_->GetAbsoluteGeometry().IsInside(click))
    return;

  auto& settings = Settings::Instance();

  if (!focused && !settings.lim_unfocused_popup())
    return;

  if (settings.lim_double_click_wait() > 0)
  {
    sources_.AddTimeout(settings.lim_double_click_wait(), [this, click] {
      ActivateEntryAt(click.x, click.y);
      return false;
    }, INTEGRATED_MENUS_DOUBLE_CLICK_TIMEOUT);

    auto conn = std::make_shared<connection::Wrapper>();
    *conn = titlebar_grab_area_->mouse_double_click.connect([this, conn] (int, int, unsigned long, unsigned long) {
      sources_.Remove(INTEGRATED_MENUS_DOUBLE_CLICK_TIMEOUT);
      (*conn)->disconnect();
    });
  }
  else
  {
    ActivateEntryAt(click.x, click.y);
  }
}

void PanelMenuView::OnMaximizedActivate(int x, int y)
{
  Window maximized = maximized_window();

  if (maximized != 0)
  {
    if (maximized != active_window())
    {
      auto& wm = WindowManager::Default();
      wm.Raise(maximized);
      wm.Activate(maximized);
    }

    if (integrated_menus_)
    {
      // Adjusting the click coordinates to be absolute.
      auto const& grab_geo = titlebar_grab_area_->GetAbsoluteGeometry();
      nux::Point click(grab_geo.x + x, grab_geo.y + y);
      ActivateIntegratedMenus(click);
    }
  }
}

void PanelMenuView::MaximizedWindowWMAction(int x, int y, unsigned button)
{
  Window maximized = maximized_window();

  if (!maximized)
    return;

  using namespace decoration;
  auto& wm = WindowManager::Default();
  auto action = decoration::Style::Get()->WindowManagerAction(WMEvent(button));

  switch (action)
  {
    case WMAction::TOGGLE_SHADE:
      if (wm.IsWindowShaded(maximized))
        wm.UnShade(maximized);
      else
        wm.Shade(maximized);
      break;
    case WMAction::TOGGLE_MAXIMIZE:
      wm.Restore(maximized);
      is_inside_ = true;
      break;
    case WMAction::TOGGLE_MAXIMIZE_HORIZONTALLY:
      wm.HorizontallyMaximize(maximized);
      is_inside_ = true;
      break;
    case WMAction::TOGGLE_MAXIMIZE_VERTICALLY:
      wm.VerticallyMaximize(maximized);
      is_inside_ = true;
      break;
    case WMAction::MINIMIZE:
      wm.Minimize(maximized);
      is_inside_ = true;
      break;
    case WMAction::SHADE:
      wm.Shade(maximized);
      break;
    case WMAction::MENU:
    {
      auto const& event = nux::GetGraphicsDisplay()->GetCurrentEvent();
      auto const& abs_geo = titlebar_grab_area_->GetAbsoluteGeometry();
      int button = event.GetEventButton();
      nux::Point click(abs_geo.x + x, abs_geo.y + y);
      auto& wm = WindowManager::Default();
      wm.UnGrabMousePointer(event.x11_timestamp, button, click.x, click.y);
      wm.ShowActionMenu(event.x11_timestamp, maximized, button, click);

      is_inside_ = false;
      QueueDraw();
      break;
    }
    case WMAction::LOWER:
      wm.Lower(maximized);
      break;
    default:
      break;
  }
}

void PanelMenuView::OnMaximizedDoubleClicked(int x, int y)
{
  MaximizedWindowWMAction(x, y, 1);
}

void PanelMenuView::OnMaximizedMiddleClicked(int x, int y)
{
  MaximizedWindowWMAction(x, y, 2);
}

void PanelMenuView::OnMaximizedRightClicked(int x, int y)
{
  MaximizedWindowWMAction(x, y, 3);
}

void PanelMenuView::OnMaximizedGrabStart(int x, int y)
{
  /* When Start dragging the panelmenu of a maximized window, change cursor
   * to simulate the dragging, waiting to go out of the panel area.
   *
   * This is a workaround to avoid that the grid plugin would be fired
   * showing the window shape preview effect. See bug #838923 */

  Window maximized = maximized_window();

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

  Window maximized = maximized_window();

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
      RefreshAndRedraw();

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

  RefreshAndRedraw();
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
  .add("focused", focused())
  .add("integrated_menus", integrated_menus_)
  .add("mouse_inside", is_inside_)
  .add("always_show_menus", always_show_menus_)
  .add("grabbed", is_grabbed_)
  .add("active_win_maximized", is_maximized_)
  .add("active_win_is_desktop", is_desktop_focused_)
  .add("panel_title", panel_title_)
  .add("desktop_active", (panel_title_ == desktop_name_))
  .add("monitor", monitor_)
  .add("active_window", active_window())
  .add("maximized_window", maximized_window())
  .add("draw_menus", ShouldDrawMenus())
  .add("draw_window_buttons", ShouldDrawButtons())
  .add("controls_active_window", we_control_active_)
  .add("fadein_duration", menu_manager_->fadein())
  .add("fadeout_duration", menu_manager_->fadeout())
  .add("discovery_duration", menu_manager_->discovery())
  .add("discovery_fadein_duration", menu_manager_->discovery_fadein())
  .add("discovery_fadeout_duration", menu_manager_->discovery_fadeout())
  .add("has_menus", HasMenus())
  .add("title_geo", title_geo_);
}

void PanelMenuView::OnSwitcherShown(GVariant* data)
{
  if (!data || integrated_menus_ || always_show_menus_)
    return;

  gboolean switcher_shown;
  gint monitor;
  g_variant_get(data, "(bi)", &switcher_shown, &monitor);

  if (switcher_shown == switcher_showing_ || monitor != monitor_)
    return;

  switcher_showing_ = switcher_shown;

  if (!switcher_showing_)
  {
    CheckMouseInside();
  }
  else
  {
    show_now_activated_ = false;
  }

  RefreshAndRedraw();
}

void PanelMenuView::OnLauncherKeyNavStarted(GVariant* data)
{
  if (launcher_keynav_ || integrated_menus_)
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
  CheckMouseInside();
  RefreshAndRedraw();
}

void PanelMenuView::OnLauncherSelectionChanged(GVariant* data)
{
  if (!data || !launcher_keynav_ || integrated_menus_)
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
    sources_.AddTimeout(menu_manager_->show_menus_wait(), cb_func, UPDATE_SHOW_NOW_TIMEOUT);
  }
}

void PanelMenuView::SetMonitor(int monitor)
{
  PanelIndicatorsView::SetMonitor(monitor);

  maximized_wins_.clear();
  monitor_geo_ = UScreen::GetDefault()->GetMonitorGeometry(monitor_);

  for (auto const& win : ApplicationManager::Default().GetWindowsForMonitor(monitor_))
  {
    auto xid = win->window_id();

    if (win->active())
      active_window = xid;

    if (win->maximized() || WindowManager::Default().IsWindowFullscreen(xid))
    {
      if (win->active())
        maximized_wins_.push_front(xid);
      else
        maximized_wins_.push_back(xid);
    }
  }

  window_buttons_->monitor = monitor_;
  UpdateMaximizedWindow();

  OnStyleChanged();
}

bool PanelMenuView::HasMenus() const
{
  if (!HasVisibleMenus())
    return false;

  return integrated_menus_ || we_control_active_;
}

bool PanelMenuView::HasKeyActivableMenus() const
{
  if (!HasVisibleMenus())
    return false;

  return integrated_menus_ ? is_maximized_ : we_control_active_;
}

bool PanelMenuView::GetControlsActive() const
{
  return we_control_active_;
}

bool PanelMenuView::CheckMouseInside()
{
  if (always_show_menus_)
    return is_inside_;

  auto const& mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  is_inside_ = GetAbsoluteGeometry().IsInside(mouse);

  return is_inside_;
}

void PanelMenuView::OnPanelViewMouseEnter(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state)
{
  if (always_show_menus_)
    return;

  if (!is_inside_)
  {
    if (is_grabbed_)
      is_grabbed_ = false;
    else
      is_inside_ = true;

    FullRedraw();
  }
}

void PanelMenuView::IgnoreLeaveEvents(bool ignore)
{
  ignore_leave_events_ = ignore;
}

void PanelMenuView::OnPanelViewMouseLeave(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state)
{
  if (always_show_menus_ || ignore_leave_events_)
    return;

  if (is_inside_)
  {
    is_inside_ = false;
    FullRedraw();
  }
}

void PanelMenuView::SetMousePosition(int x, int y)
{
  if (always_show_menus_)
    return;

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

} // namespace panel
} // namespace unity
