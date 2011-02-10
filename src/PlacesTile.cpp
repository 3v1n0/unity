/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include "Nux/Nux.h"
#include "PlacesTile.h"
PlacesTile::PlacesTile (NUX_FILE_LINE_DECL) :
View (NUX_FILE_LINE_PARAM),
_hilight_background (NULL),
_hilight_layer (NULL),
_last_width (0),
_last_height (0)
{
  _state = STATE_DEFAULT;

  OnMouseDown.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseDown));
  OnMouseUp.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseUp));
  OnMouseClick.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseClick));
  OnMouseMove.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseMove));
  //OnMouseDrag.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseDrag));
  OnMouseEnter.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseEnter));
  OnMouseLeave.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseLeave));
  _hilight_view = this;
}

PlacesTile::~PlacesTile ()
{
}

void
PlacesTile::UpdateBackground ()
{
  nux::Geometry base = GetGeometry ();

  if ((base.width == _last_width) && (base.height == _last_height))
    return;

  _last_width = base.width;
  _last_height = base.height;

  nux::CairoGraphics *cairo_graphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, base.width, base.height);
  cairo_t *cr = cairo_graphics->GetContext();

  cairo_scale (cr, 1.0f, 1.0f);

  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

  DrawRoundedRectangle (cr, 1.0, 6.0, 6.0, 15.0, base.width - 12 , base.height - 12);
  cairo_set_source_rgba (cr, 0.25, 0.25, 0.25, 0.25);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 1, 1, 1, 1.0);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);

  cairo_destroy (cr);

  nux::NBitmapData* bitmap =  cairo_graphics->GetBitmap();
  
  if (_hilight_background)
    _hilight_background->UnReference ();

  _hilight_background = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _hilight_background->Update (bitmap);
  delete bitmap;

  nux::ROPConfig rop; 
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  
  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  if (_hilight_layer)
    delete _hilight_layer;

  _hilight_layer = new nux::TextureLayer (_hilight_background->GetDeviceTexture(),
                                     texxform,
                                     nux::Color::White,
                                     true,
                                     rop);
  _hilight_background->UnReference ();
  _hilight_background = NULL;
  delete cairo_graphics;
}

static inline double
_align (double val)
{
  double fract = val - (int) val;
  if (fract != 0.5f)
    return (double) ((int) val + 0.5f);
  else
    return val;
}

void
PlacesTile::DrawRoundedRectangle (cairo_t* cr,
                                  double   aspect,
                                  double   x,
                                  double   y,
                                  double   cornerRadius,
                                  double   width,
                                  double   height)
{
  double radius = cornerRadius / aspect;

  // top-left, right of the corner
  cairo_move_to (cr, _align (x + radius), _align (y));

  // top-right, left of the corner
  cairo_line_to (cr, _align (x + width - radius), _align (y));

  // top-right, below the corner
  cairo_arc (cr,
             _align (x + width - radius),
             _align (y + radius),
             radius,
             -90.0f * G_PI / 180.0f,
             0.0f * G_PI / 180.0f);

  // bottom-right, above the corner
  cairo_line_to (cr, _align (x + width), _align (y + height - radius));

  // bottom-right, left of the corner
  cairo_arc (cr,
             _align (x + width - radius),
             _align (y + height - radius),
             radius,
             0.0f * G_PI / 180.0f,
             90.0f * G_PI / 180.0f);

  // bottom-left, right of the corner
  cairo_line_to (cr, _align (x + radius), _align (y + height));

  // bottom-left, above the corner
  cairo_arc (cr,
             _align (x + radius),
             _align (y + height - radius),
             radius,
             90.0f * G_PI / 180.0f,
             180.0f * G_PI / 180.0f);

  // top-left, right of the corner
  cairo_arc (cr,
             _align (x + radius),
             _align (y + radius),
             radius,
             180.0f * G_PI / 180.0f,
             270.0f * G_PI / 180.0f);
}


long PlacesTile::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  ret = PostProcessEvent2 (ievent, ret, ProcessEventInfo);
  return ret;
}

void PlacesTile::Draw (nux::GraphicsEngine& gfxContext,
                       bool                 forceDraw)
{
  UpdateBackground ();

  nux::Geometry base = GetGeometry ();
  gfxContext.PushClippingRectangle (base);

  nux::GetPainter ().PaintBackground (gfxContext, GetGeometry ());

  if (_state == STATE_HOVER)
  {
    gPainter.PushDrawLayer (gfxContext, GetGeometry (), _hilight_layer);
    gPainter.PopBackground ();
  }

  gfxContext.PopClippingRectangle ();
}

void PlacesTile::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  UpdateBackground ();
  GfxContext.PushClippingRectangle (GetGeometry() );

  if (_state == STATE_HOVER)
    nux::GetPainter ().PushLayer (GfxContext, GetGeometry (), _hilight_layer);

  _layout->ProcessDraw (GfxContext, force_draw);
  
  if (_state == STATE_HOVER)
    nux::GetPainter ().PopBackground ();

  GfxContext.PopClippingRectangle();
}

void PlacesTile::PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw)
{

}

void
PlacesTile::PreLayoutManagement ()
{
  nux::View::PreLayoutManagement ();
}

long
PlacesTile::PostLayoutManagement (long LayoutResult)
{
  return nux::View::PostLayoutManagement (LayoutResult);
}

void PlacesTile::SetState (TileState state)
{
  _state = state;
  NeedRedraw ();
}

PlacesTile::TileState PlacesTile::GetState ()
{
  return _state;
}

void PlacesTile::RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  sigClick.emit(this);

  NeedRedraw();
  _layout->NeedRedraw ();
}

void PlacesTile::RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  NeedRedraw();
  _layout->NeedRedraw ();
}

void PlacesTile::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _state = STATE_PRESSED;
  NeedRedraw();
  _layout->NeedRedraw ();
}

void PlacesTile::RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{

}

void PlacesTile::RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetState (STATE_HOVER);
  NeedRedraw();
  _layout->NeedRedraw ();
}

void PlacesTile::RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetState (STATE_DEFAULT);
  NeedRedraw();
  _layout->NeedRedraw ();
}


