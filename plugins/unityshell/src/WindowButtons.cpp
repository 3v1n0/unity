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

#include "config.h"

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/Button.h>

#include <NuxGraphics/GLThread.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include <UnityCore/Variant.h>
#include "WindowButtons.h"

#include "UBusWrapper.h"
#include "UBusMessages.h"

#include "DashSettings.h"
#include "PanelStyle.h"

#include <UnityCore/GLibWrapper.h>

using namespace unity;

class WindowButton : public nux::Button
{
  // A single window button
public:
  WindowButton(panel::WindowButtonType type)
    : nux::Button("", NUX_TRACKER_LOCATION),
      _type(type),
      _overlay_is_open(false),
      _mouse_is_down(false),
      _opacity(1.0f),
      _focused(true)
  {
    LoadImages();
    UpdateDashUnmaximize();
    panel::Style::Instance().changed.connect(sigc::mem_fun(this, &WindowButton::LoadImages));
    dash::Settings::Instance().changed.connect(sigc::mem_fun(this, &WindowButton::UpdateDashUnmaximize));

    _ubus_manager.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &WindowButton::OnOverlayShown));
    _ubus_manager.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &WindowButton::OnOverlayHidden));

    /* FIXME HasMouseFocus() doesn't seem to work correctly, so we use this workaround */
    mouse_down.connect([&_mouse_is_down](int, int, unsigned long, unsigned long) {
      _mouse_is_down = true;
    });
    mouse_up.connect([&_mouse_is_down](int, int, unsigned long, unsigned long) {
      _mouse_is_down = false;
    });
  }

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
  {
    nux::Geometry const& geo = GetGeometry();
    nux::BaseTexture* tex;
    nux::TexCoordXForm texxform;

    GfxContext.PushClippingRectangle(geo);

    if (_overlay_is_open)
    {
      if (_type == panel::WindowButtonType::UNMAXIMIZE && !_overlay_can_maximize)
      {
        tex = _disabled_dash_tex.GetPointer();
      }
      else
      {
        //FIXME should use HasMouseFocus()
        if (_mouse_is_down && IsMouseInside())
          tex = _pressed_dash_tex.GetPointer();
        else if (IsMouseInside())
          tex = _prelight_dash_tex.GetPointer();
        else
          tex = _normal_dash_tex.GetPointer();
      }
    }
    else if (!_focused)
    {
      //FIXME should use HasMouseFocus()
      if (_mouse_is_down && IsMouseInside())
        tex = _unfocused_pressed_tex.GetPointer();
      else if (IsMouseInside())
        tex = _unfocused_prelight_tex.GetPointer();
      else
        tex = _unfocused_tex.GetPointer();
    }
    else
    {
      //FIXME should use HasMouseFocus()
      if (_mouse_is_down && IsMouseInside())
        tex = _pressed_tex.GetPointer();
      else if (IsMouseInside())
        tex = _prelight_tex.GetPointer();
      else
        tex = _normal_tex.GetPointer();
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
    _unfocused_prelight_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::UNFOCUSED_PRELIGHT));
    _unfocused_pressed_tex.Adopt(style.GetWindowButton(_type, panel::WindowState::UNFOCUSED_PRESSED));
    _normal_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::NORMAL));
    _prelight_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::PRELIGHT));
    _pressed_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::PRESSED));

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

  void UpdateDashUnmaximize()
  {
    // only update the unmaximize button
    if (_type != panel::WindowButtonType::UNMAXIMIZE)
      return;

    //!!FIXME!! - don't have disabled instances of the (un)maximize buttons
    if (dash::Settings::Instance().GetFormFactor() == dash::FormFactor::DESKTOP)
    {
      // get maximize buttons
      _normal_dash_tex.Adopt(GetDashMaximizeWindowButton(panel::WindowState::NORMAL));
      _prelight_dash_tex.Adopt(GetDashMaximizeWindowButton(panel::WindowState::PRELIGHT));
      _pressed_dash_tex.Adopt(GetDashMaximizeWindowButton(panel::WindowState::PRESSED));
      _disabled_dash_tex.Adopt(GetDashMaximizeWindowButton(panel::WindowState::DISABLED));
    }
    else
    {
      // get unmaximize buttons
      _normal_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::NORMAL));
      _prelight_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::PRELIGHT));
      _pressed_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::PRESSED));
      _disabled_dash_tex.Adopt(GetDashWindowButton(_type, panel::WindowState::DISABLED));
    }

    // still check if the dash is really opened,
    // someone could change the form factor through dconf
    // when the dash is closed
    if (_overlay_is_open)
    {
      if (_normal_dash_tex)
        SetMinMaxSize(_normal_dash_tex->GetWidth(), _normal_dash_tex->GetHeight());
    }

    QueueDraw();
  }

  void SetOpacity(double opacity)
  {
    if (_opacity != opacity)
    {
      _opacity = opacity;
      NeedRedraw();
    }
  }

  double GetOpacity()
  {
    return _opacity;
  }

  void SetFocusedState(bool focused)
  {
    if (_focused != focused)
    {
      _focused = focused;
      NeedRedraw();
    }
  }

