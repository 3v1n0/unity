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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "LockScreenShield.h"

#include <NuxCore/Color.h> // FIXME: remove this

namespace unity 
{
namespace lockscreen
{

Shield::Shield(bool is_primary)
  : primary(is_primary)
{
  SetBackgroundColor(is_primary ?  nux::color::Red : nux::color::Yellow);

  mouse_enter.connect(sigc::mem_fun(this, &Shield::OnMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &Shield::OnMouseLeave));
  primary.changed.connect(sigc::mem_fun(this, &Shield::OnPrimaryChanged));
}

void Shield::OnMouseEnter(int /*x*/, int /*y*/, unsigned long /**/, unsigned long /**/)
{
  primary = true;
}

void Shield::OnMouseLeave(int /*x*/, int /**/, unsigned long /**/, unsigned long /**/)
{
  primary = false;
}

void Shield::OnPrimaryChanged(bool value)
{
  SetBackgroundColor(value ?  nux::color::Red : nux::color::Yellow);
  QueueDraw();
}

}
}
