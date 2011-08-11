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

#include "Nux/Nux.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/Button.h"

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include <UnityCore/Variant.h>
#include "WindowButtons.h"

#include <glib.h>

#include "PanelStyle.h"
class WindowButton : public nux::Button
{
  // A single window button
public:
  WindowButton(PanelStyle::WindowButtonType type)
    : nux::Button("", NUX_TRACKER_LOCATION),
      _type(type),
      _normal_tex(NULL),
      _prelight_tex(NULL),
      _pressed_tex(NULL)
  {
    LoadImages();
    PanelStyle::GetDefault()->changed.connect(sigc::mem_fun(this, &WindowButton::LoadImages));
  }

  ~WindowButton()
  {
    _normal_tex->UnReference();
    _prelight_tex->UnReference();
    _pressed_tex->UnReference();
  }

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
  {
    nux::Geometry      geo  = GetGeometry();
    nux::BaseTexture*  tex;
    nux::TexCoordXForm texxform;

    GfxContext.PushClippingRectangle(geo);

    if (HasMouseFocus() && IsMouseInside())
    {
      tex = _pressed_tex;
    }
    else if (IsMouseInside())
    {
      tex = _prelight_tex;
    }
    else
    {
      tex = _normal_tex;
    }

    GfxContext.GetRenderStates().SetBlend(true);
    GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
    GfxContext.GetRenderStates().SetColorMask(true, true, true, true);
    if (tex)
      GfxContext.QRP_1Tex(geo.x,
                          geo.y,
                          (float)geo.width,
                          (float)geo.height,
                          tex->GetDeviceTexture(),
                          texxform,
                          nux::color::White);
    GfxContext.GetRenderStates().SetBlend(false);
    GfxContext.PopClippingRectangle();
  }

  void LoadImages()
  {
    PanelStyle* style = PanelStyle::GetDefault();

    if (_normal_tex)
      _normal_tex->UnReference();
    if (_prelight_tex)
      _prelight_tex->UnReference();
    if (_pressed_tex)
      _pressed_tex->UnReference();

    _normal_tex = style->GetWindowButton(_type, PanelStyle::WINDOW_STATE_NORMAL);
    _prelight_tex = style->GetWindowButton(_type, PanelStyle::WINDOW_STATE_PRELIGHT);
    _pressed_tex = style->GetWindowButton(_type, PanelStyle::WINDOW_STATE_PRESSED);

    if (_normal_tex)
      SetMinMaxSize(_normal_tex->GetWidth(), _normal_tex->GetHeight());

    QueueDraw();
  }

private:
  PanelStyle::WindowButtonType _type;
  nux::BaseTexture* _normal_tex;
  nux::BaseTexture* _prelight_tex;
  nux::BaseTexture* _pressed_tex;
};


WindowButtons::WindowButtons()
  : HLayout("", NUX_TRACKER_LOCATION)
{
  WindowButton* but;

  auto lambda_statechanged = [&](int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    redraw_signal.emit();
  };

  auto lambda_moved = [&](int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
  {
    mouse_moved.emit(x, y, dx, dy, button_flags, key_flags);
  };
  but = new WindowButton(PanelStyle::WINDOW_BUTTON_CLOSE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->Activated.connect(sigc::mem_fun(this, &WindowButtons::OnCloseClicked));
  but->OnMouseEnter.connect(lambda_statechanged);
  but->OnMouseLeave.connect(lambda_statechanged);
  but->OnMouseMove.connect(lambda_moved);

  but = new WindowButton(PanelStyle::WINDOW_BUTTON_MINIMIZE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->Activated.connect(sigc::mem_fun(this, &WindowButtons::OnMinimizeClicked));
  but->OnMouseEnter.connect(lambda_statechanged);
  but->OnMouseLeave.connect(lambda_statechanged);
  but->OnMouseMove.connect(lambda_moved);

  but = new WindowButton(PanelStyle::WINDOW_BUTTON_UNMAXIMIZE);
  AddView(but, 0, nux::eCenter, nux::eFix);
  but->Activated.connect(sigc::mem_fun(this, &WindowButtons::OnRestoreClicked));
  but->OnMouseEnter.connect(lambda_statechanged);
  but->OnMouseLeave.connect(lambda_statechanged);
  but->OnMouseMove.connect(lambda_moved);

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

const gchar*
WindowButtons::GetName()
{
  return "window-buttons";
}

const gchar*
WindowButtons::GetChildsName()
{
  return "";
}

void
WindowButtons::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder).add(GetGeometry());
}
