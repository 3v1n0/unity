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

#include "unity-shared/CairoTexture.h"
#include "PanelMenuView.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

#include <UnityCore/Variant.h>

#include <glib/gi18n-lib.h>

namespace unity
{

namespace
{
  nux::logging::Logger logger("unity.panel.menu");
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
  : _matcher(bamf_matcher_get_default()),
    _is_inside(false),
    _is_grabbed(false),
    _is_maximized(false),
    _last_active_view(nullptr),
    _new_application(nullptr),
    _overlay_showing(false),
    _switcher_showing(false),
    _launcher_keynav(false),
    _show_now_activated(false),
    _we_control_active(false),
    _new_app_menu_shown(false),
    _monitor(0),
    _active_xid(0),
    _desktop_name(_("Ubuntu Desktop")),
    _menus_fadein(DEFAULT_MENUS_FADEIN),
    _menus_fadeout(DEFAULT_MENUS_FADEOUT),
    _menus_discovery(DEFAULT_MENUS_DISCOVERY),
    _menus_discovery_fadein(DEFAULT_DISCOVERY_FADEIN),
    _menus_discovery_fadeout(DEFAULT_DISCOVERY_FADEOUT),
    _fade_in_animator(_menus_fadein),
    _fade_out_animator(_menus_fadeout)
{
  layout_->SetContentDistribution(nux::MAJOR_POSITION_START);

  BamfWindow* active_win = bamf_matcher_get_active_window(_matcher);
  if (BAMF_IS_WINDOW(active_win))
    _active_xid = bamf_window_get_xid(active_win);

  _view_opened_signal.Connect(_matcher, "view-opened",
                              sigc::mem_fun(this, &PanelMenuView::OnViewOpened));
  _view_closed_signal.Connect(_matcher, "view-closed",
                              sigc::mem_fun(this, &PanelMenuView::OnViewClosed));
  _active_win_changed_signal.Connect(_matcher, "active-window-changed",
                                     sigc::mem_fun(this, &PanelMenuView::OnActiveWindowChanged));
  _active_app_changed_signal.Connect(_matcher, "active-application-changed",
                                     sigc::mem_fun(this, &PanelMenuView::OnActiveAppChanged));

  _window_buttons = new WindowButtons();
  _window_buttons->SetParentObject(this);
  _window_buttons->SetMonitor(_monitor);
  _window_buttons->SetControlledWindow(_active_xid);
  _window_buttons->SetLeftAndRightPadding(MAIN_LEFT_PADDING, MENUBAR_PADDING);
  _window_buttons->SetMaximumHeight(panel::Style::Instance().panel_height);
  _window_buttons->ComputeContentSize();

  _window_buttons->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  _window_buttons->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));
  AddChild(_window_buttons.GetPointer());

  layout_->SetLeftAndRightPadding(_window_buttons->GetContentWidth(), 0);
  layout_->SetBaseHeight(panel::Style::Instance().panel_height);

  _titlebar_grab_area = new PanelTitlebarGrabArea();
  _titlebar_grab_area->SetParentObject(this);
  _titlebar_grab_area->activate_request.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedActivate));
  _titlebar_grab_area->restore_request.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedRestore));
  _titlebar_grab_area->lower_request.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedLower));
  _titlebar_grab_area->grab_started.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabStart));
  _titlebar_grab_area->grab_move.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabMove));
  _titlebar_grab_area->grab_end.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabEnd));
  AddChild(_titlebar_grab_area.GetPointer());

  WindowManager& wm = WindowManager::Default();
  wm.window_minimized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMinimized));
  wm.window_unminimized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUnminimized));
  wm.window_maximized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMaximized));
  wm.window_restored.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowRestored));
  wm.window_unmapped.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUnmapped));
  wm.window_mapped.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMapped));
  wm.window_moved.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMoved));
  wm.window_resized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMoved));
  wm.window_decorated.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowDecorated));
  wm.window_undecorated.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUndecorated));
  wm.initiate_spread.connect(sigc::mem_fun(this, &PanelMenuView::OnSpreadInitiate));
  wm.terminate_spread.connect(sigc::mem_fun(this, &PanelMenuView::OnSpreadTerminate));
  wm.initiate_expo.connect(sigc::mem_fun(this, &PanelMenuView::OnExpoInitiate));
  wm.terminate_expo.connect(sigc::mem_fun(this, &PanelMenuView::OnExpoTerminate));
  wm.screen_viewport_switch_ended.connect(sigc::mem_fun(this, &PanelMenuView::OnExpoTerminate));

  _style_changed_connection = panel::Style::Instance().changed.connect([&] {
    _window_buttons->ComputeContentSize();
    layout_->SetLeftAndRightPadding(_window_buttons->GetContentWidth(), 0);

    Refresh(true);
    FullRedraw();
  });

  mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));
  //mouse_move.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseMove));

  _titlebar_grab_area->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  _titlebar_grab_area->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));

  _ubus_manager.RegisterInterest(UBUS_SWITCHER_SHOWN, sigc::mem_fun(this, &PanelMenuView::OnSwitcherShown));

  _ubus_manager.RegisterInterest(UBUS_LAUNCHER_START_KEY_NAV, sigc::mem_fun(this, &PanelMenuView::OnLauncherKeyNavStarted));
  _ubus_manager.RegisterInterest(UBUS_LAUNCHER_END_KEY_NAV, sigc::mem_fun(this, &PanelMenuView::OnLauncherKeyNavEnded));
  _ubus_manager.RegisterInterest(UBUS_LAUNCHER_START_KEY_SWITCHER, sigc::mem_fun(this, &PanelMenuView::OnLauncherKeyNavStarted));
  _ubus_manager.RegisterInterest(UBUS_LAUNCHER_END_KEY_SWITCHER, sigc::mem_fun(this, &PanelMenuView::OnLauncherKeyNavEnded));
  _ubus_manager.RegisterInterest(UBUS_LAUNCHER_SELECTION_CHANGED, sigc::mem_fun(this, &PanelMenuView::OnLauncherSelectionChanged));

  _fade_in_animator.animation_updated.connect(sigc::mem_fun(this, &PanelMenuView::OnFadeInChanged));
  _fade_in_animator.animation_ended.connect(sigc::mem_fun(this, &PanelMenuView::FullRedraw));
  _fade_out_animator.animation_updated.connect(sigc::mem_fun(this, &PanelMenuView::OnFadeOutChanged));
  _fade_out_animator.animation_ended.connect(sigc::mem_fun(this, &PanelMenuView::FullRedraw));

  SetOpacity(0.0f);
  _window_buttons->SetOpacity(0.0f);

  Refresh();
  FullRedraw();
}

