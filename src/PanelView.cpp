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
#include <Nux/VLayout.h>
#include <Nux/WindowCompositor.h>

#include <NuxImage/CairoGraphics.h>
#include <NuxImage/ImageSurface.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>

#include "PanelView.h"

#include "IndicatorObjectFactoryRemote.h"
#include "PanelIndicatorObjectView.h"

PanelView::PanelView (NUX_FILE_LINE_DECL)
:   View (NUX_FILE_LINE_PARAM)
{
  _bg_layer = new nux::ColorLayer (nux::Color (0xff595853), true);

  _layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);
   SetCompositionLayout (_layout);

   // Home button
   _home_button = new PanelHomeButton ();
   _layout->AddView (_home_button, 0, nux::eCenter, nux::eFull);

  _remote = new IndicatorObjectFactoryRemote ();
  _remote->OnObjectAdded.connect (sigc::mem_fun (this, &PanelView::OnObjectAdded));
}

PanelView::~PanelView ()
{
  delete _remote;
  delete _bg_layer;
}

long
PanelView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);
  return ret;
}

void
PanelView::Draw (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  gPainter.PushDrawLayer (GfxContext, GetGeometry (), _bg_layer);

  gPainter.PopBackground ();

  GfxContext.PopClippingRectangle ();
}

void
PanelView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  gPainter.PushLayer (GfxContext, GetGeometry (), _bg_layer);
  
  _layout->ProcessDraw (GfxContext, force_draw);

  gPainter.PopBackground ();
  GfxContext.PopClippingRectangle();
}

void
PanelView::PreLayoutManagement ()
{
  nux::View::PreLayoutManagement ();
}

long
PanelView::PostLayoutManagement (long LayoutResult)
{
  // I'm imagining this is a good as time as any to update the background
  UpdateBackground ();

  return nux::View::PostLayoutManagement (LayoutResult);
}

void
PanelView::UpdateBackground ()
{
  nux::Geometry geo = GetGeometry ();

  if (geo.width == _last_width && geo.height == _last_height)
    return;

  _last_width = geo.width;
  _last_height = geo.height;

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

//
// Signals
//
void
PanelView::OnObjectAdded (IndicatorObjectProxy *proxy)
{
  PanelIndicatorObjectView *view = new PanelIndicatorObjectView (proxy);

  // Appmenu is treated differently as it needs to expand
  // We could do this in a more special way, but who has the time for special?
  _layout->AddView (view, (g_strstr_len (proxy->GetName ().c_str (), -1, "appmenu") != NULL), nux::eCenter, nux::eFull);
  _layout->SetContentDistribution (nux::eStackLeft);

  this->ComputeChildLayout (); 
  NeedRedraw ();
}
