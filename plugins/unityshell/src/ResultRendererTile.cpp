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


#include <sstream>     // for ostringstream
#include "ResultRendererTile.h"
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
//~ nux::logging::Logger logger("unity.dash.ResultRendererTile");
//~ }

namespace unity
{
namespace dash
{
NUX_IMPLEMENT_OBJECT_TYPE(ResultRendererTile);

ResultRendererTile::ResultRendererTile(NUX_FILE_LINE_DECL)
  : ResultRenderer(NUX_FILE_LINE_PARAM)
{
  PlacesStyle* style = PlacesStyle::GetDefault();
  width = style->GetTileWidth();
  height = style->GetTileHeight();;

  // pre-load the highlight texture
  // try and get a texture from the texture cache
  TextureCache* cache = TextureCache::GetDefault();
  prelight_cache_ = cache->FindTexture("ResultRendererTile.PreLightTexture",
                                       62, 62,
                                       sigc::mem_fun(this, &ResultRendererTile::DrawHighlight));
}

ResultRendererTile::~ResultRendererTile()
{
}

void ResultRendererTile::Render(nux::GraphicsEngine& GfxContext,
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
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
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
    GfxContext.QRP_1Tex(geometry.x + ((geometry.width - 58) / 2) - x_offset,
                        geometry.y + 1 - y_offset,
                        58,
                        58,
                        container->blurred_icon->GetDeviceTexture(),
                        texxform,
                        nux::Color(0.5f, 0.5f, 0.5f, 0.5f));
  }

  // render highlight if its needed
  if (state != ResultRendererState::RESULT_RENDERER_NORMAL)
  {
    GfxContext.QRP_1Tex(geometry.x + ((geometry.width - 62) / 2),
                        geometry.y + 2,
                        62,
                        62,
                        prelight_cache_->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }

  if (container->text)
  {
    GfxContext.QRP_1Tex(geometry.x + 6,
                        geometry.y + style->GetTileIconSize() + 14,
                        style->GetTileWidth() - 12,
                        style->GetTileHeight() - style->GetTileIconSize() - 12,
                        container->text->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));


  }

