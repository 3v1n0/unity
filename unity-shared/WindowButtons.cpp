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

#include "config.h"

#include <Nux/Nux.h>
#include <array>

#include <UnityCore/GLibWrapper.h>

#include "WindowButtons.h"
#include "WindowButtonPriv.h"

#include "unity-shared/TextureCache.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/WindowManager.h"

namespace unity
{

namespace internal
{
WindowButton::WindowButton(panel::WindowButtonType type)
  : nux::Button("", NUX_TRACKER_LOCATION)
  , enabled(sigc::mem_fun(this, &WindowButton::IsViewEnabled),
            sigc::mem_fun(this, &WindowButton::EnabledSetter))
  , overlay_mode(false)
  , type_(type)
{
  overlay_mode.changed.connect([this] (bool) { UpdateSize(); QueueDraw(); });
  SetAcceptKeyNavFocusOnMouseDown(false);
  panel::Style::Instance().changed.connect(sigc::mem_fun(this, &WindowButton::LoadImages));
  LoadImages();
}

void WindowButton::SetVisualState(nux::ButtonVisualState new_state)
{
  if (new_state != visual_state_)
  {
    visual_state_ = new_state;
    QueueDraw();
  }
}

void WindowButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  nux::BaseTexture* tex = nullptr;

  GfxContext.PushClippingRectangle(geo);

  if (overlay_mode())
  {
    if (!enabled())
    {
      tex = disabled_dash_tex_.GetPointer();
    }
    else
    {
      switch (visual_state_)
      {
        case nux::VISUAL_STATE_PRESSED:
          tex = pressed_dash_tex_.GetPointer();
          break;
        case nux::VISUAL_STATE_PRELIGHT:
          tex = prelight_dash_tex_.GetPointer();
          break;
        default:
          tex = normal_dash_tex_.GetPointer();
      }
    }
  }
  else if (!enabled())
  {
    tex = disabled_tex_.GetPointer();
  }
  else if (!Parent()->focused())
  {
    switch (visual_state_)
    {
      case nux::VISUAL_STATE_PRESSED:
        tex = unfocused_pressed_tex_.GetPointer();
        break;
      case nux::VISUAL_STATE_PRELIGHT:
        tex = unfocused_prelight_tex_.GetPointer();
        break;
      default:
        tex = unfocused_tex_.GetPointer();
    }
  }
  else
  {
    switch (visual_state_)
    {
      case nux::VISUAL_STATE_PRESSED:
        tex = pressed_tex_.GetPointer();
        break;
      case nux::VISUAL_STATE_PRELIGHT:
        tex = prelight_tex_.GetPointer();
        break;
      default:
        tex = normal_tex_.GetPointer();
    }
  }

  if (tex)
  {
    nux::TexCoordXForm texxform;
    GfxContext.QRP_1Tex(geo.x, geo.y, geo.width, geo.height,
                        tex->GetDeviceTexture(), texxform,
                        nux::color::White * Parent()->opacity());
  }

