
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

#include "OverlayScrollView.h"
#include "PlacesOverlayVScrollBar.h"
#include "RawPixel.h"

namespace unity
{
namespace dash
{
namespace
{
  const RawPixel MOUSE_WHEEL_SCROLL_SIZE = 32_em;
}

ScrollView::ScrollView(NUX_FILE_LINE_DECL)
  : nux::ScrollView(NUX_FILE_LINE_PARAM)
{
  auto* scrollbar = new PlacesOverlayVScrollBar(NUX_TRACKER_LOCATION);
  SetVScrollBar(scrollbar);

  scale.SetGetterFunction([scrollbar] { return scrollbar->scale(); });
  scale.SetSetterFunction([scrollbar] (double scale) {
    if (scrollbar->scale() == scale)
      return false;

    scrollbar->scale = scale;
    return true;
  });

  m_MouseWheelScrollSize = MOUSE_WHEEL_SCROLL_SIZE.CP(scale);

  scale.changed.connect([this] (double scale) {
    m_MouseWheelScrollSize = MOUSE_WHEEL_SCROLL_SIZE.CP(scale);
  });

  page_direction.connect([scrollbar] (ScrollDir dir) {
    scrollbar->PerformPageNavigation(dir);
  });
}

} // dash namespace
} // unity namespace
