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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include "config.h"
#include <math.h>

#include <Nux/Nux.h>
#include <NuxGraphics/NuxGraphics.h>
#include <NuxGraphics/GpuDevice.h>
#include <NuxGraphics/GLTextureResourceManager.h>

#include <NuxImage/CairoGraphics.h>

#include <gtk/gtk.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "IconRenderer.h"

namespace unity {
namespace ui {

IconRenderer::IconRenderer (int size)
{
  icon_size = size;
  GenerateTextures ();
}

IconRenderer::~IconRenderer ()
{

}

void IconRenderer::SetTargetSize (int size)
{
  icon_size = size;
  DestroyTextures ();
  GenerateTextures ();
}

void IconRenderer::PreprocessIcons (std::list<RenderArg> &args, nux::Geometry target_window)
{
  
}

void IconRenderer::RenderIcon (nux::GraphicsEngine& GfxContext, RenderArg const &arg, nux::Geometry anchor_geo)
{
  
}

nux::BaseTexture* IconRenderer::RenderCharToTexture (const char label, int width, int height)
{
  nux::BaseTexture*     texture  = NULL;
  nux::CairoGraphics*   cg       = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32,
                                                           width,
                                                           height);
  cairo_t*              cr       = cg->GetContext ();
  PangoLayout*          layout   = NULL;
  PangoContext*         pangoCtx = NULL;
  PangoFontDescription* desc     = NULL;
  GtkSettings*          settings = gtk_settings_get_default (); // not ref'ed
  gchar*                fontName = NULL;

  double label_pos = double(icon_size / 3.0f);
  double text_size = double(icon_size / 4.0f);
  double label_x = label_pos;
  double label_y = label_pos;
  double label_w = label_pos;
  double label_h = label_pos;
  double label_r = 3.0f;

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cg->DrawRoundedRectangle (cr, 1.0f, label_x, label_y, label_r, label_w, label_h);
  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.65f);
  cairo_fill (cr);

  layout = pango_cairo_create_layout (cr);
  g_object_get (settings, "gtk-font-name", &fontName, NULL);
  desc = pango_font_description_from_string (fontName);
  pango_font_description_set_absolute_size (desc, text_size * PANGO_SCALE);
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_text (layout, &label, 1);
  pangoCtx = pango_layout_get_context (layout); // is not ref'ed

  PangoRectangle logRect;
  PangoRectangle inkRect;
  pango_layout_get_extents (layout, &inkRect, &logRect);

  /* position and paint text */
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  double x = label_x - ((logRect.width / PANGO_SCALE) - label_w) / 2.0f;
  double y = label_y - ((logRect.height / PANGO_SCALE) - label_h) / 2.0f - 1;
  cairo_move_to (cr, x, y);
  pango_cairo_show_layout (cr, layout);

  nux::NBitmapData* bitmap = cg->GetBitmap ();
  texture = nux::GetGraphicsDisplay ()->GetGpuDevice ()->CreateSystemCapableTexture ();
  texture->Update (bitmap);
  delete bitmap;
  delete cg;
  g_object_unref (layout);
  pango_font_description_free (desc);
  g_free (fontName);

  return texture;
}

void IconRenderer::RenderElement (nux::GraphicsEngine& GfxContext,
                                  RenderArg const &arg,
                                  nux::IntrusiveSP<nux::IOpenGLBaseTexture> icon,
                                  nux::Color bkg_color,
                                  float alpha,
                                  nux::Vector4 xform_coords[])
{
  
}

void IconRenderer::RenderIndicators (nux::GraphicsEngine& GfxContext,
                                     RenderArg const &arg,
                                     int running,
                                     int active,
                                     float alpha,
                                     nux::Geometry& geo)
{
  
}

void IconRenderer::RenderProgressToTexture (nux::GraphicsEngine& GfxContext, 
                                            nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture, 
                                            float progress_fill, 
                                            float bias)
{
  
}

void IconRenderer::DestroyTextures ()
{
  _icon_bkg_texture->UnReference ();
  _icon_outline_texture->UnReference ();
  _icon_shine_texture->UnReference ();
  _icon_glow_texture->UnReference ();
  _icon_glow_hl_texture->UnReference ();
  _progress_bar_trough->UnReference ();
  _progress_bar_fill->UnReference ();
  
  _pip_ltr->UnReference ();
  _arrow_ltr->UnReference ();
  _arrow_empty_ltr->UnReference ();
  
  _pip_rtl->UnReference ();
  _arrow_rtl->UnReference ();
  _arrow_empty_rtl->UnReference ();

  for (int i = 0; i < MAX_SHORTCUT_LABELS; i++)
    if (_superkey_labels[i])
      _superkey_labels[i]->UnReference ();
}

void IconRenderer::GenerateTextures ()
{
  _icon_bkg_texture       = nux::CreateTexture2DFromFile (PKGDATADIR"/round_corner_54x54.png", -1, true);
  _icon_outline_texture   = nux::CreateTexture2DFromFile (PKGDATADIR"/round_outline_54x54.png", -1, true);
  _icon_shine_texture     = nux::CreateTexture2DFromFile (PKGDATADIR"/round_shine_54x54.png", -1, true);
  _icon_glow_texture      = nux::CreateTexture2DFromFile (PKGDATADIR"/round_glow_62x62.png", -1, true);
  _icon_glow_hl_texture   = nux::CreateTexture2DFromFile (PKGDATADIR"/round_glow_hl_62x62.png", -1, true);
  _progress_bar_trough    = nux::CreateTexture2DFromFile (PKGDATADIR"/progress_bar_trough.png", -1, true);
  _progress_bar_fill      = nux::CreateTexture2DFromFile (PKGDATADIR"/progress_bar_fill.png", -1, true);

  _pip_ltr                = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_pip_ltr.png", -1, true);
  _arrow_ltr              = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_arrow_ltr.png", -1, true);
  _arrow_empty_ltr        = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_arrow_outline_ltr.png", -1, true);

  _pip_rtl                = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_pip_rtl.png", -1, true);
  _arrow_rtl              = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_arrow_rtl.png", -1, true);
  _arrow_empty_rtl        = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_arrow_outline_rtl.png", -1, true);

  for (int i = 0; i < MAX_SHORTCUT_LABELS; i++)
    _superkey_labels[i] = RenderCharToTexture ((char) ('0' + ((i  + 1) % 10)), icon_size, icon_size);
}

}
}
