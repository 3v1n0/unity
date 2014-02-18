// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <sigc++/adaptors/hide.h>
#include "DecorationsWindowButton.h"
#include "DecorationsDataPool.h"

#include "unity-shared/UnitySettings.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace decoration
{

WindowButton::WindowButton(CompWindow* win, WindowButtonType type)
  : type_(type)
  , pressed_(false)
  , was_pressed_(false)
  , win_(win)
{
  auto cb = sigc::hide(sigc::mem_fun(this, &WindowButton::UpdateTexture));
  mouse_owner.changed.connect(cb);
  focused.changed.connect(cb);
  UpdateTexture();
}

void WindowButton::UpdateTexture()
{
  SetTexture(DataPool::Get()->ButtonTexture(type_, GetCurrentState()));
  UpdateTextureScale();
}

void WindowButton::UpdateTextureScale()
{
  WindowManager const& wm = WindowManager::Default();
  CompRect const& c_geo = Geometry();
  nux::Geometry geo(c_geo.x(), c_geo.y(), c_geo.width(), c_geo.height());

  int monitor = wm.MonitorGeometryIn(geo);
  float scale = unity::Settings::Instance().em(monitor).DPIScale();

  SetTextureScale(scale);
}

WidgetState WindowButton::GetCurrentState() const
{
  if (focused())
  {
    if (mouse_owner() && pressed_)
    {
      return WidgetState::PRESSED;
    }
    else if (mouse_owner() && !was_pressed_)
    {
      return WidgetState::PRELIGHT;
    }
    else
    {
      return WidgetState::NORMAL;
    }
  }
  else
  {
    if (mouse_owner() && pressed_)
    {
      return WidgetState::BACKDROP_PRESSED;
    }
    else if (mouse_owner() && !was_pressed_)
    {
      return WidgetState::BACKDROP_PRELIGHT;
    }
    else
    {
      return WidgetState::BACKDROP;
    }
  }
}

void WindowButton::ButtonDownEvent(CompPoint const& p, unsigned button, Time)
{
  if (!pressed_ && button <= Button3)
  {
    pressed_ = true;
    was_pressed_ = true;
    UpdateTexture();
  }
}

void WindowButton::ButtonUpEvent(CompPoint const& p, unsigned button, Time timestamp)
{
  if (pressed_ && button <= Button3)
  {
    pressed_ = false;
    UpdateTexture();

    switch (type_)
    {
      case WindowButtonType::CLOSE:
        if (win_->actions() & CompWindowActionCloseMask)
          win_->close(timestamp);
        break;
      case WindowButtonType::MINIMIZE:
        if (win_->actions() & CompWindowActionMinimizeMask)
          win_->minimize();
        break;
      case WindowButtonType::MAXIMIZE:
        switch (button)
        {
          case Button1:
            if ((win_->state() & CompWindowStateMaximizedVertMask) ||
                (win_->state() & CompWindowStateMaximizedHorzMask))
              win_->maximize(0);
            else if (win_->actions() & (CompWindowActionMaximizeHorzMask|CompWindowActionMaximizeVertMask))
              win_->maximize(MAXIMIZE_STATE);
            break;
          case Button2:
            if (win_->actions() & CompWindowActionMaximizeVertMask)
            {
              if (!(win_->state() & CompWindowStateMaximizedVertMask))
                win_->maximize(CompWindowStateMaximizedVertMask);
              else
                win_->maximize(0);
            }
            break;
          case Button3:
            if (win_->actions() & CompWindowActionMaximizeHorzMask)
            {
              if (!(win_->state() & CompWindowStateMaximizedHorzMask))
                win_->maximize(CompWindowStateMaximizedHorzMask);
              else
                win_->maximize(0);
            }
            break;
        }
        break;
      default:
        break;
    }
  }

  was_pressed_ = false;
}

void WindowButton::MotionEvent(CompPoint const& p, Time)
{
  if (pressed_)
  {
    if (!Geometry().contains(p))
    {
      pressed_ = false;
      UpdateTexture();
    }
  }
  else if (was_pressed_)
  {
    if (Geometry().contains(p))
    {
      pressed_ = true;
      UpdateTexture();
    }
  }
}

std::string WindowButton::GetName() const
{
  switch (type_)
  {
    case WindowButtonType::CLOSE:
      return "CloseWindowButton";
    case WindowButtonType::MINIMIZE:
      return "MinimizeWindowButton";
    case WindowButtonType::MAXIMIZE:
      return "MaximizeWindowButton";
    case WindowButtonType::UNMAXIMIZE:
      return "UnmaximizeWindowButton";
    default:
      return "WindowButton";
  }
}

void WindowButton::AddProperties(debug::IntrospectionData& data)
{
  TexturedItem::AddProperties(data);
  data.add("pressed", pressed_);

  switch(GetCurrentState())
  {
    case WidgetState::NORMAL:
      data.add("state", "normal");
      break;
    case WidgetState::PRELIGHT:
      data.add("state", "prelight");
      break;
    case WidgetState::PRESSED:
      data.add("state", "pressed");
      break;
    case WidgetState::DISABLED:
      data.add("state", "disabled");
      break;
    case WidgetState::BACKDROP:
      data.add("state", "backdrop");
      break;
    case WidgetState::BACKDROP_PRELIGHT:
      data.add("state", "backdrop_prelight");
      break;
    case WidgetState::BACKDROP_PRESSED:
      data.add("state", "backdrop_pressed");
      break;
    default:
      data.add("state", "unknown");
  }
}

} // decoration namespace
} // unity namespace