PanelMenuView::~PanelMenuView()
{
  _style_changed_connection.disconnect();
  _window_buttons->UnParentObject();
  _titlebar_grab_area->UnParentObject();
}

void PanelMenuView::OverlayShown()
{
  _overlay_showing = true;
  QueueDraw();
}

void PanelMenuView::OverlayHidden()
{
  _overlay_showing = false;
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
  {
    _menus_fadein = fadein;
    _fade_in_animator.SetDuration(_menus_fadein);
  }

  if (fadeout > -1)
  {
    _menus_fadeout = fadeout;
    _fade_out_animator.SetDuration(_menus_fadeout);
  }

  if (discovery > -1)
    _menus_discovery = discovery;

  if (discovery_fadein > -1)
    _menus_discovery_fadein = discovery_fadein;

  if (discovery_fadeout > -1)
    _menus_discovery_fadeout = discovery_fadeout;
}

void PanelMenuView::FullRedraw()
{
  QueueDraw();
  _window_buttons->QueueDraw();
}

nux::Area* PanelMenuView::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  bool mouse_inside = TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type);

  if (!mouse_inside)
    return nullptr;

  Area* found_area = nullptr;

  if (_overlay_showing)
  {
    if (_window_buttons)
      return _window_buttons->FindAreaUnderMouse(mouse_position, event_type);
  }

  if (!_we_control_active)
  {
    /* When the current panel is not active, it all behaves like a grab-area */
    if (GetAbsoluteGeometry().IsInside(mouse_position))
      return _titlebar_grab_area.GetPointer();
  }

  if (_is_maximized)
  {
    if (_window_buttons)
    {
      found_area = _window_buttons->FindAreaUnderMouse(mouse_position, event_type);
      NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
    }
  }

  if (_titlebar_grab_area && !_overlay_showing)
  {
    found_area = _titlebar_grab_area->FindAreaUnderMouse(mouse_position, event_type);
    NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
  }

  return PanelIndicatorsView::FindAreaUnderMouse(mouse_position, event_type);
}

void PanelMenuView::PreLayoutManagement()
{
  PanelIndicatorsView::PreLayoutManagement();
  nux::Geometry const& geo = GetGeometry();

  _window_buttons->ComputeContentSize();
  int buttons_diff = geo.height - _window_buttons->GetContentHeight();
  _window_buttons->SetBaseY(buttons_diff > 0 ? std::ceil(buttons_diff/2.0f) : 0);

  layout_->ComputeContentSize();
  int layout_width = layout_->GetContentWidth();

  _titlebar_grab_area->SetBaseX(layout_width);
  _titlebar_grab_area->SetBaseHeight(geo.height);
  _titlebar_grab_area->SetMinimumWidth(geo.width - layout_width);
  _titlebar_grab_area->SetMaximumWidth(geo.width - layout_width);

  SetMaximumEntriesWidth(geo.width - _window_buttons->GetContentWidth());
}

void PanelMenuView::OnFadeInChanged(double opacity)
{
  if (DrawMenus() && GetOpacity() != 1.0f)
    SetOpacity(opacity);

  if (DrawWindowButtons() && _window_buttons->GetOpacity() != 1.0f)
    _window_buttons->SetOpacity(opacity);

  QueueDraw();
}

void PanelMenuView::OnFadeOutChanged(double progress)
{
  double opacity = CLAMP(1.0f - progress, 0.0f, 1.0f);

  if (!DrawMenus() && GetOpacity() != 0.0f)
    SetOpacity(opacity);

  if (!DrawWindowButtons() && _window_buttons->GetOpacity() != 0.0f)
    _window_buttons->SetOpacity(opacity);

  QueueDraw();
}

