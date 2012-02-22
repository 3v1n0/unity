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

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

#include "WindowButtons.h"

#include "DashSettings.h"
#include "PanelStyle.h"
#include "UBusMessages.h"
#include "WindowManager.h"

using namespace unity;

class WindowButton : public nux::Button
{
  // A single window button
public:
  WindowButton(panel::WindowButtonType type)
    : nux::Button("", NUX_TRACKER_LOCATION)
    , _type(type)
    , _enabled(true)
    , _focused(true)
    , _overlay_is_open(false)
    , _opacity(1.0f)
  {
    LoadImages();
    panel::Style::Instance().changed.connect(sigc::mem_fun(this, &WindowButton::LoadImages));
  }

  void SetVisualState(nux::ButtonVisualState new_state)
  {
    if (new_state != visual_state_)
    {
      visual_state_ = new_state;
      QueueDraw();
    }
  }

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
  {
    nux::Geometry const& geo = GetGeometry();
    nux::BaseTexture* tex;
    nux::TexCoordXForm texxform;

    GfxContext.PushClippingRectangle(geo);

    if (_overlay_is_open)
    {
      if (!_enabled)
      {
        tex = _disabled_dash_tex.GetPointer();
      }
      else
      {
        switch (visual_state_)
        {
          case nux::VISUAL_STATE_PRESSED:
            tex = _pressed_dash_tex.GetPointer();
            break;
          case nux::VISUAL_STATE_PRELIGHT:
            tex = _prelight_dash_tex.GetPointer();
            break;
          default:
            tex = _normal_dash_tex.GetPointer();
        }
      }
    }
    else if (!_enabled)
    {
      tex = _disabled_tex.GetPointer();
    }
    else if (!_focused)
    {
      switch (visual_state_)
      {
        case nux::VISUAL_STATE_PRESSED:
          tex = _unfocused_pressed_tex.GetPointer();
          break;
        case nux::VISUAL_STATE_PRELIGHT:
          tex = _unfocused_prelight_tex.GetPointer();
          break;
        default:
          tex = _unfocused_tex.GetPointer();
      }
    }
    else
    {
      switch (visual_state_)
      {
        case nux::VISUAL_STATE_PRESSED:
          tex = _pressed_tex.GetPointer();
          break;
        case nux::VISUAL_STATE_PRELIGHT:
          tex = _prelight_tex.GetPointer();
          break;
        default:
          tex = _normal_tex.GetPointer();
      }
    }

    if (tex)
    {
      GfxContext.QRP_1Tex(geo.x,
                          geo.y,
                          geo.width,
                          geo.height,
                          tex->GetDeviceTexture(),
                          texxform,
                          nux::color::White * _opacity);
    }

    GfxContext.PopClippingRectangle();
  }

  void LoadImages()
  {
    panel::Style& style = panel::Style::Instance();

    _normal_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::NORMAL));
    _prelight_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::PRELIGHT));
    _pressed_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::PRESSED));
    _unfocused_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::UNFOCUSED));
    _disabled_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::DISABLED));
    _unfocused_prelight_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::UNFOCUSED_PRELIGHT));
    _unfocused_pressed_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::UNFOCUSED_PRESSED));
    _normal_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::NORMAL));
    _prelight_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::PRELIGHT));
    _pressed_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::PRESSED));
    _disabled_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::DISABLED));

    if (_overlay_is_open)
    {
      if (_normal_dash_tex)
        SetMinMaxSize(_normal_dash_tex->GetWidth(), _normal_dash_tex->GetHeight());
    }
    else
    {
      if (_normal_tex)
        SetMinMaxSize(_normal_tex->GetWidth(), _normal_tex->GetHeight());
    }

    QueueDraw();
  }

  void SetOpacity(double opacity)
  {
    if (_opacity != opacity)
    {
      _opacity = opacity;
      QueueDraw();
    }
  }

  double GetOpacity() const
  {
    return _opacity;
  }

  void SetFocusedState(bool focused)
  {
    if (_focused != focused)
    {
      _focused = focused;
      QueueDraw();
    }
  }

  void SetOverlayOpen(bool open)
  {
    if (_overlay_is_open == open)
      return;

    _overlay_is_open = open;

    if (_overlay_is_open)
    {
      if (_normal_dash_tex)
        SetMinMaxSize(_normal_dash_tex->GetWidth(), _normal_dash_tex->GetHeight());
    }
    else
    {
      if (_normal_tex)
        SetMinMaxSize(_normal_tex->GetWidth(), _normal_tex->GetHeight());
    }

    QueueDraw();
  }

  bool IsOverlayOpen()
  {
    return _overlay_is_open;
  }

  panel::WindowButtonType GetType() const
  {
    return _type;
  }

  void ChangeType(panel::WindowButtonType new_type)
  {
    if (_type != new_type)
    {
      _type = new_type;
      LoadImages();
    }
  }

  void SetEnabled(bool enabled)
  {
    if (enabled == _enabled)
      return;

    _enabled = enabled;
    QueueDraw();
  }  

  bool IsEnabled()
  {
    return _enabled;
  }

