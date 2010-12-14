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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#include "config.h"

#include "Nux/Nux.h"
#include "NuxGraphics/GLThread.h"
#include "UBusMessages.h"

#include "PlacesView.h"

PlacesView::PlacesView (NUX_FILE_LINE_DECL)
:   BaseWindow("", NUX_FILE_LINE_PARAM)
{
  SetBaseSize (800, 600);
  //SetWidth (800);
  //SetHeight (600);
  SetBaseX (0);
  SetBaseY (200);
  SetMaximumSize (600, 200);
  SetMinimumSize (600, 200);
  //SetGeometry (nux::Geometry (0, 0, 100, 100));
  IsVisible = false;
}

PlacesView::~PlacesView ()
{

}

long PlacesView::ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  long ProcEvInfo = 0;

  nux::IEvent window_event = ievent;
  nux::Geometry base = GetGeometry();
  window_event.e_x_root = base.x;
  window_event.e_y_root = base.y;

  // The child layout get the Mouse down button only if the MouseDown happened inside the client view Area
  nux::Geometry viewGeometry = GetGeometry();

  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    if (!viewGeometry.IsPointInside (ievent.e_x - ievent.e_x_root, ievent.e_y - ievent.e_y_root) )
    {
      ProcEvInfo = nux::eDoNotProcess;
    }
  }

  // hide if outside our window
  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    if (!(GetGeometry ().IsPointInside (ievent.e_x, ievent.e_y)))
    {
      Hide ();
      return nux::eMouseEventSolved;
    }
  }

  return ret;
}

void PlacesView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::GetPainter().PaintBackground (GfxContext, GetGeometry() );

  nux::Color *color = new nux::Color (0.9, 0.3, 0.1, 1.0);
  nux::GetPainter ().Paint2DQuadColor (GfxContext, GetGeometry (), *color);
}


void PlacesView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{

}

void PlacesView::PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw)
{

}

void PlacesView::ShowWindow (bool b, bool start_modal)
{
  BaseWindow::ShowWindow (b, start_modal);
}

void PlacesView::Show ()
{
  if (IsVisible)
    return;

  IsVisible = true;
  // FIXME: ShowWindow shouldn't need to be called first
  ShowWindow (true, false);
  EnableInputWindow (true, 1);
  GrabPointer ();
  SetBaseX (0);
  SetBaseY (0);
  NeedRedraw ();
}

void PlacesView::Hide ()
{
  if (!IsVisible)
    return;

  IsVisible = false;
  CaptureMouseDownAnyWhereElse (false);
  ForceStopFocus (1, 1);
  UnGrabPointer ();
  EnableInputWindow (false);
  ShowWindow (false, false);
}


/* Introspection */
const gchar *
PlacesView::GetName ()
{
  return "PlacesView";
}

void
PlacesView::AddProperties (GVariantBuilder *builder)
{
}
