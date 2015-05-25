// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Luke Yelavich <luke.yelavich@canonical.com>
 */

#include "ExpanderView.h"

#include <Nux/Layout.h>

namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(ExpanderView);

ExpanderView::ExpanderView(NUX_FILE_LINE_DECL)
  : nux::View(NUX_FILE_LINE_PARAM)
{
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(true);
}

void ExpanderView::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
}

void ExpanderView::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if (GetLayout())
    GetLayout()->ProcessDraw(graphics_engine, force_draw);
}

bool ExpanderView::AcceptKeyNavFocus()
{
  return true;
}

nux::Area* ExpanderView::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  if (TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type))
    return this;

  return nullptr;
}

} // namespace unity