bool PanelMenuView::DrawMenus() const
{
  WindowManager& wm = WindowManager::Default();
  bool screen_grabbed = (wm.IsExpoActive() || wm.IsScaleActive());

  if (_we_control_active && !_overlay_showing && !screen_grabbed &&
      !_switcher_showing && !_launcher_keynav)
  {
    if (_is_inside || _last_active_view || _show_now_activated || _new_application)
    {
      return true;
    }
  }

  return false;
}

bool PanelMenuView::DrawWindowButtons() const
{
  WindowManager& wm = WindowManager::Default();
  bool screen_grabbed = (wm.IsExpoActive() || wm.IsScaleActive());

  if (_overlay_showing)
    return true;

  if (_we_control_active && _is_maximized && !screen_grabbed &&
      !_launcher_keynav && !_switcher_showing)
  {
    if (_is_inside || _show_now_activated || _new_application)
    {
      return true;
    }
  }

  return false;
}

void PanelMenuView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  int button_width = _window_buttons->GetContentWidth();
  const float factor = 4;
  button_width /= factor;

  if (geo != _last_geo)
  {
    _last_geo = geo;
    QueueRelayout();
    Refresh(true);
  }

  GfxContext.PushClippingRectangle(geo);

  /* "Clear" out the background */
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::ColorLayer layer(nux::Color(0x00000000), true, rop);
  nux::GetPainter().PushDrawLayer(GfxContext, GetGeometry(), &layer);

  if (_title_texture)
  {
    guint blend_alpha = 0, blend_src = 0, blend_dest = 0;
    bool draw_menus = DrawMenus();
    bool draw_window_buttons = DrawWindowButtons();
    bool has_menu = false;
    bool draw_faded_title = false;

    GfxContext.GetRenderStates().GetBlend(blend_alpha, blend_src, blend_dest);

    for (auto entry : entries_)
    {
      if (entry.second->IsVisible())
      {
        has_menu = true;
        break;
      }
    }

    if (!draw_window_buttons && _we_control_active && has_menu &&
        (draw_menus || (GetOpacity() > 0.0f && _window_buttons->GetOpacity() == 0.0f)))
    {
      draw_faded_title = true;
    }

    if (draw_faded_title)
    {
      bool build_gradient = false;
      nux::SURFACE_LOCKED_RECT lockrect;
      lockrect.pBits = 0;
      bool locked = false;

      if (_gradient_texture.IsNull() || (_gradient_texture->GetWidth() != geo.width))
      {
        build_gradient = true;
      }
      else
      {
        if (_gradient_texture->LockRect(0, &lockrect, nullptr) != OGL_OK)
          build_gradient = true;
        else
          locked = true;

        if (!lockrect.pBits)
        {
          build_gradient = true;

          if (locked)
            _gradient_texture->UnlockRect(0);
        }
      }

      if (build_gradient)
      {
        nux::NTextureData texture_data(nux::BITFMT_R8G8B8A8, geo.width, 1, 1);

        _gradient_texture = nux::GetGraphicsDisplay()->GetGpuDevice()->
                            CreateSystemCapableDeviceTexture(texture_data.GetWidth(),
                            texture_data.GetHeight(), 1, texture_data.GetFormat());
        locked = (_gradient_texture->LockRect(0, &lockrect, nullptr) == OGL_OK);
      }

      BYTE* dest_buffer = (BYTE*) lockrect.pBits;
      int gradient_opacity = 255.0f * GetOpacity();
      int buttons_opacity = 255.0f * _window_buttons->GetOpacity();

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
          a = 0xff - gradient_opacity * (((float)x - (first_step)) /
                                         (float)(button_width));
        }
        else
        {
          if (!draw_menus)
          {
            a = 0xff - gradient_opacity;
          }
          else
          {
            // If we're fading-out the title, it's better to quickly hide
            // the transparent right-most area
            a = CLAMP(0xff - gradient_opacity - 0x55, 0x00, 0xff);
          }
        }

        *(dest_buffer + 4 * x + 0) = (r * a) / 0xff; //red
        *(dest_buffer + 4 * x + 1) = (g * a) / 0xff; //green
        *(dest_buffer + 4 * x + 2) = (b * a) / 0xff; //blue
        *(dest_buffer + 4 * x + 3) = a;
      }

      // FIXME Nux shouldn't make unity to crash if we try to unlock a wrong rect
      if (locked)
        _gradient_texture->UnlockRect(0);

      GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      nux::TexCoordXForm texxform0;
      nux::TexCoordXForm texxform1;

      // Modulate the checkboard and the gradient texture
      GfxContext.QRP_2TexMod(geo.x, geo.y,
                             geo.width, geo.height,
                             _gradient_texture, texxform0,
                             nux::color::White,
                             _title_texture->GetDeviceTexture(),
                             texxform1,
                             nux::color::White);
    }
    else if (!_overlay_showing)
    {
      double title_opacity = 0.0f;

      if (_we_control_active && _window_buttons->GetOpacity() == 0.0 &&
          (!has_menu || (has_menu && GetOpacity() == 0.0)))
      {
        title_opacity = 1.0f;
      }
      else
      {
        title_opacity = 1.0f;

        if (has_menu)
          title_opacity -= MAX(GetOpacity(), _window_buttons->GetOpacity());
        else
          title_opacity -= _window_buttons->GetOpacity();

        if (!draw_window_buttons && !draw_menus)
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

      if (title_opacity > 0.0f)
      {
        nux::TexCoordXForm texxform;
        GfxContext.QRP_1Tex(geo.x, geo.y, geo.width, geo.height,
                            _title_texture->GetDeviceTexture(), texxform,
                            nux::color::White * title_opacity);
      }
    }

    GfxContext.GetRenderStates().SetBlend(blend_alpha, blend_src, blend_dest);
  }

  nux::GetPainter().PopBackground();

  GfxContext.PopClippingRectangle();
}

