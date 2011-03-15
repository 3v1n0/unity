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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#include "Nux/Nux.h"
#include "PlacesVScrollBar.h"

const int PLACES_VSCROLLBAR_WIDTH  = 5;
const int PLACES_VSCROLLBAR_HEIGHT = 10;

PlacesVScrollBar::PlacesVScrollBar (NUX_FILE_LINE_DECL)
  : VScrollBar (NUX_FILE_LINE_PARAM)
{
  /*m_Track->OnMouseDown.connect (sigc::mem_fun (this,
                                               &PlacesVScrollBar::RecvMouseDown));
  m_Track->OnMouseUp.connect (sigc::mem_fun (this,
                                             &PlacesVScrollBar::RecvMouseUp));*/

  m_SlideBar->OnMouseEnter.connect (sigc::mem_fun (this,
                                 &PlacesVScrollBar::RecvMouseEnter));

  m_SlideBar->OnMouseLeave.connect (sigc::mem_fun (this,
                                 &PlacesVScrollBar::RecvMouseLeave));

  m_SlideBar->OnMouseDown.connect (sigc::mem_fun (this,
                                &PlacesVScrollBar::RecvMouseDown));

  m_SlideBar->OnMouseUp.connect (sigc::mem_fun (this,
                              &PlacesVScrollBar::RecvMouseUp));

  m_SlideBar->OnMouseDrag.connect (sigc::mem_fun (this,
                                &PlacesVScrollBar::RecvMouseDrag));

  _state = STATE_OFF;
}

PlacesVScrollBar::~PlacesVScrollBar ()
{

}

void
PlacesVScrollBar::RecvMouseEnter (int           x,
                                  int           y,
                                  unsigned long button_flags,
                                  unsigned long key_flags)
{
  g_debug ("PlacesVScrollBar::RecvMouseEnter() called");
  _state = STATE_OVER;
  QueueDraw ();
}

void
PlacesVScrollBar::RecvMouseLeave (int           x,
                                  int           y,
                                  unsigned long button_flags,
                                  unsigned long key_flags)
{
  g_debug ("PlacesVScrollBar::RecvMouseLeave() called");
  _state = STATE_OFF;
  QueueDraw ();
}

void
PlacesVScrollBar::RecvMouseDown (int           x,
                                 int           y,
                                 unsigned long button_flags,
                                 unsigned long key_flags)
{
  g_debug ("PlacesVScrollBar::RecvMouseDown() called");
  _state = STATE_DOWN;
  QueueDraw ();
}

void
PlacesVScrollBar::RecvMouseUp (int           x,
                               int           y,
                               unsigned long button_flags,
                               unsigned long key_flags)
{
  g_debug ("PlacesVScrollBar::RecvMouseUp() called");
  _state = STATE_OVER;
  QueueDraw ();
}

void
PlacesVScrollBar::RecvMouseDrag (int           x,
                                 int           y,
                                 int           dx,
                                 int           dy,
                                 unsigned long button_flags,
                                 unsigned long key_flags)
{
  g_debug ("PlacesVScrollBar::RecvMouseDrag() called");
  QueueDraw ();
}

void
PlacesVScrollBar::Draw (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  nux::GetPainter().PaintBackground (GfxContext, base);

  base.OffsetPosition (0, PLACES_VSCROLLBAR_HEIGHT);
  base.OffsetSize (0, -2 * PLACES_VSCROLLBAR_HEIGHT);

  nux::Color color;

  switch (_state)
  {
    case STATE_OFF:
      color.SetRGBA (1.0f, 0.0f, 0.0f, 1.0f);
    break;

    case STATE_OVER:
      color.SetRGBA (0.0f, 1.0f, 0.0f, 1.0f);
    break;

    case STATE_DOWN:
      color.SetRGBA (0.0f, 0.0f, 1.0f, 1.0f);
    break;

    default:
      color.SetRGBA (0.5f, 0.5f, 0.5f, 1.0f);
    break;
  }

  if (m_contentHeight > m_containerHeight)
  {
    nux::Geometry slider_geo = m_SlideBar->GetGeometry ();
    GfxContext.QRP_Color (slider_geo.x,
                          slider_geo.y,
                          slider_geo.width,
                          slider_geo.height,
                          color);
  }
}
