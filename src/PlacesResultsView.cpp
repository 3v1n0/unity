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
{
  m_horizontal_scrollbar_enable   = false;
  m_vertical_scrollbar_enable      = true;
  _layout = new nux::VLayout ("", NUX_TRACKER_LOCATION);

  _layout->SetContentDistribution(nux::MAJOR_POSITION_TOP);

  setBorder (12);
  EnableVerticalScrollBar (true);

  SetCompositionLayout (_layout);
}

PlacesResultsView::~PlacesResultsView ()
{
  _layout->Clear ();
}

void
PlacesResultsView::ReJiggyGroups ()
{
  _layout->Clear ();
 std::list<PlacesGroup *>::iterator it;

  for (it = _groups.begin(); it != _groups.end(); it++)
  {
    if ((*it)->IsVisible ())
    {
      _layout->AddView ((*it), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
    }
  }
}

void
PlacesResultsView::Draw (nux::GraphicsEngine &GfxContext, bool force_draw)
{
}

void
PlacesResultsView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  if (_layout)
    _layout->ProcessDraw (GfxContext, force_draw);

  if (m_vertical_scrollbar_enable)
    vscrollbar->ProcessDraw (GfxContext, force_draw);

  GfxContext.PopClippingRectangle();
}

void
PlacesResultsView::AddGroup (PlacesGroup *group)
{
  _groups.push_back (group);
  _layout->AddView (group, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
}

void
PlacesResultsView::RemoveGroup (PlacesGroup *group)
{
  _groups.remove (group);
  _layout->RemoveChildObject (group);
}

void
PlacesResultsView::PositionChildLayout (float offsetX, float offsetY)
{
  ScrollView::PositionChildLayout (offsetX, offsetY);
}


void PlacesResultsView::PreLayoutManagement()
{
  ScrollView::PreLayoutManagement();
}

long PlacesResultsView::PostLayoutManagement (long LayoutResult)
{
  long result = ScrollView::PostLayoutManagement (LayoutResult);
  return result;
}

long PlacesResultsView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  long ProcEvInfo = 0;

  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    if (!m_Geometry.IsPointInside (ievent.e_x - ievent.e_x_root, ievent.e_y - ievent.e_y_root) )
    {
      ProcEvInfo = nux::eDoNotProcess;
      //return TraverseInfo;
    }
  }

  if (m_vertical_scrollbar_enable)
    ret = vscrollbar->ProcessEvent (ievent, ret, ProcEvInfo);

  // The child layout get the Mouse down button only if the MouseDown happened inside the client view Area
  nux::Geometry viewGeometry = nux::Geometry (m_ViewX, m_ViewY, m_ViewWidth, m_ViewHeight);
  bool traverse = true;

  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    if (!viewGeometry.IsPointInside (ievent.e_x - ievent.e_x_root, ievent.e_y - ievent.e_y_root) )
    {
      ProcEvInfo = nux::eDoNotProcess;
      traverse = false;
    }
  }

  if (_layout)
    ret = _layout->ProcessEvent (ievent, ret, ProcEvInfo);

  ret = PostProcessEvent2 (ievent, ret, 0);
  return ret;
}

void PlacesResultsView::PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw)
{

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
}

void
PlacesResultsView::ScrollDown (float stepy, int mousedy)
{
  ScrollView::ScrollDown (stepy, mousedy);
}