void PanelMenuView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  bool draw_menus = DrawMenus();
  bool draw_buttons = DrawWindowButtons();

  GfxContext.PushClippingRectangle(geo);

  if (draw_menus)
  {
    for (auto entry : entries_)
      entry.second->SetDisabled(false);

    layout_->ProcessDraw(GfxContext, true);

    _fade_out_animator.Stop();

    if (_new_application && !_is_inside)
    {
      _fade_in_animator.Start(_menus_discovery_fadein, GetOpacity());
    }
    else
    {
      _fade_in_animator.Start(GetOpacity());
      _new_app_menu_shown = false;
    }
  }
  else
  {
    for (auto entry : entries_)
      entry.second->SetDisabled(true);
  }

  if (GetOpacity() != 0.0f && !draw_menus && !_overlay_showing)
  {
    layout_->ProcessDraw(GfxContext, true);

    _fade_in_animator.Stop();

    if (!_new_app_menu_shown)
    {
      _fade_out_animator.Start(1.0f - GetOpacity());
    }
    else
    {
      _fade_out_animator.Start(_menus_discovery_fadeout, 1.0f - GetOpacity());
    }
  }

  if (draw_buttons)
  {
    _window_buttons->ProcessDraw(GfxContext, true);

    if (_window_buttons->GetOpacity() != 1.0f)
    {
      _fade_out_animator.Stop();
      _fade_in_animator.Start(_window_buttons->GetOpacity());
    }
  }

  if (_window_buttons->GetOpacity() != 0.0f && !draw_buttons)
  {
    _window_buttons->ProcessDraw(GfxContext, true);
    _fade_in_animator.Stop();

    /* If we try to hide only the buttons, then use a faster fadeout */
    if (!_fade_out_animator.IsRunning())
    {
      _fade_out_animator.Start(_menus_fadeout/3, 1.0f - _window_buttons->GetOpacity());
    }
  }

  GfxContext.PopClippingRectangle();
}