private:
  panel::WindowButtonType _type;
  bool _enabled;
  bool _focused;
  bool _overlay_is_open;
  double _opacity;

  nux::ObjectPtr<nux::BaseTexture> _normal_tex;
  nux::ObjectPtr<nux::BaseTexture> _prelight_tex;
  nux::ObjectPtr<nux::BaseTexture> _pressed_tex;
  nux::ObjectPtr<nux::BaseTexture> _unfocused_tex;
  nux::ObjectPtr<nux::BaseTexture> _unfocused_prelight_tex;
  nux::ObjectPtr<nux::BaseTexture> _unfocused_pressed_tex;
  nux::ObjectPtr<nux::BaseTexture> _disabled_tex;
  nux::ObjectPtr<nux::BaseTexture> _normal_dash_tex;
  nux::ObjectPtr<nux::BaseTexture> _prelight_dash_tex;
  nux::ObjectPtr<nux::BaseTexture> _pressed_dash_tex;
  nux::ObjectPtr<nux::BaseTexture> _disabled_dash_tex;

  nux::BaseTexture* GetDashWindowButton(panel::WindowButtonType type,
                                        panel::WindowState state)
  {
    nux::BaseTexture* texture = nullptr;
    const char* names[] = { "close_dash", "minimize_dash", "unmaximize_dash", "maximize_dash" };
    const char* states[] = { "", "_prelight", "_pressed", "_disabled" };

    std::ostringstream subpath;
    subpath << names[static_cast<int>(type)]
            << states[static_cast<int>(state)] << ".png";

    glib::String filename(g_build_filename(PKGDATADIR, subpath.str().c_str(), NULL));

    glib::Error error;
    glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file(filename, &error));

    if (pixbuf && !error)
      texture = nux::CreateTexture2DFromPixbuf(pixbuf, true);

    if (!texture)
      texture = panel::Style::Instance().GetFallbackWindowButton(type, state);

    return texture;
  }
};


