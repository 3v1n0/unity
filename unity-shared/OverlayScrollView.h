// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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

#ifndef _UNITY_SCROLL_VIEW_H_
#define _UNITY_SCROLL_VIEW_H_

#include <Nux/Nux.h>
#include "PlacesOverlayVScrollBar.h"

namespace unity
{
namespace dash
{

class ScrollView : public nux::ScrollView
{
public:
  ScrollView(NUX_FILE_LINE_PROTO);

  nux::RWProperty<double> scale;
  sigc::signal<void, ScrollDir> page_direction;

  using nux::ScrollView::SetVScrollBar;
};

} // dash namespace
} // unity namespace

#endif // _UNITY_SCROLL_VIEW_H_
