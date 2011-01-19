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

NUX_IMPLEMENT_OBJECT_TYPE (PlacesView);

PlacesView::PlacesView (NUX_FILE_LINE_DECL)
:   nux::BaseWindow("", NUX_FILE_LINE_PARAM)
{
  SetMinMaxSize (1024, 600);
  Hide ();

  _layout = new nux::VLayout (NUX_TRACKER_LOCATION);

  _search_bar = new PlacesSearchBar ();
  _search_bar->SetMinMaxSize (1024, 48);
  _layout->AddView (_search_bar, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);

  SetLayout (_layout);
  AddChild (_search_bar);
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
      //ProcEvInfo = nux::eDoNotProcess;
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

  if (_layout)
  {
    ret = _layout->ProcessEvent (window_event, ret, ProcessEventInfo);
  }

  return ret;
}

void PlacesView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry ();
  // Coordinates inside the BaseWindow are relative to the top-left corner (0, 0). 
  base.x = base.y = 0;

  GfxContext.PushClippingRectangle (base);

  nux::Color color (0.0, 0.0, 0.0, 0.8);
  // You can use this function to draw a colored Quad:
  //nux::GetPainter ().Paint2DQuadColor (GfxContext, GetGeometry (), color);
  // or this one:
  GfxContext.QRP_Color (0, 0, GetGeometry ().width, GetGeometry ().height, color);

  GfxContext.PopClippingRectangle ();
}


void PlacesView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  if (_layout)
    _layout->ProcessDraw (GfxContext, force_draw);
}

void PlacesView::PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw)
{
}

long
PlacesView::PostLayoutManagement (long LayoutResult)
{
  return nux::BaseWindow::PostLayoutManagement (LayoutResult);
}



/*void PlacesView::ShowWindow (bool b, bool start_modal)
{
  nux::BaseWindow::ShowWindow (b, start_modal);
}*/

void PlacesView::Show ()
{
  if (IsVisible ())
    return;

  // FIXME: ShowWindow shouldn't need to be called first
  ShowWindow (true, false);
  EnableInputWindow (true, 1);
  GrabPointer ();
  GrabKeyboard ();
  NeedRedraw ();
}

void PlacesView::Hide ()
{
  if (!IsVisible ())
    return;

  CaptureMouseDownAnyWhereElse (false);
  ForceStopFocus (1, 1);
  UnGrabPointer ();
  UnGrabKeyboard ();
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
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}
