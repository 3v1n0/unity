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
  Hide ();

  _layout = new nux::VLayout (NUX_TRACKER_LOCATION);
  SetLayout (_layout);

  /* FIXME: Not needed this week */
  _search_bar = new PlacesSearchBar ();
  _search_bar->SetMinMaxSize (1024, 48);
  _layout->AddView (_search_bar, 0, nux::eCenter, nux::eFull);
  AddChild (_search_bar);
  
  _home_view = new PlacesHomeView ();
  _layout->AddView (_home_view, 1, nux::eCenter, nux::eFull);
  AddChild (_home_view);
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
  //window_event.e_x_root = base.x;
  //window_event.e_y_root = base.y;

  // The child layout get the Mouse down button only if the MouseDown happened inside the client view Area
  nux::Geometry viewGeometry = GetGeometry();

  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    if (!viewGeometry.IsPointInside (ievent.e_x - ievent.e_x_root, ievent.e_y - ievent.e_y_root))
    {
      ProcEvInfo = nux::eDoNotProcess;
    }
  }

  // hide if outside our window
  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    nux::Geometry home_geo (0, 0, 66, 24);

    if (!(GetGeometry ().IsPointInside (ievent.e_x, ievent.e_y))
        && !home_geo.IsPointInside (ievent.e_x, ievent.e_y))
    {
      Hide ();
      return nux::eMouseEventSolved;
    }
  }

  if (_layout)
    ret = _layout->ProcessEvent (window_event, ret, ProcessEventInfo);

  return ret;
}

void PlacesView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry ();
  // Coordinates inside the BaseWindow are relative to the top-left corner (0, 0). 
  //base.x = base.y = 0;

  GfxContext.PushClippingRectangle (base);

  nux::Color color (0.0, 0.0, 0.0, 0.9);
  // You can use this function to draw a colored Quad:
  //nux::GetPainter ().Paint2DQuadColor (GfxContext, GetGeometry (), color);
  // or this one:
  GfxContext.QRP_Color (0, 0, GetGeometry ().width, GetGeometry ().height, color);

  GfxContext.PopClippingRectangle ();
}


void PlacesView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry ();
  // Coordinates inside the BaseWindow are relative to the top-left corner (0, 0). 
  //base.x = base.y = 0;

  nux::GetPainter ().PushColorLayer (GfxContext, base, nux::Color (0.0, 0.0, 0.0, 0.9), true);
  if (_layout)
    _layout->ProcessDraw (GfxContext, force_draw);

  nux::GetPainter ().PopBackground ();
}

void PlacesView::PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw)
{
}

long
PlacesView::PostLayoutManagement (long LayoutResult)
{
  return nux::BaseWindow::PostLayoutManagement (LayoutResult);
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
