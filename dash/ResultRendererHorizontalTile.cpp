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

#include <pango/pangocairo.h>
#include <gtk/gtk.h>

#include "unity-shared/CairoTexture.h"
#include "unity-shared/TextureCache.h"
#include <NuxGraphics/GdkGraphics.h>


namespace unity
{
namespace
{
const int   CARD_VIEW_PADDING              = 4;   // pixels
const int   CARD_VIEW_ICON_SIZE            = 64;  // pixels
const int   CARD_VIEW_ICON_TEXT_GAP        = 10;  // pixels
const int   CARD_VIEW_WIDTH                = 277; // pixels
const int   CARD_VIEW_HEIGHT               = 74;  // pixels
const int   CARD_VIEW_HIGHLIGHT_CORNER_RADIUS = 2; // pixels
const int   CARD_VIEW_ICON_OUTLINE_WIDTH   = 1;   // pixels
const int   CARD_VIEW_TEXT_LINE_SPACING    = 0; // points

void RenderTexture(nux::GraphicsEngine& GfxContext, 
                   int x,
                   int y,
                   int width,
                   int height,
                   nux::ObjectPtr<nux::IOpenGLBaseTexture> const& texture,
                   nux::TexCoordXForm &texxform,
                   const nux::Color &color,
                   float saturate
)
{
  if (saturate == 1.0)
  {
    GfxContext.QRP_1Tex(x,
                        y,
                        width,
                        height,
                        texture,
                        texxform,
                        color);
  }
  else
  {
    GfxContext.QRP_TexDesaturate(x,
                                 y,
                                 width,
                                 height,
                                 texture,
                                 texxform,
                                 color,
                                 saturate);
  }
}

}

namespace dash
{
NUX_IMPLEMENT_OBJECT_TYPE(ResultRendererHorizontalTile);

ResultRendererHorizontalTile::ResultRendererHorizontalTile(NUX_FILE_LINE_DECL)
  : ResultRendererTile(NUX_FILE_LINE_PARAM)
{
  width = CARD_VIEW_WIDTH;
  height = CARD_VIEW_HEIGHT;

  // pre-load the highlight texture
  // try and get a texture from the texture cache
  TextureCache& cache = TextureCache::GetDefault();
  prelight_cache_ = cache.FindTexture("ResultRendererHorizontalTile.PreLightTexture",
                                      CARD_VIEW_WIDTH,
                                      CARD_VIEW_HEIGHT,
                                      sigc::mem_fun(this, &ResultRendererHorizontalTile::DrawHighlight));
  normal_cache_ = cache.FindTexture("ResultRendererHorizontalTile.NormalTexture",
                                      CARD_VIEW_WIDTH,
                                      CARD_VIEW_HEIGHT,
                                      sigc::mem_fun(this, &ResultRendererHorizontalTile::DrawNormal));

}

void ResultRendererHorizontalTile::Render(nux::GraphicsEngine& GfxContext,
                                          Result& row,
                                          ResultRendererState state,
                                          nux::Geometry const& geometry,
                                          int x_offset, int y_offset,
                                          nux::Color const& color,
                                          float saturate)
{
  TextureContainer* container = row.renderer<TextureContainer*>();
  if (container == nullptr)
    return;

  // set up our texture mode
  nux::TexCoordXForm texxform;

  int icon_left_hand_side = geometry.x + padding;
  int icon_top_side = geometry.y + ((geometry.height - CARD_VIEW_ICON_SIZE) / 2);

  // render overall tile background "rectangle"
  if (state == ResultRendererState::RESULT_RENDERER_NORMAL)
  {
    int x = icon_left_hand_side;
    int y = icon_top_side;
    int w = CARD_VIEW_WIDTH;
    int h = CARD_VIEW_HEIGHT;

    unsigned int alpha = 0;
    unsigned int src   = 0;
    unsigned int dest  = 0;
    GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
    GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    RenderTexture(GfxContext,
                  x,
                  y,
                  w,
                  h,
                  normal_cache_->GetDeviceTexture(),
                  texxform,
                  color,
                  saturate);

    GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
  }

  // render highlight if its needed
  if (state != ResultRendererState::RESULT_RENDERER_NORMAL)
  {
    int x = icon_left_hand_side;
    int y = icon_top_side;
    int w = CARD_VIEW_WIDTH;
    int h = CARD_VIEW_HEIGHT;

    RenderTexture(GfxContext,
                  x,
                  y,
                  w,
                  h,
                  prelight_cache_->GetDeviceTexture(),
                  texxform,
                  color,
                  saturate);
  }

  // draw the icon
  if (container->icon)
  {
    int x = icon_left_hand_side + CARD_VIEW_PADDING + CARD_VIEW_ICON_OUTLINE_WIDTH;
    int y = icon_top_side + CARD_VIEW_PADDING + CARD_VIEW_ICON_OUTLINE_WIDTH;
    int w = CARD_VIEW_ICON_SIZE;
    int h = CARD_VIEW_ICON_SIZE;
    gPainter.Paint2DQuadColor(GfxContext,
                              x - CARD_VIEW_ICON_OUTLINE_WIDTH,
                              y - CARD_VIEW_ICON_OUTLINE_WIDTH,
                              w + 2 * CARD_VIEW_ICON_OUTLINE_WIDTH,
                              h + 2 * CARD_VIEW_ICON_OUTLINE_WIDTH,
                              nux::color::Black);
    RenderTexture(GfxContext,
                  x,
                  y,
                  w,
                  h,
                  container->icon->GetDeviceTexture(),
                  texxform,
                  color,
                  saturate);
  }

  if (container->text)
  {
    int x = icon_left_hand_side + CARD_VIEW_PADDING + 2 * CARD_VIEW_ICON_OUTLINE_WIDTH + CARD_VIEW_ICON_SIZE + CARD_VIEW_ICON_TEXT_GAP;
    int y = icon_top_side + CARD_VIEW_PADDING;
    int w = container->text->GetWidth();
    int h = container->text->GetHeight();

    RenderTexture(GfxContext,
                  x,
                  y,
                  w,
                  h,
                  container->text->GetDeviceTexture(),
                  texxform,
                  color,
                  saturate);
  }
}

nux::BaseTexture* ResultRendererHorizontalTile::DrawHighlight(std::string const& texid,
                                                              int width, int height)
{
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cairo_graphics.GetInternalContext();

  cairo_scale(cr, 1.0f, 1.0f);

  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  // draw the highlight
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.2f);
  cairo_graphics.DrawRoundedRectangle(cr,
                                      1.0f,
                                      0.0f,
                                      0.0f,
                                      CARD_VIEW_HIGHLIGHT_CORNER_RADIUS,
                                      width,
                                      height,
                                      false);
  cairo_fill(cr);