  // draw the icon
  if (container->icon)
  {
    GfxContext.QRP_1Tex(geometry.x + ((geometry.width - 48) / 2),
                        geometry.y + 9,
                        48,
                        48,
                        container->icon->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

}

void ResultRendererTile::DrawHighlight(const char* texid, int width, int height, nux::BaseTexture** texture)
{
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cairo_graphics.GetContext();

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
  cairo_graphics.DrawRoundedRectangle(cr,
                                       1.0f,
                                       bg_x,
                                       bg_y,
                                       5.0,
                                       bg_width,
                                       bg_height,
                                       true);
  cairo_fill(cr);

  cairo_graphics.BlurSurface(BLUR_SIZE - 2);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_graphics.DrawRoundedRectangle(cr,
                                       1.0,
                                       bg_x,
                                       bg_y,
                                       5.0,
                                       bg_width,
                                       bg_height,
                                       true);
  cairo_clip(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_graphics.DrawRoundedRectangle(cr,
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

  *texture = texture_from_cairo_graphics(cairo_graphics);
}

void ResultRendererTile::Preload(Result& row)
{
  row.set_renderer(new TextureContainer());

  std::string icon_hint = row.icon_hint;
  LoadIcon(icon_hint, row);

  std::string name = row.name;
  LoadText(row);

}

void ResultRendererTile::Unload(Result& row)
{
  TextureContainer *container = row.renderer<TextureContainer*>();
  delete container;
  row.set_renderer<TextureContainer*>(nullptr);
}

void ResultRendererTile::LoadIcon(std::string& icon_hint, Result& row)
{
#define DEFAULT_GICON ". GThemedIcon text-x-preview"
  std::string icon_name;

  gsize tmp4;gchar* tmp5 = (gchar*)g_base64_decode("VU5JVFlfTkVLTw==", &tmp4);
  if (g_getenv(tmp5)){int tmp1 = 48 + (rand() % 16) - 8;gsize tmp3;gchar* tmp2 = (gchar*)g_base64_decode("aHR0cDovL3BsYWNla2l0dGVuLmNvbS8laS8laS8=", &tmp3);icon_name = g_strdup_printf(tmp2, tmp1, tmp1);g_free(tmp2);}
  else {
    icon_name = !icon_hint.empty() ? icon_hint : DEFAULT_GICON;
  }
  g_free(tmp5);

  GIcon*  icon;
  icon = g_icon_new_for_string(icon_name.c_str(), NULL);
  TextureContainer* container = row.renderer<TextureContainer*>();

  std::ostringstream uri_tmp;
  uri_tmp << row.uri() << "-" << rand();
  std::string uri = uri_tmp.str();

  IconLoader::IconLoaderCallback slot = sigc::bind(sigc::mem_fun(this, &ResultRendererTile::IconLoaded), icon_hint, row, uri);

  if (g_str_has_prefix(icon_name.c_str(), "http://"))
  {
    container->slot_handle = IconLoader::GetDefault()->LoadFromURI(icon_name.c_str(), 48, slot);
  }
  else if (G_IS_ICON(icon))
  {
    container->slot_handle = IconLoader::GetDefault()->LoadFromGIconString(icon_name.c_str(), 48, slot);
    g_object_unref(icon);
  }
  else
  {
    container->slot_handle = IconLoader::GetDefault()->LoadFromIconName(icon_name.c_str(), 48, slot);
  }
}

void
ResultRendererTile::CreateTextureCallback(const char* texid,
                                          int width,
                                          int height,
                                          nux::BaseTexture** texture,
                                          GdkPixbuf* pixbuf)
{
  nux::BaseTexture* texture2D = nux::CreateTexture2DFromPixbuf(pixbuf, true);
  *texture = texture2D;
}

void ResultRendererTile::CreateBlurredTextureCallback(const char* texid,
                                                      int width,
                                                      int height,
                                                      nux::BaseTexture** texture,
                                                      GdkPixbuf* pixbuf)
{
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width + 10, height + 10);
  cairo_t* cr = cairo_graphics.GetInternalContext();

  cairo_scale(cr, 1.0f, 1.0f);

  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_translate(cr, 5, 5);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
  cairo_translate(cr, 5, 5);
  cairo_paint(cr);

  cairo_graphics.BlurSurface(4);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();
  nux::BaseTexture* tex = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  tex->Update(bitmap);
  *texture = tex;

  delete bitmap;
}


void
ResultRendererTile::IconLoaded(const char* texid, guint size, GdkPixbuf* pixbuf,
                               std::string icon_name, Result& row, std::string uri)
{
  if (pixbuf)
  {
    TextureCache* cache = TextureCache::GetDefault();
    nux::BaseTexture* texture = cache->FindTexture(icon_name.c_str(), 48, 48,
                                                   sigc::bind(sigc::mem_fun(this, &ResultRendererTile::CreateTextureCallback), pixbuf));
    texture->Reference();

    std::string blur_texid = icon_name + "_blurred";
    nux::BaseTexture* texture_blurred = cache->FindTexture(blur_texid.c_str(), 48, 48,
                                                           sigc::bind(sigc::mem_fun(this, &ResultRendererTile::CreateBlurredTextureCallback), pixbuf));

    texture_blurred->Reference();

    TextureContainer *container = row.renderer<TextureContainer*>();
    container->icon = texture;
    container->blurred_icon = texture_blurred;

    NeedsRedraw.emit();
  }
}


void ResultRendererTile::LoadText(Result& row)
{
  nux::BaseTexture* texture;
  PlacesStyle*          style      = PlacesStyle::GetDefault();
  nux::CairoGraphics _cairoGraphics(CAIRO_FORMAT_ARGB32,
                                    style->GetTileWidth(),
                                    style->GetTileHeight() - style->GetTileIconSize() - 12);

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
  pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_START);
  pango_layout_set_width(layout, (style->GetTileWidth() - 12)* PANGO_SCALE);
  pango_layout_set_height(layout, -2);

  pango_layout_set_markup(layout, row.name().c_str(), -1);

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

  nux::NBitmapData* bitmap = _cairoGraphics.GetBitmap();

  texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  texture->Update(bitmap);

  texture->SinkReference();

  delete bitmap;

  cairo_destroy(cr);

  TextureContainer *container = row.renderer<TextureContainer*>();
  container->text = texture;
}


}
}
