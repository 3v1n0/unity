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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include <Nux/Nux.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>

namespace unity
{
namespace ui
{

class PointerBarrierWrapper
{
public:
  typedef std::shared_ptr<PointerBarrierWrapper> Ptr;

  nux::Property<int> x1;
  nux::Property<int> x2;
  nux::Property<int> y1;
  nux::Property<int> y2;

  nux::Property<int> threshold;

  nux::Property<bool> active;

  void ConstructBarrier();
  void DestroyBarrier();

private:
  PointerBarrier barrier;
};

}
}