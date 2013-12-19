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

#include "DecorationsWindowButton.h"
#include "DecorationsDataPool.h"
#include <sigc++/sigc++.h>

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
  auto const& new_tex = DataPool::Get()->ButtonTexture(type_, GetCurrentState());

  if (texture_.st != new_tex)
  {
    texture_.SetTexture(new_tex);
    Damage();
  }
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

void WindowButton::ButtonDownEvent(CompPoint const& p, unsigned button)
{
  if (!pressed_ && button <= Button3)
  {
    pressed_ = true;
    was_pressed_ = true;
    UpdateTexture();
  }
}

void WindowButton::ButtonUpEvent(CompPoint const& p, unsigned button)
{
  if (pressed_ && button <= Button3)
  {
    pressed_ = false;
    UpdateTexture();

    switch (type_)
    {
      case WindowButtonType::CLOSE:
        if (win_->actions() & CompWindowActionCloseMask)
          win_->close(screen->getCurrentTime());
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

void WindowButton::MotionEvent(CompPoint const& p)
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

} // decoration namespace
} // unity namespace