  GfxContext.PopClippingRectangle();
}

void WindowButton::UpdateSize()
{
  int panel_height = panel::Style::Instance().panel_height;
  nux::BaseTexture* tex;
  tex = (overlay_mode()) ? normal_dash_tex_.GetPointer() : normal_tex_.GetPointer();
  int width = 0;
  int height = 0;

  if (tex)
  {
    width = std::min(panel_height, tex->GetWidth());
    height = std::min(panel_height, tex->GetHeight());
  }

  SetMinMaxSize(width, height);
}

void WindowButton::LoadImages()
{
  panel::Style& style = panel::Style::Instance();

  normal_tex_ = style.GetWindowButton(type_, panel::WindowState::NORMAL);
  prelight_tex_ = style.GetWindowButton(type_, panel::WindowState::PRELIGHT);
  pressed_tex_ = style.GetWindowButton(type_, panel::WindowState::PRESSED);
  unfocused_tex_ = style.GetWindowButton(type_, panel::WindowState::BACKDROP);
  disabled_tex_ = style.GetWindowButton(type_, panel::WindowState::DISABLED);
  unfocused_prelight_tex_ = style.GetWindowButton(type_, panel::WindowState::BACKDROP_PRELIGHT);
  unfocused_pressed_tex_ = style.GetWindowButton(type_, panel::WindowState::BACKDROP_PRESSED);
  normal_dash_tex_ = GetDashWindowButton(type_, panel::WindowState::NORMAL);
  prelight_dash_tex_ = GetDashWindowButton(type_, panel::WindowState::PRELIGHT);
  pressed_dash_tex_ = GetDashWindowButton(type_, panel::WindowState::PRESSED);
  disabled_dash_tex_ = GetDashWindowButton(type_, panel::WindowState::DISABLED);

  UpdateSize();
  QueueDraw();
}

nux::ObjectPtr<nux::BaseTexture> WindowButton::GetDashWindowButton(panel::WindowButtonType type, panel::WindowState state)
{
  nux::ObjectPtr<nux::BaseTexture> texture;
  static const std::array<std::string, 4> names = {{ "close_dash", "minimize_dash", "unmaximize_dash", "maximize_dash" }};
  static const std::array<std::string, 4> states = {{ "", "_prelight", "_pressed", "_disabled" }};

  std::string subpath = names[static_cast<int>(type)] + states[static_cast<int>(state)] + ".png";

  auto& cache = TextureCache::GetDefault();
  texture = cache.FindTexture(subpath);

  if (!texture)
    texture = panel::Style::Instance().GetFallbackWindowButton(type, state);

  return texture;
}

panel::WindowButtonType WindowButton::GetType() const
{
  return type_;
}

bool WindowButton::EnabledSetter(bool new_value)
{
  if (enabled() == new_value)
    return false;

  SetEnableView(new_value);
  QueueDraw();
  return true;
}

std::string WindowButton::GetName() const
{
  return "WindowButton";
}

void WindowButton::AddProperties(debug::IntrospectionData& introspection)
{
  std::string type_name;
  std::string state_name;

  switch (type_)
  {
    case panel::WindowButtonType::CLOSE:
      type_name = "Close";
      break;
    case panel::WindowButtonType::MINIMIZE:
      type_name = "Minimize";
      break;
    case panel::WindowButtonType::MAXIMIZE:
      type_name = "Maximize";
      break;
    case panel::WindowButtonType::UNMAXIMIZE:
      type_name = "Unmaximize";
      break;
  }

  switch (visual_state_)
  {
    case nux::VISUAL_STATE_PRESSED:
      state_name = "pressed";
      break;
    case nux::VISUAL_STATE_PRELIGHT:
      state_name = "prelight";
      break;
    default:
      state_name = "normal";
  }

  introspection.add(GetAbsoluteGeometry())
                                  .add("type", type_name)
                                  .add("visible", IsVisible() && Parent()->opacity() != 0.0f)
                                  .add("sensitive", Parent()->GetInputEventSensitivity())
                                  .add("enabled", enabled())
                                  .add("visual_state", state_name)
                                  .add("opacity", Parent()->opacity())
                                  .add("focused", Parent()->focused())
                                  .add("overlay_mode", overlay_mode());
}
} // Internal Namespace


