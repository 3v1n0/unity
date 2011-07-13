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

#include "config.h"

#include "TextureCache.h"
#include "Nux/Nux.h"
#include "PlacesTile.h"

#define PADDING 8
#define BLUR_SIZE 6

PlacesTile::PlacesTile (NUX_FILE_LINE_DECL, const void *id) :
  View (NUX_FILE_LINE_PARAM),
  _id (id),
  _hilight_background (NULL),
  _hilight_layer (NULL),
  _last_width (0),
  _last_height (0)
{
  OnMouseDown.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseDown));
  OnMouseUp.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseUp));
  OnMouseClick.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseClick));
  OnMouseEnter.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseEnter));
  OnMouseLeave.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseLeave));
  OnKeyNavFocusChange.connect (sigc::mem_fun (this, &PlacesTile::OnFocusChanged));
  OnKeyNavFocusActivate.connect (sigc::mem_fun (this, &PlacesTile::OnFocusActivated));
  _can_pass_focus_to_composite_layout = false;
}

PlacesTile::~PlacesTile ()
{
  if (_hilight_background)
  {
    _hilight_background->UnReference ();
    _hilight_background = NULL;
    con_obj.disconnect ();
  }

  if (_hilight_layer)
  {
    delete _hilight_layer;
    _hilight_layer = NULL;
  }
}

const void *
PlacesTile::GetId ()
{
  return _id;
}

void
PlacesTile::OnFocusChanged (nux::Area *label)
{
  QueueDraw ();
}

nux::Geometry
PlacesTile::GetHighlightGeometry ()
{
  return GetGeometry ();
}

void
PlacesTile::DrawHighlight (const char *texid, int width, int height, nux::BaseTexture **texture)
{
  nux::Geometry base = GetGeometry ();
  nux::Geometry highlight_geo = GetHighlightGeometry ();
  
  nux::CairoGraphics *cairo_graphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32,
                                                               highlight_geo.width + PADDING + (BLUR_SIZE*3),
                                                               highlight_geo.height + PADDING + (BLUR_SIZE*3));
  cairo_t *cr = cairo_graphics->GetContext();

  cairo_scale (cr, 1.0f, 1.0f);

  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  
  int bg_width = highlight_geo.width + PADDING + 1;
  int bg_height = highlight_geo.height + PADDING +1;
 	int bg_x = BLUR_SIZE-1;
 	int bg_y = BLUR_SIZE-1;
	
  // draw the glow
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width (cr, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.75f);
  cairo_graphics->DrawRoundedRectangle (cr,
                                       1.0f,
                                       bg_x,
                                       bg_y,
                                       5.0,
                                       bg_width,
                                       bg_height,
                                       true);
  cairo_fill (cr);
  cairo_graphics->BlurSurface (BLUR_SIZE - 2);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_graphics->DrawRoundedRectangle (cr,
                                        1.0,
                                        bg_x,
                                        bg_y,
                                        5.0,
                                        bg_width,
                                        bg_height,
                                        true);
  cairo_clip (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_graphics->DrawRoundedRectangle (cr,
                                        1.0,
                                        bg_x,
                                        bg_y,
                                        5.0,
                                        bg_width,
                                        bg_height,
                                        true);
  cairo_set_source_rgba (cr, 240/255.0f, 240/255.0f, 240/255.0f, 1.0f);
  cairo_fill_preserve (cr);

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0);
  cairo_stroke (cr); 
  
  cairo_destroy (cr);

  nux::NBitmapData *bitmap =  cairo_graphics->GetBitmap();
  nux::BaseTexture *tex = nux::GetGraphicsDisplay ()->GetGpuDevice ()->CreateSystemCapableTexture ();
  tex->Update (bitmap);
  *texture = tex;

  delete bitmap;
  delete cairo_graphics;
}

void
PlacesTile::OnDestroyNotify (nux::Trackable *Object)
{
  g_warning ("Texture destroyed before we were ready");
  _hilight_background = NULL;
  UpdateBackground ();
}


