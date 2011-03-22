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

const int PLACES_VSCROLLBAR_WIDTH  = 10;
const int PLACES_VSCROLLBAR_HEIGHT = 10;
const int BLUR_SIZE                =  7;

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

  _drag    = false;
  _entered = false;

  _slider[STATE_OFF]  = NULL;
  _slider[STATE_OVER] = NULL;
  _slider[STATE_DOWN] = NULL;
  _track              = NULL;

  _state = STATE_OFF;

  m_SlideBar->SetMinimumSize (PLACES_VSCROLLBAR_WIDTH + 2 * BLUR_SIZE,
                              PLACES_VSCROLLBAR_HEIGHT + 2 * BLUR_SIZE);
  m_Track->SetMinimumSize (PLACES_VSCROLLBAR_WIDTH + 2 * BLUR_SIZE,
                           PLACES_VSCROLLBAR_HEIGHT + 2 * BLUR_SIZE);
  SetMinimumSize (PLACES_VSCROLLBAR_WIDTH + 2 * BLUR_SIZE,
                  PLACES_VSCROLLBAR_HEIGHT + 2 * BLUR_SIZE);
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
  _entered = true;
  if (!_drag)
  {
    _state = STATE_OVER;
    NeedRedraw ();
  }
}

void
PlacesVScrollBar::RecvMouseLeave (int           x,
                                  int           y,
                                  unsigned long button_flags,
                                  unsigned long key_flags)
{
  g_debug ("PlacesVScrollBar::RecvMouseLeave() called");
  _entered = false;
  if (!_drag)
  {
    _state = STATE_OFF;
    NeedRedraw ();
  }
}

void
PlacesVScrollBar::RecvMouseDown (int           x,
                                 int           y,
                                 unsigned long button_flags,
                                 unsigned long key_flags)
{
  g_debug ("PlacesVScrollBar::RecvMouseDown() called");
  _state = STATE_DOWN;
  NeedRedraw ();
}

void
PlacesVScrollBar::RecvMouseUp (int           x,
                               int           y,
                               unsigned long button_flags,
                               unsigned long key_flags)
{
  g_debug ("PlacesVScrollBar::RecvMouseUp() called");
  _drag = false;
  if (_entered)
    _state = STATE_OVER;
  else
    _state = STATE_OFF;
  NeedRedraw ();
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
  _drag = true;
  NeedRedraw ();
}

