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

  _slider[STATE_OFF]  = NULL;
  _slider[STATE_OVER] = NULL;
  _slider[STATE_DOWN] = NULL;
  _track              = NULL;

  _state = STATE_OFF;
}

PlacesVScrollBar::~PlacesVScrollBar ()
{
  if (_slider[STATE_OFF])
    _slider[STATE_OFF]->UnReference ();

  if (_slider[STATE_OVER])
    _slider[STATE_OVER]->UnReference ();

  if (_slider[STATE_DOWN])
    _slider[STATE_DOWN]->UnReference ();

  if (_track)
    _track->UnReference ();
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
PlacesVScrollBar::PreLayoutManagement ()
{
  if (!_slider[STATE_OFF]  ||
      !_slider[STATE_OVER] ||
      !_slider[STATE_DOWN] ||
      !_track)
  {
    UpdateTexture ();
  }

  nux::VScrollBar::PreLayoutManagement ();
}

void
PlacesVScrollBar::Draw (nux::GraphicsEngine &gfxContext, bool force_draw)
{
  nux::Color         color = nux::Color::White;
  nux::Geometry      base  = GetGeometry ();
  nux::TexCoordXForm texxform;

  gfxContext.PushClippingRectangle (base);

  nux::GetPainter().PaintBackground (gfxContext, base);

  g_debug ("PlacesVScrollBar::Draw() - before");

  // check if textures have been computed... if they haven't, exit function
  if (!_slider[STATE_OFF])
    return;

  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates ().SetBlend (true);
  gfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

  base.OffsetPosition (0, PLACES_VSCROLLBAR_HEIGHT);
  base.OffsetSize (0, -2 * PLACES_VSCROLLBAR_HEIGHT);

  g_debug ("PlacesVScrollBar::Draw() - almost ");

  if (m_contentHeight > m_containerHeight)
  {
    g_debug ("PlacesVScrollBar::Draw() - trying to paint");
    nux::Geometry track_geo = m_Track->GetGeometry ();
    gfxContext.QRP_1Tex (track_geo.x,
                         track_geo.y,
                         track_geo.width,
                         track_geo.height,
                         _track->GetDeviceTexture (),
                         texxform,
                         color);

    nux::Geometry slider_geo = m_SlideBar->GetGeometry ();
    gfxContext.QRP_1Tex (slider_geo.x,
                         slider_geo.y,
                         slider_geo.width,
                         slider_geo.height,
                         _slider[_state]->GetDeviceTexture (),
                         texxform,
                         color);
  }

  gfxContext.GetRenderStates().SetBlend (false);
  gfxContext.PopClippingRectangle ();
}

void
PlacesVScrollBar::UpdateTexture ()
{
  nux::Color          transparent   = nux::Color (0.0f, 0.0f, 0.0f, 0.0f);
  int                 width         = 0;
  int                 height        = 0;
  nux::CairoGraphics* cairoGraphics = NULL;
  cairo_t*            cr            = NULL;
  nux::NBitmapData*   bitmap        = NULL;

  // update texture of off-state of slider
  width  = m_SlideBar->GetBaseWidth ();
  height = m_SlideBar->GetBaseHeight ();
  cairoGraphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairoGraphics->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 0.0f, 0.5f);
  cairo_rectangle (cr, 0.0f, 0.0f, (double) width, (double) height);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 0.0f, 1.0f);
  cairo_stroke (cr);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/slider_off.png");

  bitmap = cairoGraphics->GetBitmap ();

  if (_slider[STATE_OFF])
    _slider[STATE_OFF]->UnReference ();

  _slider[STATE_OFF] = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _slider[STATE_OFF]->Update (bitmap);

  cairo_destroy (cr);
  delete bitmap;
  delete cairoGraphics;

  // update texture of over-state of slider
  width  = m_SlideBar->GetBaseWidth ();
  height = m_SlideBar->GetBaseHeight ();
  cairoGraphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairoGraphics->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (cr, 1.0f, 0.0f, 0.0f, 0.5f);
  cairo_rectangle (cr, 0.0f, 0.0f, (double) width, (double) height);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 1.0f, 0.0f, 0.0f, 1.0f);
  cairo_stroke (cr);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/slider_over.png");

  bitmap = cairoGraphics->GetBitmap ();

  if (_slider[STATE_OVER])
    _slider[STATE_OVER]->UnReference ();

  _slider[STATE_OVER] = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _slider[STATE_OVER]->Update (bitmap);

  cairo_destroy (cr);
  delete bitmap;
  delete cairoGraphics;

  // update texture of down-state of slider
  width  = m_SlideBar->GetBaseWidth ();
  height = m_SlideBar->GetBaseHeight ();
  cairoGraphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairoGraphics->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (cr, 0.0f, 1.0f, 0.0f, 0.5f);
  cairo_rectangle (cr, 0.0f, 0.0f, (double) width, (double) height);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 0.0f, 1.0f, 0.0f, 1.0f);
  cairo_stroke (cr);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/slider_down.png");

  bitmap = cairoGraphics->GetBitmap ();

  if (_slider[STATE_DOWN])
    _slider[STATE_DOWN]->UnReference ();

  _slider[STATE_DOWN] = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _slider[STATE_DOWN]->Update (bitmap);

  cairo_destroy (cr);
  delete bitmap;
  delete cairoGraphics;

  // update texture of track
  width  = m_Track->GetBaseWidth ();
  height = m_Track->GetBaseHeight ();
  cairoGraphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairoGraphics->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (cr, 0.0f, 0.0f, 1.0f, 0.5f);
  cairo_rectangle (cr, 0.0f, 0.0f, (double) width, (double) height);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 0.0f, 0.0f, 1.0f, 1.0f);
  cairo_stroke (cr);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/track.png");

  bitmap = cairoGraphics->GetBitmap ();

  if (_track)
    _track->UnReference ();

  _track = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _track->Update (bitmap);

  cairo_destroy (cr);
  delete bitmap;
  delete cairoGraphics;
}