WindowButtons::WindowButtons()
  : HLayout("", NUX_TRACKER_LOCATION)
  , monitor(0)
  , controlled_window(0)
  , opacity(1.0f, sigc::mem_fun(this, &WindowButtons::OpacitySetter))
  , focused(true)
{
  controlled_window.changed.connect(sigc::mem_fun(this, &WindowButtons::OnControlledWindowChanged));
  focused.changed.connect(sigc::hide(sigc::mem_fun(this, &WindowButtons::QueueDraw)));

  auto lambda_enter = [this](int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    mouse_enter.emit(x, y, button_flags, key_flags);
  };

  auto lambda_leave = [this](int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    mouse_leave.emit(x, y, button_flags, key_flags);
  };

  auto lambda_moved = [this](int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
  {
    mouse_move.emit(x, y, dx, dy, button_flags, key_flags);
  };

  internal::WindowButton* but;
  but = new internal::WindowButton(panel::WindowButtonType::CLOSE);
  AddView(but, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  AddChild(but);
  but->click.connect(sigc::mem_fun(this, &WindowButtons::OnCloseClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  but = new internal::WindowButton(panel::WindowButtonType::MINIMIZE);
  AddView(but, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  AddChild(but);
  but->click.connect(sigc::mem_fun(this, &WindowButtons::OnMinimizeClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  but = new internal::WindowButton(panel::WindowButtonType::UNMAXIMIZE);
  AddView(but, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  AddChild(but);
  but->click.connect(sigc::mem_fun(this, &WindowButtons::OnRestoreClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  but = new internal::WindowButton(panel::WindowButtonType::MAXIMIZE);
  AddView(but, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  AddChild(but);
  but->click.connect(sigc::mem_fun(this, &WindowButtons::OnMaximizeClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);
  but->SetVisible(false);

  SetContentDistribution(nux::MAJOR_POSITION_START);

  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &WindowButtons::OnOverlayShown));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &WindowButtons::OnOverlayHidden));
  Settings::Instance().form_factor.changed.connect(sigc::mem_fun(this, &WindowButtons::OnDashSettingsUpdated));
}

nux::Area* WindowButtons::FindAreaUnderMouse(const nux::Point& mouse, nux::NuxEventType event_type)
{
  if (!GetInputEventSensitivity())
    return nullptr;

  /* The first button should be clickable on the left space too, to
   * make Fitts happy. All also on their top side. See bug #839690 */
  bool first_found = false;

  for (auto area : GetChildren())
  {
    if (area->IsVisible())
    {
      nux::Geometry const& geo = area->GetAbsoluteGeometry();

      if (!first_found)
      {
        first_found = true;

        if (mouse.x < geo.x && mouse.y < (geo.y + geo.height))
          return area;
      }

      if (geo.IsPointInside(mouse.x, mouse.y))
        return area;

      if (mouse.x >= geo.x && mouse.x <= (geo.x + geo.width) && mouse.y <= geo.y)
        return area;
    }
  }

  return nullptr;
}

void WindowButtons::OnCloseClicked(nux::Button *button)
{
  auto win_button = static_cast<internal::WindowButton*>(button);

  if (!win_button->enabled())
    return;

  if (win_button->overlay_mode())
  {
    ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
  }
  else
  {
    WindowManager::Default().Close(controlled_window());
  }

  close_clicked.emit();
}

void WindowButtons::OnMinimizeClicked(nux::Button *button)
{
  auto win_button = static_cast<internal::WindowButton*>(button);

  if (!win_button->enabled())
    return;

  if (!win_button->overlay_mode())
    WindowManager::Default().Minimize(controlled_window());

  minimize_clicked.emit();
}

void WindowButtons::OnRestoreClicked(nux::Button *button)
{
  auto win_button = static_cast<internal::WindowButton*>(button);

  if (!win_button->enabled())
    return;

  if (win_button->overlay_mode())
  {
    Settings::Instance().form_factor = FormFactor::DESKTOP;
  }
  else
  {
    WindowManager& wm = WindowManager::Default();
    Window to_restore = controlled_window();

    wm.Raise(to_restore);
    wm.Activate(to_restore);
    wm.Restore(to_restore);
  }

  restore_clicked.emit();
}

void WindowButtons::OnMaximizeClicked(nux::Button *button)
{
  auto win_button = static_cast<internal::WindowButton*>(button);

  if (!win_button->enabled())
    return;

  if (win_button->overlay_mode())
  {
    Settings::Instance().form_factor = FormFactor::NETBOOK;
  }

  maximize_clicked.emit();
}

void WindowButtons::OnOverlayShown(GVariant* data)
{
  internal::WindowButton* maximize_button = nullptr;
  internal::WindowButton* restore_button = nullptr;
  glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  int width, height;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

  if (overlay_monitor != monitor())
  {
    for (auto area : GetChildren())
      static_cast<internal::WindowButton*>(area)->enabled = false;

    return;
  }

  active_overlay_ = overlay_identity.Str();

  for (auto area : GetChildren())
  {
    auto button = static_cast<internal::WindowButton*>(area);
    if (button->GetType() == panel::WindowButtonType::CLOSE)
      button->enabled = true;

    if (button->GetType() == panel::WindowButtonType::UNMAXIMIZE)
      restore_button = button;

    if (button->GetType() == panel::WindowButtonType::MAXIMIZE)
      maximize_button = button;

    if (button->GetType() == panel::WindowButtonType::MINIMIZE)
      button->enabled = false;

    button->overlay_mode = true;
  }

  if (restore_button && maximize_button)
  {
    Settings &dash_settings = Settings::Instance();
    bool maximizable = (dash_settings.form_factor() == FormFactor::DESKTOP);

    restore_button->enabled = can_maximise;
    maximize_button->enabled = can_maximise;

    if (maximizable != maximize_button->IsVisible())
    {
      if (maximize_button->IsVisible())
        restore_button->SetVisualState(maximize_button->GetVisualState());
      else if (restore_button->IsVisible())
        maximize_button->SetVisualState(restore_button->GetVisualState());

      restore_button->SetVisible(!maximizable);
      maximize_button->SetVisible(maximizable);

      QueueDraw();
    }
  }
}

void WindowButtons::OnOverlayHidden(GVariant* data)
{
  internal::WindowButton* maximize_button = nullptr;
  internal::WindowButton* restore_button = nullptr;

  glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  int width, height;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

  if (overlay_monitor != monitor())
  {
    for (auto area : GetChildren())
      static_cast<internal::WindowButton*>(area)->enabled = true;
  }

  if (active_overlay_ != overlay_identity.Str())
    return;

  active_overlay_ = "";

  WindowManager& wm = WindowManager::Default();
  for (auto area : GetChildren())
  {
    auto button = static_cast<internal::WindowButton*>(area);

    if (controlled_window())
    {
      if (button->GetType() == panel::WindowButtonType::CLOSE)
        button->enabled = wm.IsWindowClosable(controlled_window());

      if (button->GetType() == panel::WindowButtonType::MINIMIZE)
        button->enabled = wm.IsWindowMinimizable(controlled_window());
    }

    if (button->GetType() == panel::WindowButtonType::UNMAXIMIZE)
      restore_button = button;

    if (button->GetType() == panel::WindowButtonType::MAXIMIZE)
      maximize_button = button;

    button->overlay_mode = false;
  }

  if (restore_button && maximize_button)
  {
    restore_button->enabled = true;
    maximize_button->enabled = true;

    if (!restore_button->IsVisible())
    {
      restore_button->SetVisualState(maximize_button->GetVisualState());

      restore_button->SetVisible(true);
      maximize_button->SetVisible(false);

      QueueDraw();
    }
  }
}

void WindowButtons::OnDashSettingsUpdated(FormFactor form_factor)
{
  internal::WindowButton* maximize_button = nullptr;
  internal::WindowButton* restore_button = nullptr;

  for (auto area : GetChildren())
  {
    auto button = static_cast<internal::WindowButton*>(area);

    if (!button->overlay_mode())
      break;

    if (button->GetType() == panel::WindowButtonType::UNMAXIMIZE)
      restore_button = button;

    if (button->GetType() == panel::WindowButtonType::MAXIMIZE)
      maximize_button = button;

    if (restore_button && maximize_button)
      break;
  }

  if (restore_button && restore_button->overlay_mode() && maximize_button)
  {
    bool maximizable = (form_factor == FormFactor::DESKTOP);

    if (maximizable != maximize_button->IsVisible())
    {
      if (maximize_button->IsVisible())
        restore_button->SetVisualState(maximize_button->GetVisualState());
      else if (restore_button->IsVisible())
        maximize_button->SetVisualState(restore_button->GetVisualState());

      restore_button->SetVisible(!maximizable);
      maximize_button->SetVisible(maximizable);

      QueueRelayout();
    }
  }
}

bool WindowButtons::OpacitySetter(double& target, double new_value)
{
  double opacity = CLAMP(new_value, 0.0f, 1.0f);

  if (opacity != target)
  {
    target = opacity;
    SetInputEventSensitivity(opacity != 0.0f);
    QueueDraw();

    return true;
  }

  return false;
}

void WindowButtons::OnControlledWindowChanged(Window xid)
{
  if (xid && active_overlay_.empty())
  {
    WindowManager& wm = WindowManager::Default();
    for (auto area : GetChildren())
    {
      auto button = static_cast<internal::WindowButton*>(area);

      if (button->GetType() == panel::WindowButtonType::CLOSE)
        button->enabled = wm.IsWindowClosable(xid);

      if (button->GetType() == panel::WindowButtonType::MINIMIZE)
        button->enabled = wm.IsWindowMinimizable(xid);
    }
  }
}

bool WindowButtons::IsMouseOwner()
{
  for (auto* area : GetChildren())
  {
    if (static_cast<internal::WindowButton*>(area)->IsMouseOwner())
      return true;
  }

  return false;
}

std::string WindowButtons::GetName() const
{
  return "WindowButtons";
}

void WindowButtons::AddProperties(debug::IntrospectionData& introspection)
{
  introspection.add(GetAbsoluteGeometry())
               .add("monitor", monitor())
               .add("opacity", opacity())
               .add("visible", opacity() != 0.0f)
               .add("sensitive", GetInputEventSensitivity())
               .add("focused", focused())
               .add("controlled_window", controlled_window());
}

} // unity namespace