private:
  panel::WindowButtonType _type;
  bool _overlay_is_open;
  bool _overlay_can_maximize;
  bool _mouse_is_down;
  guint32 _place_shown_interest;
  guint32 _place_hidden_interest;
  double _opacity;
  bool _focused;

  nux::ObjectPtr<nux::BaseTexture> _normal_tex;
  nux::ObjectPtr<nux::BaseTexture> _prelight_tex;
  nux::ObjectPtr<nux::BaseTexture> _pressed_tex;
  nux::ObjectPtr<nux::BaseTexture> _unfocused_tex;
  nux::ObjectPtr<nux::BaseTexture> _unfocused_prelight_tex;
  nux::ObjectPtr<nux::BaseTexture> _unfocused_pressed_tex;
  nux::ObjectPtr<nux::BaseTexture> _normal_dash_tex;
  nux::ObjectPtr<nux::BaseTexture> _prelight_dash_tex;
  nux::ObjectPtr<nux::BaseTexture> _pressed_dash_tex;
  nux::ObjectPtr<nux::BaseTexture> _disabled_dash_tex;
  UBusManager _ubus_manager;

  void OnOverlayShown(GVariant* data)
  {
    unity::glib::String overlay_identity;
    gboolean can_maximise = FALSE;
    gint32 overlay_monitor = 0;
    g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, 
                  &overlay_identity, &can_maximise, &overlay_monitor);

    _overlay_is_open = true;
    if (_normal_dash_tex)
      SetMinMaxSize(_normal_dash_tex->GetWidth(), _normal_dash_tex->GetHeight());

    _overlay_can_maximize = (can_maximise) ? true : false;

    QueueDraw();
  }

  void OnOverlayHidden(GVariant* data)
  {
    _overlay_is_open = false;

    if (_normal_tex)
      SetMinMaxSize(_normal_tex->GetWidth(), _normal_tex->GetHeight());

    QueueDraw();
  }

  nux::BaseTexture* GetDashWindowButton(panel::WindowButtonType type,
                                        panel::WindowState state)
  {
    const char* names[] = { "close_dash", "minimize_dash", "unmaximize_dash" };
    const char* states[] = { "", "_prelight", "_pressed", "_disabled" };

    std::ostringstream subpath;
    subpath << names[static_cast<int>(type)]
            << states[static_cast<int>(state)] << ".png";

    glib::String filename(g_build_filename(PKGDATADIR, subpath.str().c_str(), NULL));

    glib::Error error;
    glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file(filename.Value(), &error));

    // not handling broken texture or missing files
    return nux::CreateTexture2DFromPixbuf(pixbuf, true);
  }

  nux::BaseTexture* GetDashMaximizeWindowButton(panel::WindowState state)
  {
    const char* states[] = { "", "_prelight", "_pressed", "_disabled" };

    std::ostringstream subpath;
    subpath << "maximize_dash" << states[static_cast<int>(state)] << ".png";

    glib::String filename(g_build_filename(PKGDATADIR, subpath.str().c_str(), NULL));

    glib::Error error;
    glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file(filename.Value(), &error));

    // not handling broken texture or missing files
    return nux::CreateTexture2DFromPixbuf(pixbuf, true);
  }
};


WindowButtons::WindowButtons()
  : HLayout("", NUX_TRACKER_LOCATION)
  , opacity_(1.0f)
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
  but->state_change.connect(sigc::mem_fun(this, &WindowButtons::OnCloseClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  but = new WindowButton(panel::WindowButtonType::MINIMIZE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->state_change.connect(sigc::mem_fun(this, &WindowButtons::OnMinimizeClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  but = new WindowButton(panel::WindowButtonType::UNMAXIMIZE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->state_change.connect(sigc::mem_fun(this, &WindowButtons::OnRestoreClicked));
  but->mouse_enter.connect(lambda_enter);
  but->mouse_leave.connect(lambda_leave);
  but->mouse_move.connect(lambda_moved);

  SetContentDistribution(nux::eStackLeft);
}


WindowButtons::~WindowButtons()
{
}

void
WindowButtons::OnCloseClicked(nux::View* view)
{
  close_clicked.emit();
}

void
WindowButtons::OnMinimizeClicked(nux::View* view)
{
  minimize_clicked.emit();
}

void
WindowButtons::OnRestoreClicked(nux::View* view)
{
  restore_clicked.emit();
}

nux::Area*
WindowButtons::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  return nux::HLayout::FindAreaUnderMouse(mouse_position, event_type);
}

void
WindowButtons::SetOpacity(double opacity)
{
  opacity = CLAMP(opacity, 0.0f, 1.0f);

  for (auto area : GetChildren())
  {
    auto but = dynamic_cast<WindowButton*>(area);

    if (but)
      but->SetOpacity(opacity);
  }

  if (opacity_ != opacity)
  {
    opacity_ = opacity;
    NeedRedraw();
  }
}

double
WindowButtons::GetOpacity()
{
  return opacity_;
}

void
WindowButtons::SetFocusedState(bool focused)
{
  for (auto area : GetChildren())
  {
    auto but = dynamic_cast<WindowButton*>(area);

    if (but)
      but->SetFocusedState(focused);
  }

  if (focused_ != focused)
  {
    focused_ = focused;
    NeedRedraw();
  }
}

bool
WindowButtons::GetFocusedState()
{
  return focused_;
}

std::string
WindowButtons::GetName() const
{
  return "window-buttons";
}

void
WindowButtons::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder).add(GetGeometry());
}