void
PlacesTile::UpdateBackground ()
{
  nux::Geometry base = GetGeometry ();
  nux::Geometry highlight_geo = GetHighlightGeometry ();

  if ((base.width == _last_width) && (base.height == _last_height))
    return;

  _last_width = base.width;
  _last_height = base.height;

  // try and get a texture from the texture cache
  TextureCache *cache = TextureCache::GetDefault ();
  nux::BaseTexture * hilight_tex = cache->FindTexture ("PlacesTile.HilightTexture",
                                                       highlight_geo.width, highlight_geo.height,
                                                       sigc::mem_fun (this, &PlacesTile::DrawHighlight));

  if (_hilight_background)
  {
    _hilight_background->UnReference ();
    _hilight_background = NULL;
    con_obj.disconnect ();
  }

  con_obj = hilight_tex->OnDestroyed.connect (sigc::mem_fun (this, &PlacesTile::OnDestroyNotify));

  _hilight_background = hilight_tex;
  _hilight_background->Reference ();

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
                                          nux::color::White,
                                          true,
                                          rop);
}

long
PlacesTile::ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  return PostProcessEvent2 (ievent, TraverseInfo, ProcessEventInfo);
}

nux::Area*
PlacesTile::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
    bool mouse_inside = TestMousePointerInclusion(mouse_position, event_type);

    if(mouse_inside == false)
      return NULL;

    return this;
}

void
PlacesTile::Draw (nux::GraphicsEngine& gfxContext,
                       bool                 forceDraw)
{
  nux::Geometry base = GetGeometry ();

  nux::GetPainter ().PaintBackground (gfxContext, base);

  gfxContext.PushClippingRectangle (base);

  if (HasKeyFocus () || IsMouseInside ())
  {
    UpdateBackground ();
    nux::Geometry hl_geo = GetHighlightGeometry ();
    nux::Geometry total_highlight_geo = nux::Geometry (base.x + hl_geo.x - (PADDING)/2 - BLUR_SIZE,
                                                       base.y + hl_geo.y - (PADDING)/2 - BLUR_SIZE,
                                                       hl_geo.width + PADDING + BLUR_SIZE*2,
                                                       hl_geo.height + PADDING + BLUR_SIZE*2);

    _hilight_layer->SetGeometry (total_highlight_geo);
    nux::GetPainter ().RenderSinglePaintLayer (gfxContext, total_highlight_geo, _hilight_layer);
  }

  gfxContext.PopClippingRectangle ();
}

void
PlacesTile::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry ();

  GfxContext.PushClippingRectangle (base);

  if (HasKeyFocus () || IsMouseInside ())
  {
    UpdateBackground ();

    nux::Geometry hl_geo = GetHighlightGeometry ();
    nux::Geometry total_highlight_geo = nux::Geometry (base.x + hl_geo.x - (PADDING)/2 - BLUR_SIZE,
                                                       base.y + hl_geo.y - (PADDING)/2 - BLUR_SIZE,
                                                       hl_geo.width + PADDING + BLUR_SIZE*2,
                                                       hl_geo.height + PADDING + BLUR_SIZE*2);

    nux::GetPainter ().PushLayer (GfxContext, total_highlight_geo, _hilight_layer);
  }

  if (GetCompositionLayout ())
    GetCompositionLayout ()->ProcessDraw (GfxContext, force_draw);

  if (IsMouseInside () || HasKeyFocus ())
    nux::GetPainter ().PopBackground ();

  GfxContext.PopClippingRectangle();
}

void
PlacesTile::RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (nux::GetEventButton (button_flags) == 1)
    sigClick.emit (this);
  QueueDraw ();
}

void
PlacesTile::RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  QueueDraw ();
}

void
PlacesTile::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  QueueDraw ();
}

void
PlacesTile::RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  QueueDraw ();
}

void
PlacesTile::RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  QueueDraw ();
}

void
PlacesTile::OnFocusActivated (nux::Area *label)
{
  sigClick.emit (this);
}
