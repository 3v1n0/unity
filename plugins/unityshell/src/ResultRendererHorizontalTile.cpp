// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include "ResultRendererHorizontalTile.h"

#include <sstream>

#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <UnityCore/GLibWrapper.h>

#include "CairoTexture.h"
#include "IconLoader.h"
#include "IconTexture.h"
#include "PlacesStyle.h"
#include "TextureCache.h"

//~ namespace
//~ {
//~ nux::logging::Logger logger("unity.dash.ResultRendererHorizontalTile");
//~ }

namespace unity
{
namespace dash
{
NUX_IMPLEMENT_OBJECT_TYPE(ResultRendererHorizontalTile);

ResultRendererHorizontalTile::ResultRendererHorizontalTile(NUX_FILE_LINE_DECL)
  : ResultRendererTile(NUX_FILE_LINE_PARAM)
{
  PlacesStyle* style = PlacesStyle::GetDefault();
  width = style->GetTileWidth() * 2;
  height = style->GetTileIconSize() + 4;

  // pre-load the highlight texture
  // try and get a texture from the texture cache
  TextureCache* cache = TextureCache::GetDefault();
  prelight_cache_ = cache->FindTexture("ResultRendererHorizontalTile.PreLightTexture",
                                       style->GetTileIconSize() + 8, style->GetTileIconSize() + 8,
                                       sigc::mem_fun(this, &ResultRendererHorizontalTile::DrawHighlight));
}

ResultRendererHorizontalTile::~ResultRendererHorizontalTile()
{
}

void ResultRendererHorizontalTile::Render(nux::GraphicsEngine& GfxContext,
                                Result& row,
                                ResultRendererState state,
                                nux::Geometry& geometry,
                                int x_offset, int y_offset)
{
  std::string row_text = row.name;
  std::string row_iconhint = row.icon_hint;
  PlacesStyle* style = PlacesStyle::GetDefault();

  // set up our texture mode
  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);


  // clear what is behind us
  nux::t_u32 alpha = 0, src = 0, dest = 0;

  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::Color col = nux::color::Black;
  col.alpha = 0;
  GfxContext.QRP_Color(geometry.x,
                       geometry.y,
                       geometry.width,
                       geometry.height,
                       col);

  TextureContainer *container = row.renderer<TextureContainer*>();

  if (container->blurred_icon)
  {
    GfxContext.QRP_1Tex(geometry.x + 2 - x_offset,
                        geometry.y + ((geometry.height - style->GetTileIconSize()) / 2 - y_offset),
                        style->GetTileIconSize() + 4,
                        style->GetTileIconSize() + 4,
                        container->blurred_icon->GetDeviceTexture(),
                        texxform,
                        nux::Color(0.5f, 0.5f, 0.5f, 0.5f));
  }

  // render highlight if its needed
  if (state != ResultRendererState::RESULT_RENDERER_NORMAL)
  {
    GfxContext.QRP_1Tex(geometry.x,
                        geometry.y + ((geometry.height - style->GetTileIconSize()) / 2) - 4,
                        style->GetTileIconSize() + 8,
                        style->GetTileIconSize() + 8,
                        prelight_cache_->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }



  if (container->text)
  {
    GfxContext.QRP_1Tex(geometry.x + style->GetTileIconSize() + 6,
                        geometry.y + 2,
                        width() - style->GetTileIconSize(),
                        style->GetTileIconSize() - 4,
                        container->text->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }

  // draw the icon
  if (container->icon)
  {
    GfxContext.QRP_1Tex(geometry.x + 4,
                        geometry.y + ((geometry.height - style->GetTileIconSize()) / 2),
                        style->GetTileIconSize(),
                        style->GetTileIconSize(),
                        container->icon->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

}

void ResultRendererHorizontalTile::DrawHighlight(const char* texid, int width, int height, nux::BaseTexture** texture)
{
  nux::CairoGraphics* cairo_graphics = new nux::CairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cairo_graphics->GetContext();

  cairo_scale(cr, 1.0f, 1.0f);

  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  int PADDING = 4;
  int BLUR_SIZE = 5;

  int bg_width = width - PADDING - BLUR_SIZE;
  int bg_height = height - PADDING - BLUR_SIZE;
  int bg_x = BLUR_SIZE - 1;
  int bg_y = BLUR_SIZE - 1;

  // draw the glow
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width(cr, 1.0f);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.75f);
  cairo_graphics->DrawRoundedRectangle(cr,
                                       1.0f,
                                       bg_x,
                                       bg_y,
                                       5.0,
                                       bg_width,
                                       bg_height,
                                       true);
  cairo_fill(cr);

  cairo_graphics->BlurSurface(BLUR_SIZE - 2);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_graphics->DrawRoundedRectangle(cr,
                                       1.0,
                                       bg_x,
                                       bg_y,
                                       5.0,
                                       bg_width,
                                       bg_height,
                                       true);
  cairo_clip(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_graphics->DrawRoundedRectangle(cr,
                                       1.0,
                                       bg_x,
                                       bg_y,
                                       5.0,
                                       bg_width,
                                       bg_height,
                                       true);
  cairo_set_source_rgba(cr, 240 / 255.0f, 240 / 255.0f, 240 / 255.0f, 1.0f);
  cairo_fill_preserve(cr);

  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0);
  cairo_stroke(cr);

  cairo_destroy(cr);

  *texture = texture_from_cairo_graphics(*cairo_graphics);
  delete cairo_graphics;
}

void ResultRendererHorizontalTile::LoadText(Result& row)
{
  std::stringstream final_text;
  final_text << row.name() << "\n<span size=\"small\">"
             << row.comment() << "</span>";

  PlacesStyle*          style      = PlacesStyle::GetDefault();
  nux::CairoGraphics _cairoGraphics(CAIRO_FORMAT_ARGB32,
                                    width() - style->GetTileIconSize(),
                                    height() - 4);

  cairo_t* cr = _cairoGraphics.GetContext();

  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pango_context   = NULL;
  GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed
  glib::String          font;
  int                   dpi = -1;

  g_object_get(gtk_settings_get_default(), "gtk-font-name", &font, NULL);
  g_object_get(gtk_settings_get_default(), "gtk-xft-dpi", &dpi, NULL);

  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(font.Value());

  pango_layout_set_font_description(layout, desc);
  pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_width(layout, (width() - style->GetTileIconSize())* PANGO_SCALE);
  pango_layout_set_height(layout, (height() - 4) * PANGO_SCALE);

  pango_layout_set_markup(layout, final_text.str().c_str(), -1);

  pango_context = pango_layout_get_context(layout);  // is not ref'ed
  pango_cairo_context_set_font_options(pango_context,
                                       gdk_screen_get_font_options(screen));
  pango_cairo_context_set_resolution(pango_context,
                                     dpi == -1 ? 96.0f : dpi/(float) PANGO_SCALE);
  pango_layout_context_changed(layout);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);

  cairo_move_to(cr, 0.0f, 0.0f);
  pango_cairo_show_layout(cr, layout);

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);

  texture->SinkReference();
  cairo_destroy(cr);

  nux::BaseTexture* texture = texture_from_cairo_graphics(_cairoGraphics);

  TextureContainer *container = row.renderer<TextureContainer*>();
  container->text = texture;
  // The container smart pointer is now the only reference to the texture.
  texture->UnReference();
}


}
}
