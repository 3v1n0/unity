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

#include "PlacesSearchBar.h"

PlacesSearchBar::PlacesSearchBar (NUX_FILE_LINE_DECL)
:   View (NUX_FILE_LINE_PARAM)
{
  _bg_layer = new nux::ColorLayer (nux::Color (0xff595853), true);

  _layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);
   SetCompositionLayout (_layout);
}

PlacesSearchBar::~PlacesSearchBar ()
{
  delete _bg_layer;
}

const gchar* PlacesSearchBar::GetName ()
{
	return "PlacesSearchBar";
}

const gchar *
PlacesSearchBar::GetChildsName ()
{
  return "";
}

void PlacesSearchBar::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}

long
PlacesSearchBar::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);
  return ret;
}

void
PlacesSearchBar::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  gPainter.PushDrawLayer (GfxContext, GetGeometry (), _bg_layer);

  gPainter.PopBackground ();

  GfxContext.PopClippingRectangle ();
}

void
PlacesSearchBar::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  gPainter.PushLayer (GfxContext, GetGeometry (), _bg_layer);
  
  _layout->ProcessDraw (GfxContext, force_draw);

  gPainter.PopBackground ();
  GfxContext.PopClippingRectangle();
}

void
PlacesSearchBar::PreLayoutManagement ()
{
  nux::View::PreLayoutManagement ();
}

long
PlacesSearchBar::PostLayoutManagement (long LayoutResult)
{
  // I'm imagining this is a good as time as any to update the background
  UpdateBackground ();

  return nux::View::PostLayoutManagement (LayoutResult);
}

void
PlacesSearchBar::UpdateBackground ()
{
  nux::Geometry geo = GetGeometry ();

  if (geo.width == _last_width && geo.height == _last_height)
    return;

  _last_width = geo.width;
  _last_height = geo.height;

  g_debug ("%d %d", _last_width, _last_height);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, _last_width, _last_height);
  cairo_t *cr = cairo_graphics.GetContext();
  cairo_set_line_width (cr, 1);

  cairo_pattern_t *pat = cairo_pattern_create_linear (0, 0, 0, _last_height);
  cairo_pattern_add_color_stop_rgb (pat, 0.0f, 89/255.0f, 88/255.0f, 83/255.0f);
  cairo_pattern_add_color_stop_rgb (pat, 1.0f, 50/255.0f, 50/255.0f, 45/255.0f);
  cairo_set_source (cr, pat);
  cairo_rectangle (cr, 0, 0, _last_width, _last_height);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

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
  rop.Blend = false;                      // Disable the blending. By default rop.Blend is false.
  rop.SrcBlend = GL_SRC_ALPHA;            // Set the source blend factor.
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;  // Set the destination blend factor.
  
  _bg_layer = new nux::TextureLayer (texture2D->GetDeviceTexture(),
                                     texxform,          // The Oject that defines the texture wraping and coordinate transformation.
                                     nux::Color::White, // The color used to modulate the texture.
                                     true,              // Write the alpha value of the texture to the destination buffer.
                                     rop                // Use the given raster operation to set the blending when the layer is being rendered.
  );

  texture2D->UnReference ();

  NeedRedraw ();
}