std::string PanelMenuView::GetActiveViewName(bool use_appname) const
{
  std::string label;
  BamfWindow* window;

  window = bamf_matcher_get_active_window(_matcher);

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
      label = _desktop_name;
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
      app = bamf_matcher_get_application_for_window(_matcher, window);

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

void PanelMenuView::DrawTitle(cairo_t *cr_real, nux::Geometry const& geo, std::string const& label) const
{
  using namespace panel;
  cairo_t* cr;
  cairo_pattern_t* linpat;
  int x = MAIN_LEFT_PADDING + TITLE_PADDING + geo.x;
  int y = geo.y;

  int text_width = 0;
  int text_height = 0;
  int text_space = 0;

  // Find out dimensions first
  GdkScreen* screen = gdk_screen_get_default();
  PangoContext* cxt;
  PangoRectangle log_rect;
  PangoFontDescription* desc;

  nux::CairoGraphics util_cg(CAIRO_FORMAT_ARGB32, 1, 1);
  cr = util_cg.GetContext();

  int dpi = Style::Instance().GetTextDPI();

  std::string font_description(Style::Instance().GetFontDescription(PanelItem::TITLE));
  desc = pango_font_description_from_string(font_description.c_str());

  glib::Object<PangoLayout> layout(pango_cairo_create_layout(cr));
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_markup(layout, label.c_str(), -1);

  cxt = pango_layout_get_context(layout);
  pango_cairo_context_set_font_options(cxt, gdk_screen_get_font_options(screen));
  pango_cairo_context_set_resolution(cxt, dpi / static_cast<float>(PANGO_SCALE));
  pango_layout_context_changed(layout);

  pango_layout_get_extents(layout, nullptr, &log_rect);
  text_width = log_rect.width / PANGO_SCALE;
  text_height = log_rect.height / PANGO_SCALE;

  pango_font_description_free(desc);
  cairo_destroy(cr);

  // Draw the text
  GtkStyleContext* style_context = Style::Instance().GetStyleContext();
  text_space = geo.width - x;
  cr = cr_real;
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  gtk_style_context_save(style_context);

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);
  gtk_widget_path_iter_set_name(widget_path, -1 , "UnityPanelWidget");

  gtk_style_context_set_path(style_context, widget_path);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

  y += (geo.height - text_height) / 2;

  pango_cairo_update_layout(cr, layout);

  if (text_width > text_space)
  {
    int out_pixels = text_width - text_space;
    const int fading_pixels = 35;
    int fading_width = out_pixels < fading_pixels ? out_pixels : fading_pixels;

    cairo_push_group(cr);
    gtk_render_layout(style_context, cr, x, y, layout);
    cairo_pop_group_to_source(cr);

    linpat = cairo_pattern_create_linear(geo.width - fading_width, y, geo.width, y);
    cairo_pattern_add_color_stop_rgba(linpat, 0, 0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba(linpat, 1, 0, 0, 0, 0);
    cairo_mask(cr, linpat);

    cairo_pattern_destroy(linpat);
  }
  else
  {
    gtk_render_layout(style_context, cr, x, y, layout);
  }

  x += text_width;

  gtk_widget_path_free(widget_path);
  gtk_style_context_restore(style_context);
}

void PanelMenuView::Refresh(bool force)
{
  nux::Geometry const& geo = GetGeometry();

  // We can get into a race that causes the geometry to be wrong as there hasn't been a
  // layout cycle before the first callback. This is to protect from that.
  if (geo.width > _monitor_geo.width)
    return;

  WindowManager& wm = WindowManager::Default();
  std::string new_title;

  if (wm.IsScaleActive())
  {
    if (wm.IsScaleActiveForGroup())
      new_title = GetActiveViewName(true);
    else if (_we_control_active)
      new_title = _desktop_name;
  }
  else if (wm.IsExpoActive())
  {
    new_title = _desktop_name;
  }
  else if (!_we_control_active)
  {
    new_title = "";
  }
  else if (!_switcher_showing && !_launcher_keynav)
  {
    new_title = GetActiveViewName();
    _window_buttons->SetControlledWindow(_active_xid);
  }

  if (!_switcher_showing && !_launcher_keynav)
  {
    if (_panel_title != new_title)
    {
      _panel_title = new_title;
    }
    else if (!force && _last_geo == geo && _title_texture)
    {
      // No need to redraw the title, let's save some CPU time!
      return;
    }
  }

  if (_panel_title.empty())
  {
    _title_texture = nullptr;
    return;
  }

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, geo.width, geo.height);
  cairo_t* cr = cairo_graphics.GetContext();

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  glib::String escaped(g_markup_escape_text(_panel_title.c_str(), -1));

  std::ostringstream bold_label;
  bold_label << "<b>" << escaped.Str() << "</b>";

  DrawTitle(cr, geo, bold_label.str());

  cairo_destroy(cr);

  _title_texture = texture_ptr_from_cairo_graphics(cairo_graphics);
}

void PanelMenuView::OnActiveChanged(PanelIndicatorEntryView* view, bool is_active)
{
  if (is_active)
  {
    _last_active_view = view;
  }
  else
  {
    if (_last_active_view == view)
    {
      _last_active_view = nullptr;
    }
  }

  Refresh();
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
  _last_active_view = nullptr;

  auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  _is_inside = GetAbsoluteGeometry().IsInside(mouse);
  FullRedraw();
}

void PanelMenuView::OnNameChanged(BamfView* bamf_view, gchar* new_name, gchar* old_name)
{
  Refresh();
  FullRedraw();
}

bool PanelMenuView::OnNewAppShow()
{
  BamfApplication* active_app = bamf_matcher_get_active_application(_matcher);
  _new_application = glib::Object<BamfApplication>(active_app, glib::AddRef());
  QueueDraw();

  if (_sources.GetSource(NEW_APP_HIDE_TIMEOUT))
  {
    _new_app_menu_shown = false;
  }

  auto cb_func = sigc::mem_fun(this, &PanelMenuView::OnNewAppHide);
  _sources.AddTimeoutSeconds(_menus_discovery, cb_func, NEW_APP_HIDE_TIMEOUT);

  return false;
}

bool PanelMenuView::OnNewAppHide()
{
  OnApplicationClosed(_new_application);
  _new_app_menu_shown = true;
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

  _new_apps.push_front(glib::Object<BamfApplication>(BAMF_APPLICATION(view), glib::AddRef()));
}

void PanelMenuView::OnApplicationClosed(BamfApplication* app)
{
  if (BAMF_IS_APPLICATION(app))
  {
    if (std::find(_new_apps.begin(), _new_apps.end(), app) != _new_apps.end())
    {
      _new_apps.remove(glib::Object<BamfApplication>(app, glib::AddRef()));
    }
    else if (_new_apps.empty())
    {
      _new_application = nullptr;
    }
  }

  if (app == _new_application)
  {
    _new_application = nullptr;
  }
}

