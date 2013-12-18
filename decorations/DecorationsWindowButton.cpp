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

namespace unity
{
namespace decoration
{

Window::Impl::Button::Button(WindowButtonType type)
  : type_(type)
  , pressed_(false)
  , was_pressed_(false)
{
  auto cb = sigc::hide(sigc::mem_fun(this, &Button::UpdateTexture));
  mouse_owner.changed.connect(cb);
  focused.changed.connect(cb);
  UpdateTexture();
}

void Window::Impl::Button::UpdateTexture()
{
  texture_.SetTexture(manager_->impl_->GetButtonTexture(type_, GetCurrentState()));
  Damage();
}

WidgetState Window::Impl::Button::GetCurrentState() const
{
  if (focused())
  {
    if (mouse_owner() && pressed_)
    {
      return WidgetState::PRESSED;
    }
    else if (mouse_owner())
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
    else if (mouse_owner())
    {
      return WidgetState::BACKDROP_PRELIGHT;
    }
    else
    {
      return WidgetState::BACKDROP;
    }
  }
}

void Window::Impl::Button::ButtonDownEvent(CompPoint const& p, unsigned button)
{
  if (!pressed_ && button <= 3)
  {
    pressed_ = true;
    was_pressed_ = true;
    UpdateTexture();
  }
}

void Window::Impl::Button::ButtonUpEvent(CompPoint const& p, unsigned button)
{
  if (pressed_ && button <= 3)
  {
    pressed_ = false;
    UpdateTexture();
  }

  was_pressed_ = false;
}

void Window::Impl::Button::MotionEvent(CompPoint const& p)
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
