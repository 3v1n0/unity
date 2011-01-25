// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#include "Nux/Nux.h"
#include "Nux/Layout.h"
#include "Nux/WindowCompositor.h"
#include "Nux/VScrollBar.h"
#include "Nux/HScrollBar.h"
#include "Nux/Panel.h"
#include "PlacesGroup.h"
#include "PlacesResultsView.h"

PlacesResultsView::PlacesResultsView (NUX_FILE_LINE_DECL)
  :   ScrollView (NUX_FILE_LINE_PARAM)
  ,   _layout (0)
{
  m_horizontal_scrollbar_enable   = false;
  m_vertical_scrollbar_enable      = true;
  _layout = new nux::VLayout ("", NUX_TRACKER_LOCATION);
  setBorder (12);
  EnableVerticalScrollBar (true);

}

void
PlacesResultsView::Draw (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  nux::Geometry base = GetGeometry();

  if (_layout)
    _layout->NeedRedraw();

  nux::GetPainter().PaintBackground (GfxContext, base);

  if (m_vertical_scrollbar_enable)
  {
    vscrollbar->NeedRedraw();
  }

  GfxContext.PopClippingRectangle();
}

void
PlacesResultsView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  GfxContext.PushClippingRectangle (nux::Rect (m_ViewX, m_ViewY, m_ViewWidth, m_ViewHeight) );

  if (_layout)
  {
    GfxContext.PushClippingRectangle (_layout->GetGeometry() );
    _layout->ProcessDraw (GfxContext, force_draw);
    GfxContext.PopClippingRectangle();
  }

  GfxContext.PopClippingRectangle();

  if (m_vertical_scrollbar_enable)
  {
    vscrollbar->ProcessDraw (GfxContext, force_draw);
  }

  GfxContext.PopClippingRectangle();
}

void
PlacesResultsView::AddGroup (PlacesGroup *group)
{
  _layout->AddView (group, 0);
  ComputeChildLayout ();
}

void
PlacesResultsView::PositionChildLayout (float offsetX, float offsetY)
{
  ScrollView::PositionChildLayout (offsetX, offsetY);
}

void
PlacesResultsView::ScrollLeft (float stepx, int mousedx)
{
}

void
PlacesResultsView::ScrollRight (float stepx, int mousedx)
{
}

void
PlacesResultsView::ScrollUp (float stepy, int mousedy)
{
  ScrollView::ScrollUp (stepy, mousedy);
  ComputeChildLayout();
  NeedRedraw();
}

void
PlacesResultsView::ScrollDown (float stepy, int mousedy)
{
  ScrollView::ScrollDown (stepy, mousedy);
  ComputeChildLayout();
  NeedRedraw();
}