void PanelMenuView::OnViewClosed(BamfMatcher *matcher, BamfView *view)
{
  if (reinterpret_cast<BamfView*>(_view_name_changed_signal.object()) == view)
  {
    _view_name_changed_signal.Disconnect();
  }

  if (BAMF_IS_APPLICATION(view))
  {
    OnApplicationClosed(reinterpret_cast<BamfApplication*>(view));
  }
  else if (reinterpret_cast<BamfApplication*>(view) == _new_application)
  {
    _new_application = nullptr;
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
    if (std::find(_new_apps.begin(), _new_apps.end(), new_app) != _new_apps.end())
    {
      if (_new_application != new_app)
      {
        /* Add a small delay before showing the menus, this is done both
         * to fix the issues with applications that takes some time to loads
         * menus and to show the menus only when an application has been
         * kept active for some time */

        auto cb_func = sigc::mem_fun(this, &PanelMenuView::OnNewAppShow);
        _sources.AddTimeout(300, cb_func, NEW_APP_SHOW_TIMEOUT);
      }
    }
    else
    {
      _sources.Remove(NEW_APP_SHOW_TIMEOUT);

      if (_sources.GetSource(NEW_APP_HIDE_TIMEOUT))
      {
        _sources.Remove(NEW_APP_HIDE_TIMEOUT);
        _new_app_menu_shown = false;
      }

      if (_new_application)
        OnApplicationClosed(_new_application);
    }
  }
}

void PanelMenuView::OnActiveWindowChanged(BamfMatcher *matcher,
                                          BamfView* old_view,
                                          BamfView* new_view)
{
  _show_now_activated = false;
  _is_maximized = false;
  _active_xid = 0;

  _sources.Remove(WINDOW_MOVED_TIMEOUT);

  if (BAMF_IS_WINDOW(new_view))
  {
    WindowManager& wm = WindowManager::Default();
    BamfWindow* window = reinterpret_cast<BamfWindow*>(new_view);
    guint32 xid = bamf_window_get_xid(window);
    _active_xid = xid;
    _is_maximized = wm.IsWindowMaximized(xid);

    if (bamf_window_get_window_type(window) == BAMF_WINDOW_DESKTOP)
      _we_control_active = true;
    else
      _we_control_active = IsWindowUnderOurControl(xid);

    if (_decor_map.find(xid) == _decor_map.end())
    {
      _decor_map[xid] = true;

      // if we've just started tracking this window and it is maximized, let's
      // make sure it's undecorated just in case it slipped by us earlier
      // (I'm looking at you, Chromium!)
      if (_is_maximized && wm.IsWindowDecorated(xid))
      {
        wm.Undecorate(xid);
        _maximized_set.insert(xid);
      }
    }

    // first see if we need to remove and old callback
    _view_name_changed_signal.Disconnect();

    // register callback for new view
    _view_name_changed_signal.Connect(new_view, "name-changed",
                                      sigc::mem_fun(this, &PanelMenuView::OnNameChanged));

    _window_buttons->SetControlledWindow(_is_maximized ? _active_xid : 0);
  }

  Refresh();
  FullRedraw();
}

void PanelMenuView::OnSpreadInitiate()
{
  Refresh();
  QueueDraw();
}

void PanelMenuView::OnSpreadTerminate()
{
  Refresh();
  QueueDraw();
}

void PanelMenuView::OnExpoInitiate()
{
  Refresh();
  QueueDraw();
}

void PanelMenuView::OnExpoTerminate()
{
  Refresh();
  QueueDraw();
}

void PanelMenuView::OnWindowMinimized(guint32 xid)
{
  WindowManager& wm = WindowManager::Default();
  if (wm.IsWindowMaximized(xid))
  {
    wm.Decorate(xid);
    _maximized_set.erase(xid);

    Refresh();
    QueueDraw();
  }
}

void PanelMenuView::OnWindowUnminimized(guint32 xid)
{
  WindowManager& wm = WindowManager::Default();
  if (wm.IsWindowMaximized(xid))
  {
    wm.Undecorate(xid);
    _maximized_set.insert(xid);

    Refresh();
    QueueDraw();
  }
}

void PanelMenuView::OnWindowUnmapped(guint32 xid)
{
  // FIXME: compiz doesn't give us a valid xid (is always 0 on unmap)
  // we need to do this again on BamfView closed signal.
  if (_maximized_set.find(xid) != _maximized_set.end())
  {
    WindowManager::Default().Decorate(xid);
    _maximized_set.erase(xid);
    _decor_map.erase(xid);

    Refresh();
    QueueDraw();
  }
}

void PanelMenuView::OnWindowMapped(guint32 xid)
{
  WindowManager& wm = WindowManager::Default();
  if (wm.IsWindowMaximized(xid))
  {
    wm.Undecorate(xid);
    _maximized_set.insert(xid);

    Refresh();
    QueueDraw();
  }
}

void PanelMenuView::OnWindowDecorated(guint32 xid)
{
  _decor_map[xid] = true;

  if (_maximized_set.find(xid) != _maximized_set.end ())
  {
    WindowManager::Default().Undecorate(xid);
  }
}

void PanelMenuView::OnWindowUndecorated(guint32 xid)
{
  _decor_map[xid] = false;
}