  return texture_from_cairo_graphics(cairo_graphics);
}

nux::BaseTexture* ResultRendererHorizontalTile::DrawNormal(std::string const& texid,
                                                              int width, int height)
{
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cairo_graphics.GetInternalContext();

  cairo_scale(cr, 1.0f, 1.0f);

  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  // draw the normal bg
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.075f);
  cairo_graphics.DrawRoundedRectangle(cr,
                                      1.0f,
                                      0.0f,
                                      0.0f,
                                      CARD_VIEW_HIGHLIGHT_CORNER_RADIUS,
                                      width,
                                      height,
                                      false);
  cairo_fill(cr);

  return texture_from_cairo_graphics(cairo_graphics);
}

void ResultRendererHorizontalTile::LoadText(Result const& row)
{
  std::stringstream final_text;
  char *name = g_markup_escape_text(row.name().c_str()  , -1);
  char *comment = g_markup_escape_text(row.comment().c_str()  , -1);

  if(row.comment().empty())
    final_text << "<b>" << name << "</b>";
  else
    final_text << "<b>" << name << "</b>" << "\n" << comment;

  g_free(name);
  g_free(comment);

  nux::CairoGraphics _cairoGraphics(CAIRO_FORMAT_ARGB32,
                                    CARD_VIEW_WIDTH - CARD_VIEW_ICON_SIZE - 2 * CARD_VIEW_ICON_OUTLINE_WIDTH - 2 * CARD_VIEW_PADDING - CARD_VIEW_ICON_TEXT_GAP,
                                    CARD_VIEW_HEIGHT - 2 * CARD_VIEW_PADDING);

  cairo_t* cr = _cairoGraphics.GetContext();

  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pango_context   = NULL;
  GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed
  int                   dpi = -1;

  g_object_get(gtk_settings_get_default(), "gtk-xft-dpi", &dpi, NULL);

  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string("Ubuntu 10");

  pango_layout_set_font_description(layout, desc);
  pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_spacing(layout, CARD_VIEW_TEXT_LINE_SPACING * PANGO_SCALE);
  pango_layout_set_width(layout, (CARD_VIEW_WIDTH - CARD_VIEW_ICON_SIZE - 2 * CARD_VIEW_ICON_OUTLINE_WIDTH - 2 * CARD_VIEW_PADDING - CARD_VIEW_ICON_TEXT_GAP) * PANGO_SCALE);
  pango_layout_set_height(layout, -4);

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

  double offset = 0.0;
  PangoRectangle logRect = {0, 0, 0, 0};
  pango_layout_get_extents(layout, NULL, &logRect);
  if (pango_layout_get_line_count(layout) < 4)
    offset = ((CARD_VIEW_HEIGHT - 2 * CARD_VIEW_PADDING) - (logRect.height / PANGO_SCALE)) / 2.0;
  cairo_move_to(cr, 0.0f, offset);
  pango_cairo_show_layout(cr, layout);

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);

  cairo_destroy(cr);

  TextureContainer *container = row.renderer<TextureContainer*>();
  if (container)
    container->text = texture_ptr_from_cairo_graphics(_cairoGraphics);
}

nux::NBitmapData* ResultRendererHorizontalTile::GetDndImage(Result const& row) const
{
  TextureContainer* container = row.renderer<TextureContainer*>();
  nux::NBitmapData* bitmap = nullptr;

  if (container && container->drag_icon && container->drag_icon.IsType(GDK_TYPE_PIXBUF))
  {
    int width = gdk_pixbuf_get_width(container->drag_icon);
    int height = gdk_pixbuf_get_height(container->drag_icon);

    if (width != CARD_VIEW_ICON_SIZE || height != CARD_VIEW_ICON_SIZE)
    {
      nux::GdkGraphics graphics(gdk_pixbuf_scale_simple(container->drag_icon, CARD_VIEW_ICON_SIZE, CARD_VIEW_ICON_SIZE, GDK_INTERP_BILINEAR));
      bitmap = graphics.GetBitmap();
    }
  }
  return bitmap ? bitmap : ResultRendererTile::GetDndImage(row);
}


}
}