void
PlacesVScrollBar::PreLayoutManagement ()
{
  UpdateTexture ();

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

  // check if textures have been computed... if they haven't, exit function
  if (!_slider[STATE_OFF])
    return;

  //texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates ().SetBlend (true);
  gfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

  //base.OffsetPosition (0, PLACES_VSCROLLBAR_HEIGHT);
  //base.OffsetSize (0, -2 * PLACES_VSCROLLBAR_HEIGHT);

  if (m_contentHeight > m_containerHeight)
  {
    nux::Geometry track_geo = m_Track->GetGeometry ();
    gfxContext.QRP_1Tex (track_geo.x,
                         track_geo.y,
                         track_geo.width,
                         track_geo.height,
                         _track->GetDeviceTexture (),
                         texxform,
                         color);

    nux::Geometry slider_geo = m_SlideBar->GetGeometry ();
    gfxContext.QRP_1Tex (slider_geo.x - BLUR_SIZE - 2,
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
  double              half_height   = 0.0f;

  // update texture of off-state of slider
  width  = m_SlideBar->GetBaseWidth ();
  height = m_SlideBar->GetBaseHeight ();
  cairoGraphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, width, height);
  width  -= 2 * BLUR_SIZE;
  height -= 2 * BLUR_SIZE;

  cr = cairoGraphics->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width (cr, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.75f);
  cairoGraphics->DrawRoundedRectangle (cr,
                                       1.0f,
                                       BLUR_SIZE + 1.5f,
                                       BLUR_SIZE + 1.5f,
                                       (double) (width - 1) / 2.0f,
                                       (double) width - 3.0f,
                                       (double) height - 3.0f);
  cairo_fill_preserve (cr);
  cairoGraphics->BlurSurface (BLUR_SIZE - 3);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (cr, 0.125f, 0.125f, 0.125f, 0.75f);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.5f);
  cairo_stroke (cr);

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  half_height = (double) (height + 2 * BLUR_SIZE) / 2.0f;
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height - 2.0f);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height - 2.0f);
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height);
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height + 2.0f);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height + 2.0f);
  cairo_stroke (cr);

  //cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/slider_off.png");

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
  width  -= 2 * BLUR_SIZE;
  height -= 2 * BLUR_SIZE;

  cr = cairoGraphics->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width (cr, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.25f);
  cairoGraphics->DrawRoundedRectangle (cr,
                                       1.0f,
                                       BLUR_SIZE + 1.5f,
                                       BLUR_SIZE + 1.5f,
                                       (double) (width - 1) / 2.0f,
                                       (double) width - 3.0f,
                                       (double) height - 3.0f);

  cairo_fill_preserve (cr);
  cairoGraphics->BlurSurface (BLUR_SIZE - 3);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_surface_t* tmp_surf = NULL;
  cairo_t*         tmp_cr   = NULL;
  tmp_surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 6, 4);
  tmp_cr = cairo_create (tmp_surf);
  cairo_set_line_width (tmp_cr, 1.0f);
  cairo_set_antialias (tmp_cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_operator (tmp_cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (tmp_cr);
  cairo_set_operator (tmp_cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (tmp_cr, 1.0f, 1.0f, 1.0f, 0.25f);
  cairo_paint (tmp_cr);
  cairo_set_source_rgba (tmp_cr, 1.0f, 1.0f, 1.0f, 0.5f);
  cairo_rectangle (tmp_cr, 0.f, 0.f, 1.0f, 1.0f);
  cairo_rectangle (tmp_cr, 1.f, 1.f, 1.0f, 1.0f);
  cairo_rectangle (tmp_cr, 2.f, 2.f, 1.0f, 1.0f);
  cairo_rectangle (tmp_cr, 3.f, 3.f, 1.0f, 1.0f);
  cairo_rectangle (tmp_cr, 4.f, 0.f, 1.0f, 1.0f);
  cairo_rectangle (tmp_cr, 5.f, 1.f, 1.0f, 1.0f);
  cairo_fill (tmp_cr);
  cairo_destroy (tmp_cr);
  cairo_set_source_surface (cr, tmp_surf, BLUR_SIZE + 1.5f, BLUR_SIZE + 1.5f);
  cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
  //cairo_surface_write_to_png (tmp_surf, "/tmp/tmp_surf.png");

  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.5f);
  cairo_stroke (cr);

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height - 2.0f);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height - 2.0f);
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height);
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height + 2.0f);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height + 2.0f);
  cairo_stroke (cr);

  //cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/slider_over.png");

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
  width  -= 2 * BLUR_SIZE;
  height -= 2 * BLUR_SIZE;
  cr = cairoGraphics->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width (cr, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairoGraphics->DrawRoundedRectangle (cr,
                                       1.0f,
                                       BLUR_SIZE + 1.5f,
                                       BLUR_SIZE + 1.5f,
                                       (double) (width - 1) / 2.0f,
                                       (double) width - 3.0f,
                                       (double) height - 3.0f);
  cairo_fill_preserve (cr);
  cairoGraphics->BlurSurface (BLUR_SIZE - 3);
  cairo_fill (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height - 2.0f);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height - 2.0f);
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height);
  cairo_move_to (cr, BLUR_SIZE + 2.5f, half_height + 2.0f);
  cairo_line_to (cr, BLUR_SIZE + 2.5f + 5.0f, half_height + 2.0f);
  cairo_stroke (cr);

  //cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/slider_down.png");

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
  width  -= 2 * BLUR_SIZE;
  height -= 2 * BLUR_SIZE;
  cr = cairoGraphics->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width (cr, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.5f);
  cairoGraphics->DrawRoundedRectangle (cr,
                                       1.0f,
                                       BLUR_SIZE + 0.5f,
                                       BLUR_SIZE + 0.5f,
                                       (double) width / 2.0f,
                                       (double) width - 1.0f,
                                       (double) height - 1.0f);
  cairo_fill_preserve (cr);
  cairoGraphics->BlurSurface (BLUR_SIZE - 3);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.25f);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.35f);
  cairo_stroke (cr);
  //cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/track.png");

  bitmap = cairoGraphics->GetBitmap ();

  if (_track)
    _track->UnReference ();

  _track = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _track->Update (bitmap);

  cairo_destroy (cr);
  delete bitmap;
  delete cairoGraphics;
}