void PanelMenuView::OnWindowMaximized(guint xid)
{
  bool updated = false;
  bool is_active = (_active_xid == xid);

  if (is_active)
  {
    // We need to update the _is_inside state in the case of maximization by grab
    auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
    _is_inside = GetAbsoluteGeometry().IsInside(mouse);

    _is_maximized = true;
    updated = true;
  }

  // update the state of the window in the _decor_map
  WindowManager& wm = WindowManager::Default();
  _decor_map[xid] = wm.IsWindowDecorated(xid);

  if (_decor_map[xid])
    wm.Undecorate(xid);

  _maximized_set.insert(xid);

  if (updated)
  {
    Refresh();
    FullRedraw();
  }
}

void PanelMenuView::OnWindowRestored(guint xid)
{
  if (_maximized_set.find(xid) == _maximized_set.end())
    return;

  if (_active_xid == xid)
  {
    _is_maximized = false;
    _is_grabbed = false;
  }

  if (_decor_map[xid])
    WindowManager::Default().Decorate(xid);

  _maximized_set.erase(xid);

  Refresh();
  FullRedraw();
}

bool PanelMenuView::UpdateActiveWindowPosition()
{
  bool we_control_window = IsWindowUnderOurControl(_active_xid);

  if (we_control_window != _we_control_active)
  {
    _we_control_active = we_control_window;

    Refresh();
    QueueDraw();
  }

  return false;
}

void PanelMenuView::OnWindowMoved(guint xid)
{
  if (_active_xid == xid)
  {
    /* When moving the active window, if the current panel is controlling
     * the active window, then we postpone the timeout function every movement
     * that we have, setting a longer timeout.
     * Otherwise, if the moved window is not controlled by the current panel
     * every few millisecond we check the new window position */

    unsigned int timeout_length = 250;

    if (_we_control_active)
    {
      _sources.Remove(WINDOW_MOVED_TIMEOUT);
    }
    else
    {
      if (_sources.GetSource(WINDOW_MOVED_TIMEOUT))
        return;

      timeout_length = 60;
    }

    auto cb_func = sigc::mem_fun(this, &PanelMenuView::UpdateActiveWindowPosition);
    _sources.AddTimeout(timeout_length, cb_func, WINDOW_MOVED_TIMEOUT);
  }
}

bool PanelMenuView::IsWindowUnderOurControl(Window xid) const
{
  if (UScreen::GetDefault()->GetMonitors().size() > 1)
  {
    WindowManager& wm = WindowManager::Default();
    nux::Geometry const& window_geo = wm.GetWindowGeometry(xid);
    nux::Geometry const& intersect = _monitor_geo.Intersect(window_geo);

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
  for (auto xid : _maximized_set)
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
  GList* windows = bamf_matcher_get_window_stack_for_monitor(_matcher, _monitor);

  for (GList* l = windows; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    Window xid = bamf_window_get_xid(static_cast<BamfWindow*>(l->data));
    bool visible = bamf_view_user_visible(static_cast<BamfView*>(l->data));

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
    GList* windows = bamf_matcher_get_windows(_matcher);

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
  if (_overlay_showing)
    return;

  Window maximized = GetMaximizedWindow();

  if (maximized != 0)
  {
    WindowManager::Default().Restore(maximized);
    _is_inside = true;
  }
}

void PanelMenuView::OnMaximizedLower(int x, int y)
{
  if (_overlay_showing)
    return;

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
    _titlebar_grab_area->SetGrabbed(true);
  }
}

void PanelMenuView::OnMaximizedGrabMove(int x, int y)
{
  auto panel = static_cast<nux::BaseWindow*>(GetTopLevelViewWindow());

  if (!panel)
    return;

  /* Adjusting the x, y coordinates to get the absolute values */
  x += _titlebar_grab_area->GetAbsoluteX();
  y += _titlebar_grab_area->GetAbsoluteY();

  Window maximized = GetMaximizedWindow();

  /* When the drag goes out from the Panel, start the real movement.
   *
   * This is a workaround to avoid that the grid plugin would be fired
   * showing the window shape preview effect. See bug #838923 */
  if (maximized != 0 && panel)
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
      int restore_x = x - (restored_geo.width * x / panel_geo.width);
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

      _is_inside = true;
      _is_grabbed = true;
      Refresh();
      FullRedraw();

      /* Ungrab the pointer and start the X move, to make the decorator handle it */
      _titlebar_grab_area->SetGrabbed(false);
      wm.StartMove(maximized, x, y);
    }
  }
}

void PanelMenuView::OnMaximizedGrabEnd(int x, int y)
{
  _titlebar_grab_area->SetGrabbed(false);

  x += _titlebar_grab_area->GetAbsoluteX();
  y += _titlebar_grab_area->GetAbsoluteY();
  _is_inside = GetAbsoluteGeometry().IsPointInside(x, y);

  if (!_is_inside)
    _is_grabbed = false;

  Refresh();
  FullRedraw();
}

// Introspectable
std::string
PanelMenuView::GetName() const
{
  return "MenuView";
}

