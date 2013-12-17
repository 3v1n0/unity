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
    return mouse_owner() ? WidgetState::PRELIGHT : WidgetState::NORMAL;
  }
  else
  {
    return mouse_owner() ? WidgetState::BACKDROP_PRELIGHT : WidgetState::BACKDROP;
  }
}

} // decoration namespace
} // unity namespace
