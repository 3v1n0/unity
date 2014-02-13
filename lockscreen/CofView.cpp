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

#include "CofView.h"

#include "config.h"

namespace unity
{
namespace lockscreen
{

CofView::CofView()
  // FIXME (andy) if we get an svg cof we can make it fullscreen independent.
  : IconTexture(PKGDATADIR"/cof.png", 66)
{}

nux::Area* CofView::FindAreaUnderMouse(nux::Point const& mouse_position,
                                       nux::NuxEventType event_type)
{
    return nullptr;
}

}
}