void PanelMenuView::AddProperties(GVariantBuilder* builder)
{
  PanelIndicatorsView::AddProperties(builder);

  variant::BuilderWrapper(builder)
  .add("mouse_inside", _is_inside)
  .add("grabbed", _is_grabbed)
  .add("active_win_maximized", _is_maximized)
  .add("panel_title", _panel_title)
  .add("desktop_active", (_panel_title == _desktop_name))
  .add("monitor", _monitor)
  .add("active_window", _active_xid)
  .add("draw_menus", DrawMenus())
  .add("draw_window_buttons", DrawWindowButtons())
  .add("controls_active_window", _we_control_active)
  .add("fadein_duration", _menus_fadein)
  .add("fadeout_duration", _menus_fadeout)
  .add("discovery_duration", _menus_discovery)
  .add("discovery_fadein_duration", _menus_discovery_fadein)
  .add("discovery_fadeout_duration", _menus_discovery_fadeout);
}

void PanelMenuView::OnSwitcherShown(GVariant* data)
{
  if (!data)
    return;

  gboolean switcher_shown;
  gint monitor;
  g_variant_get(data, "(bi)", &switcher_shown, &monitor);

  if (switcher_shown == _switcher_showing || monitor != _monitor)
    return;

  _switcher_showing = switcher_shown;

  if (!_switcher_showing)
  {
    auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
    _is_inside = GetAbsoluteGeometry().IsInside(mouse);
  }
  else
  {
    _show_now_activated = false;
  }

  Refresh();
  QueueDraw();
}

void PanelMenuView::OnLauncherKeyNavStarted(GVariant* data)
{
  if (_launcher_keynav)
    return;


  if (!data || (data && g_variant_get_int32(data) == _monitor))
  {
    _launcher_keynav = true;
  }
}

void PanelMenuView::OnLauncherKeyNavEnded(GVariant* data)
{
  if (!_launcher_keynav)
    return;

  _launcher_keynav = false;

  auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  _is_inside = GetAbsoluteGeometry().IsInside(mouse);

  Refresh();
  QueueDraw();
}

void PanelMenuView::OnLauncherSelectionChanged(GVariant* data)
{
  if (!data || !_launcher_keynav)
    return;

  const gchar *title = g_variant_get_string(data, 0);
  _panel_title = (title ? title : "");

  Refresh();
  QueueDraw();
}

bool PanelMenuView::UpdateShowNowWithDelay()
{
  bool active = false;

  for (auto entry : entries_)
  {
    if (entry.second->GetShowNow())
    {
      active = true;
      break;
    }
  }

  if (active)
  {
    _show_now_activated = true;
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

   _sources.Remove(UPDATE_SHOW_NOW_TIMEOUT);

  if (!status && _show_now_activated)
  {
    _show_now_activated = false;
    QueueDraw();
    return;
  }

  if (status && !_show_now_activated)
  {
    auto cb_func = sigc::mem_fun(this, &PanelMenuView::UpdateShowNowWithDelay);
    _sources.AddTimeout(180, cb_func, UPDATE_SHOW_NOW_TIMEOUT);
  }
}

void PanelMenuView::SetMonitor(int monitor)
{
  _monitor = monitor;
  _monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(_monitor);

  _maximized_set.clear();
  GList* windows = bamf_matcher_get_window_stack_for_monitor(_matcher, _monitor);

  WindowManager& wm = WindowManager::Default();
  for (GList* l = windows; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    auto window = static_cast<BamfWindow*>(l->data);
    auto view = static_cast<BamfView*>(l->data);

    if (bamf_view_is_active(view))
    {
      _active_xid = bamf_window_get_xid(window);
    }

    if (bamf_window_maximized(window) == BAMF_WINDOW_MAXIMIZED)
    {
      Window xid = bamf_window_get_xid(window);

      _decor_map[xid] = wm.IsWindowDecorated(xid);

      if (_decor_map[xid])
        wm.Undecorate(xid);

      _maximized_set.insert(xid);
    }
  }

  Window maximized = GetMaximizedWindow();
  Window buttons_win = (maximized == _active_xid) ? maximized : 0;

  _window_buttons->SetMonitor(_monitor);
  _window_buttons->SetControlledWindow(buttons_win);

  g_list_free(windows);
}

bool PanelMenuView::GetControlsActive() const
{
  return _we_control_active;
}

void PanelMenuView::OnPanelViewMouseEnter(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state)
{
  if (!_is_inside)
  {
    if (_is_grabbed)
      _is_grabbed = false;
    else
      _is_inside = true;

    FullRedraw();
  }
}

void PanelMenuView::OnPanelViewMouseLeave(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state)
{
  if (_is_inside)
  {
    _is_inside = false;
    FullRedraw();
  }
}

void PanelMenuView::OnPanelViewMouseMove(int x, int y, int dx, int dy, unsigned long mouse_button_state, unsigned long special_keys_state)
{}

void PanelMenuView::SetMousePosition(int x, int y)
{
  if (_last_active_view ||
      (x >= 0 && y >= 0 && GetAbsoluteGeometry().IsPointInside(x, y)))
  {
    if (!_is_inside)
    {
      _is_inside = true;
      FullRedraw();
    }
  }
  else
  {
    if (_is_inside)
    {
      _is_inside = false;
      FullRedraw();
    }
  }
}
} // namespace unity
