// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <Nux/Layout.h>
#include <Nux/WindowCompositor.h>

#include <NuxImage/CairoGraphics.h>
#include <NuxImage/ImageSurface.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>

#include "PlacesHomeView.h"

PlacesHomeView::PlacesHomeView (NUX_FILE_LINE_DECL)
:   View (NUX_FILE_LINE_PARAM)
{
  _bg_layer = new nux::ColorLayer (nux::Color (0xff999893), true);

  _layout = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  SetCompositionLayout (_layout);
 
  for (int i = 0; i < 8; i++)
  {
    nux::ColorLayer color (nux::Color::RandomColor ());
    nux::TextureArea* texture_area = new nux::TextureArea ();
    texture_area->SetPaintLayer (&color);

    _layout->AddView (texture_area, 1, nux::eLeft, nux::eFull);
  }

  _layout->ForceChildrenSize (true);
  _layout->SetChildrenSize (186, 186);
  _layout->EnablePartialVisibility (false);

  _layout->SetVerticalExternalMargin (48);
  _layout->SetHorizontalExternalMargin (48);
  _layout->SetVerticalInternalMargin (32);
  _layout->SetHorizontalInternalMargin (32);
}

PlacesHomeView::~PlacesHomeView ()
{
  delete _bg_layer;
}

const gchar* PlacesHomeView::GetName ()
{
	return "PlacesHomeView";
}

const gchar *
PlacesHomeView::GetChildsName ()
{
  return "";
}

void PlacesHomeView::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}

long
PlacesHomeView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  nux::Geometry geo = GetGeometry ();

  long ret = TraverseInfo;
  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);

  return ret;
}

void
PlacesHomeView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::GetPainter().PaintBackground (GfxContext, GetGeometry() );

  _bg_layer->SetGeometry (GetGeometry ());
  nux::GetPainter().RenderSinglePaintLayer (GfxContext, GetGeometry(), _bg_layer);
}

void
PlacesHomeView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  gPainter.PushLayer (GfxContext, GetGeometry (), _bg_layer);
  
  _layout->ProcessDraw (GfxContext, force_draw);

  gPainter.PopBackground ();
  GfxContext.PopClippingRectangle();
}

void
PlacesHomeView::PreLayoutManagement ()
{
  nux::View::PreLayoutManagement ();
}

long
PlacesHomeView::PostLayoutManagement (long LayoutResult)
{
  // I'm imagining this is a good as time as any to update the background
  UpdateBackground ();

  return nux::View::PostLayoutManagement (LayoutResult);
}

void
PlacesHomeView::UpdateBackground ()
{
#define PADDING 24
#define RADIUS  12
  int x, y, width, height;
  nux::Geometry geo = GetGeometry ();

  if (geo.width == _last_width && geo.height == _last_height)
    return;

  _last_width = geo.width;
  _last_height = geo.height;

  x = y = PADDING;
  width = _last_width - (PADDING);
  height = _last_height - (PADDING);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, _last_width, _last_height);
  cairo_t *cr = cairo_graphics.GetContext();

  cairo_translate (cr, 0.5, 0.5);
  cairo_set_line_width (cr, 1.0);

  cairo_set_source_rgba (cr, 0.5f, 0.5f, 0.5f, 0.2f);

  cairo_move_to  (cr, x, y + RADIUS);
  cairo_curve_to (cr, x, y, x, y, x + RADIUS, y);
  cairo_line_to  (cr, width - RADIUS, y);
  cairo_curve_to (cr, width, y, width, y, width, y + RADIUS);
  cairo_line_to  (cr, width, height - RADIUS);
  cairo_curve_to (cr, width, height, width, height, width - RADIUS, height);
  cairo_line_to  (cr, x + RADIUS, height);
  cairo_curve_to (cr, x, height, x, height, x, height - RADIUS);
  cairo_close_path (cr);

  cairo_fill_preserve (cr);

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.2f);
  cairo_stroke (cr);

  cairo_destroy (cr);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();

  nux::BaseTexture* texture2D = nux::GetThreadGLDeviceFactory ()->CreateSystemCapableTexture ();
  texture2D->Update(bitmap);
  delete bitmap;

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  if (_bg_layer)
    delete _bg_layer;

  nux::ROPConfig rop; 
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  
  _bg_layer = new nux::TextureLayer (texture2D->GetDeviceTexture(),
                                     texxform,
                                     nux::Color::White,
                                     false,
                                     rop);

  texture2D->UnReference ();

  NeedRedraw ();
}