WindowButtons::WindowButtons()
  : HLayout("", NUX_TRACKER_LOCATION)
  , opacity_(1.0f)
  , focused_(true)
  , window_xid_(0)
{
  WindowButton* but;

  auto lambda_enter = [&](int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    mouse_enter.emit(x, y, button_flags, key_flags);
  };

  auto lambda_leave = [&](int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    mouse_leave.emit(x, y, button_flags, key_flags);
  };

  auto lambda_moved = [&](int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
  {
    mouse_move.emit(x, y, dx, dy, button_flags, key_flags);
  };

  but = new WindowButton(panel::WindowButtonType::CLOSE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->click.connect(sigc::mem_fun(this, &WindowButtons::OnCloseClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  but = new WindowButton(panel::WindowButtonType::MINIMIZE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->click.connect(sigc::mem_fun(this, &WindowButtons::OnMinimizeClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  but = new WindowButton(panel::WindowButtonType::UNMAXIMIZE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->click.connect(sigc::mem_fun(this, &WindowButtons::OnRestoreClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  but = new WindowButton(panel::WindowButtonType::MAXIMIZE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->click.connect(sigc::mem_fun(this, &WindowButtons::OnMaximizeClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);
  but->SetVisible(false);

  SetContentDistribution(nux::eStackLeft);

  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &WindowButtons::OnOverlayShown));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &WindowButtons::OnOverlayHidden));
  dash::Settings::Instance().changed.connect(sigc::mem_fun(this, &WindowButtons::OnDashSettingsUpdated));
}


void WindowButtons::OnCloseClicked(nux::Button *button)
{
  auto win_button = dynamic_cast<WindowButton*>(button);

  if (!win_button || !win_button->IsEnabled())
    return;

  if (win_button->IsOverlayOpen())
  {
    ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
  }
  else
  {
    WindowManager::Default()->Close(window_xid_);
  }

  close_clicked.emit();
}

void WindowButtons::OnMinimizeClicked(nux::Button *button)
{
  auto win_button = dynamic_cast<WindowButton*>(button);

  if (!win_button || !win_button->IsEnabled())
    return;

  if (!win_button->IsOverlayOpen())
    WindowManager::Default()->Minimize(window_xid_);

  minimize_clicked.emit();
}

void WindowButtons::OnRestoreClicked(nux::Button *button)
{
  auto win_button = dynamic_cast<WindowButton*>(button);

  if (!win_button || !win_button->IsEnabled())
    return;

  if (win_button->IsOverlayOpen())
  {
    dash::Settings::Instance().SetFormFactor(dash::FormFactor::DESKTOP);
  }
  else
  {
    WindowManager* wm = WindowManager::Default();
    Window to_restore = window_xid_;

    wm->Raise(to_restore);
    wm->Activate(to_restore);
    wm->Restore(to_restore);
  }

  restore_clicked.emit();
}

void WindowButtons::OnMaximizeClicked(nux::Button *button)
{
  auto win_button = dynamic_cast<WindowButton*>(button);

  if (!win_button || !win_button->IsEnabled())
    return;

  if (win_button->IsOverlayOpen())
  {
    dash::Settings::Instance().SetFormFactor(dash::FormFactor::NETBOOK);
  }

  maximize_clicked.emit();
}

void WindowButtons::OnOverlayShown(GVariant* data)
{
  WindowButton* maximize_button = nullptr;
  WindowButton* restore_button = nullptr;
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, 
                &overlay_identity, &can_maximise, &overlay_monitor);

  for (auto area : GetChildren())
  {
    auto button = dynamic_cast<WindowButton*>(area);

    if (button)
    {
      if (button->GetType() == panel::WindowButtonType::UNMAXIMIZE)
        restore_button = button;

      if (button->GetType() == panel::WindowButtonType::MAXIMIZE)
        maximize_button = button;

      button->SetOverlayOpen(true);
    }
  }

  if (restore_button && maximize_button)
  {
    dash::Settings &dash_settings = dash::Settings::Instance();
    bool maximizable = (dash_settings.GetFormFactor() == dash::FormFactor::DESKTOP);

    restore_button->SetEnabled((can_maximise) ? true : false);
    maximize_button->SetEnabled((can_maximise) ? true : false);

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
  WindowButton* maximize_button = nullptr;
  WindowButton* restore_button = nullptr;

  for (auto area : GetChildren())
  {
    auto button = dynamic_cast<WindowButton*>(area);

    if (button)
    {
      if (button->GetType() == panel::WindowButtonType::UNMAXIMIZE)
        restore_button = button;

      if (button->GetType() == panel::WindowButtonType::MAXIMIZE)
        maximize_button = button;

      button->SetOverlayOpen(false);
    }
  }

  if (restore_button && maximize_button)
  {
    restore_button->SetEnabled(true);
    maximize_button->SetEnabled(true);

    if (!restore_button->IsVisible())
    {
      restore_button->SetVisualState(maximize_button->GetVisualState());

      restore_button->SetVisible(true);
      maximize_button->SetVisible(false);

      QueueDraw();
    }
  }
}

void WindowButtons::OnDashSettingsUpdated()
{
  WindowButton* maximize_button = nullptr;
  WindowButton* restore_button = nullptr;

  for (auto area : GetChildren())
  {
    auto button = dynamic_cast<WindowButton*>(area);

    if (button)
    {
      if (!button->IsOverlayOpen())
        break;

      if (button->GetType() == panel::WindowButtonType::UNMAXIMIZE)
        restore_button = button;

      if (button->GetType() == panel::WindowButtonType::MAXIMIZE)
        maximize_button = button;

      if (restore_button && maximize_button)
        break;
    }
  }

  if (restore_button && restore_button->IsOverlayOpen() && maximize_button)
  {
    dash::Settings &dash_settings = dash::Settings::Instance();
    bool maximizable = (dash_settings.GetFormFactor() == dash::FormFactor::DESKTOP);

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

void WindowButtons::SetOpacity(double opacity)
{
  opacity = CLAMP(opacity, 0.0f, 1.0f);

  for (auto area : GetChildren())
  {
    auto button = dynamic_cast<WindowButton*>(area);

    if (button)
      button->SetOpacity(opacity);
  }

  if (opacity_ != opacity)
  {
    opacity_ = opacity;
    QueueDraw();
  }
}

double WindowButtons::GetOpacity()
{
  return opacity_;
}

void WindowButtons::SetFocusedState(bool focused)
{
  for (auto area : GetChildren())
  {
    auto button = dynamic_cast<WindowButton*>(area);

    if (button)
      button->SetFocusedState(focused);
  }

  if (focused_ != focused)
  {
    focused_ = focused;
    QueueDraw();
  }
}

bool WindowButtons::GetFocusedState()
{
  return focused_;
}

void WindowButtons::SetControlledWindow(Window xid)
{
  if (xid != window_xid_)
  {
    window_xid_ = xid;
  }
}

Window WindowButtons::GetControlledWindow()
{
  return window_xid_;
}

std::string WindowButtons::GetName() const
{
  return "window-buttons";
}

void WindowButtons::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder).add(GetGeometry())
                                         .add("opacity", opacity_)
                                         .add("focused", focused_)
                                         .add("window", window_xid_);
}
