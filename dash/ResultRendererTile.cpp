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

#include "ResultRendererTile.h"

#include <pango/pangocairo.h>

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include <NuxGraphics/GdkGraphics.h>

#include "unity-shared/CairoTexture.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/TextureCache.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/ThemeSettings.h"

namespace unity
{
DECLARE_LOGGER(logger, "unity.dash.results");

namespace
{
const std::string DEFAULT_GICON = ". GThemedIcon text-x-preview";
const RawPixel PADDING    =  6_em;
const RawPixel SPACING    = 10_em;
const int FONT_SIZE       = 10;
const int FONT_MULTIPLIER = 1024;

char const REPLACEMENT_CHAR = '?';
float const CORNER_HIGHTLIGHT_RADIUS = 2.0f;

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

NUX_IMPLEMENT_OBJECT_TYPE(ResultRendererTile);

ResultRendererTile::ResultRendererTile(NUX_FILE_LINE_DECL)
  : ResultRenderer(NUX_FILE_LINE_PARAM)
{
  UpdateWidthHeight();
  scale.changed.connect([this] (double) { UpdateWidthHeight(); });
}

void ResultRendererTile::UpdateWidthHeight()
{
  dash::Style const& style = dash::Style::Instance();
  RawPixel tile_width  = style.GetTileWidth();
  RawPixel tile_height = style.GetTileHeight();

  width  = tile_width.CP(scale());
  height = tile_height.CP(scale());
}

void ResultRendererTile::Render(nux::GraphicsEngine& GfxContext,
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

  dash::Style const& style = dash::Style::Instance();
  int tile_icon_size = style.GetTileImageSize().CP(scale);

  // set up our texture mode
  nux::TexCoordXForm texxform;

  int icon_width, icon_height;
  if (container->icon == nullptr)
  {
    icon_width = icon_height = tile_icon_size;
  }
  else
  {
    icon_width = container->icon->GetWidth();
    icon_height = container->icon->GetHeight();
  }

  int icon_left_hand_side = geometry.x + (geometry.width - icon_width) / 2;
  int icon_top_side = geometry.y + PADDING.CP(scale()) + (tile_icon_size - icon_height) / 2;

  // render highlight if its needed
  if (container->prelight && state != ResultRendererState::RESULT_RENDERER_NORMAL)
  {
    int highlight_x = (geometry.x + geometry.width/2) - style.GetTileIconHightlightWidth().CP(scale)/2;
    int highlight_y = (geometry.y + PADDING.CP(scale) + tile_icon_size / 2) - style.GetTileIconHightlightHeight().CP(scale)/2;

    RenderTexture(GfxContext,
                  highlight_x,
                  highlight_y,
                  container->prelight->GetWidth(),
                  container->prelight->GetHeight(),
                  container->prelight->GetDeviceTexture(),
                  texxform,
                  color,
                  saturate);
  }

  // draw the icon
  if (container->icon)
  {
    RenderTexture(GfxContext,
                  icon_left_hand_side,
                  icon_top_side,
                  container->icon->GetWidth(),
                  container->icon->GetHeight(),
                  container->icon->GetDeviceTexture(),
                  texxform,
                  color,
                  saturate);
  }

  if (container->text)
  {
    RenderTexture(GfxContext,
                  geometry.x + PADDING.CP(scale),
                  geometry.y + tile_icon_size + SPACING.CP(scale),
                  style.GetTileWidth().CP(scale) - (PADDING.CP(scale) * 2),
                  style.GetTileHeight().CP(scale) - tile_icon_size - SPACING.CP(scale),
                  container->text->GetDeviceTexture(),
                  texxform,
                  color,
                  saturate);
  }
}

nux::BaseTexture* ResultRendererTile::DrawHighlight(std::string const& texid, int width, int height)
{
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_surface_set_device_scale(cairo_graphics.GetSurface(), scale(), scale());
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
                                      CORNER_HIGHTLIGHT_RADIUS,
                                      width/scale(),
                                      height/scale(),
                                      false);
  cairo_fill(cr);

  return texture_from_cairo_graphics(cairo_graphics);
}

int ResultRendererTile::Padding() const
{
  return PADDING.CP(scale());
}

void ResultRendererTile::Preload(Result const& row)
{
  if (row.renderer<TextureContainer*>() == nullptr)
  {
    // Shouldn't really do this, but it's safe in this case and quicker than making a copy.
    const_cast<Result&>(row).set_renderer(new TextureContainer());
    LoadIcon(row);
    LoadText(row);
  }
}

void ResultRendererTile::ReloadResult(Result const& row)
{
  Unload(row);

  if (row.renderer<TextureContainer*>() == nullptr)
    const_cast<Result&>(row).set_renderer(new TextureContainer());

  LoadIcon(row);
  LoadText(row);
}

void ResultRendererTile::Unload(Result const& row)
{
  TextureContainer *container = row.renderer<TextureContainer*>();
  if (container)
  {
    delete container;
    // Shouldn't really do this, but it's safe in this case and quicker than making a copy.
    const_cast<Result&>(row).set_renderer<TextureContainer*>(nullptr);
  }
}

nux::NBitmapData* ResultRendererTile::GetDndImage(Result const& row) const
{
  TextureContainer* container = row.renderer<TextureContainer*>();
  nux::NBitmapData* bitmap = nullptr;

  if (container && container->drag_icon && container->drag_icon.IsType(GDK_TYPE_PIXBUF))
  {
    // Need to ref the drag icon because GdkGraphics will unref it.
    nux::GdkGraphics graphics(GDK_PIXBUF(g_object_ref(container->drag_icon)));
    bitmap = graphics.GetBitmap();
  }
  return bitmap ? bitmap : ResultRenderer::GetDndImage(row);
}

void ResultRendererTile::LoadIcon(Result const& row)
{
  Style const& style = Style::Instance();
  int tile_size   = style.GetTileImageSize().CP(scale);
  int tile_gsize  = style.GetTileGIconSize().CP(scale);

  std::string const& icon_hint = row.icon_hint;
  std::string const& icon_name = !icon_hint.empty() ? icon_hint : DEFAULT_GICON;

  glib::Object<GIcon> icon(g_icon_new_for_string(icon_name.c_str(), NULL));
  TextureContainer* container = row.renderer<TextureContainer*>();

  if (container)
  {
    TextureCache& cache = TextureCache::GetDefault();
    BaseTexturePtr texture_prelight(cache.FindTexture("resultview_prelight",
                                                      style.GetTileIconHightlightWidth().CP(scale),
                                                      style.GetTileIconHightlightHeight().CP(scale),
                                                      sigc::mem_fun(this, &ResultRendererTile::DrawHighlight)));
    container->prelight = texture_prelight;
  }

  IconLoader::IconLoaderCallback slot = sigc::bind(sigc::mem_fun(this, &ResultRendererTile::IconLoaded), icon_hint, row);

  if (icon.IsType(G_TYPE_ICON))
  {
    bool use_large_icon = icon.IsType(G_TYPE_FILE_ICON) || !icon.IsType(G_TYPE_THEMED_ICON);
    container->slot_handle = IconLoader::GetDefault().LoadFromGIconString(icon_name,
                                                                          tile_size,
                                                                          use_large_icon ?
                                                                          tile_size : tile_gsize, slot);
  }
  else
  {
    container->slot_handle = IconLoader::GetDefault().LoadFromIconName(icon_name, -1, tile_gsize, slot);
  }
}

nux::BaseTexture* ResultRendererTile::CreateTextureCallback(std::string const& texid,
                                                            int width,
                                                            int height,
                                                            glib::Object<GdkPixbuf> const& pixbuf)
{
  int pixbuf_width, pixbuf_height;
  pixbuf_width = gdk_pixbuf_get_width(pixbuf);
  pixbuf_height = gdk_pixbuf_get_height(pixbuf);
  if (G_UNLIKELY(!pixbuf_height || !pixbuf_width))
  {
    LOG_ERROR(logger) << "Pixbuf: " << texid << " has a zero height/width: "
                      << width << "," << height;
    pixbuf_width = (pixbuf_width) ? pixbuf_width : 1; // no zeros please
    pixbuf_height = (pixbuf_height) ? pixbuf_height: 1; // no zeros please
  }

  if (pixbuf_width == pixbuf_height)
  {
    // quick path for square icons
    return nux::CreateTexture2DFromPixbuf(pixbuf, true);
  }
  else
  {
    // slow path for non square icons that must be resized to fit in the square
    // texture
    double aspect = static_cast<double>(pixbuf_height) / pixbuf_width; // already sanitized width/height so can not be 0.0
    if (aspect < 1.0f)
    {
      pixbuf_width = Style::Instance().GetTileImageSize().CP(scale);
      pixbuf_height = pixbuf_width * aspect;

      if (pixbuf_height > height)
      {
        // scaled too big, scale down
        pixbuf_height = height;
        pixbuf_width = pixbuf_height / aspect;
      }
    }
    else
    {
      pixbuf_height = height;
      pixbuf_width = pixbuf_height / aspect;
    }

    if (gdk_pixbuf_get_height(pixbuf) == pixbuf_height)
    {
      // we changed our mind, fast path is good
      return nux::CreateTexture2DFromPixbuf(pixbuf, true);
    }

    nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, pixbuf_width, pixbuf_height);
    cairo_surface_set_device_scale(cairo_graphics.GetSurface(), scale(), scale());
    cairo_t* cr = cairo_graphics.GetInternalContext();

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);

    double pixmap_scale = float(pixbuf_height) / gdk_pixbuf_get_height(pixbuf) / scale();
    cairo_scale(cr, pixmap_scale, pixmap_scale);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);

    return texture_from_cairo_graphics(cairo_graphics);
  }
}

void ResultRendererTile::IconLoaded(std::string const& texid,
                                    int max_width,
                                    int max_height,
                                    glib::Object<GdkPixbuf> const& pixbuf,
                                    std::string const& icon_name,
                                    Result const& row)
{
  TextureContainer *container = row.renderer<TextureContainer*>();

  if (pixbuf && container)
  {
    TextureCache& cache = TextureCache::GetDefault();
    BaseTexturePtr texture(cache.FindTexture(icon_name, max_width, max_height,
                           sigc::bind(sigc::mem_fun(this, &ResultRendererTile::CreateTextureCallback), pixbuf)));

    container->icon = texture;
    container->drag_icon = pixbuf;

    NeedsRedraw.emit();

    if (container)
      container->slot_handle = 0;
  }
  else if (container)
  {
    // we need to load a missing icon
    IconLoader::IconLoaderCallback slot = sigc::bind(sigc::mem_fun(this, &ResultRendererTile::IconLoaded), icon_name, row);
    container->slot_handle = IconLoader::GetDefault().LoadFromGIconString(". GThemedIcon text-x-preview", max_width, max_height, slot);
  }
}

/* Blacklisted unicode ranges:
 * Burmese:  U+1000 -> U+109F
 * Extended: U+AA60 -> U+AA7B
*/
bool IsBlacklistedChar(gunichar uni_c)
{
  if ((uni_c >= 0x1000 && uni_c <= 0x109F) ||
      (uni_c >= 0xAA60 && uni_c <= 0xAA7B))
  {
    return true;
  }

  return false;
}

// FIXME Bug (lp.1239381) in the backend of pango that crashes
// when using ellipsize with/height setting in pango
std::string ReplaceBlacklistedChars(std::string const& str)
{
  std::string new_string("");

  if (!g_utf8_validate(str.c_str(), -1, NULL))
    return new_string;

  gchar const* uni_s = str.c_str();
  gunichar uni_c;
  gchar utf8_buff[6];

  int size = g_utf8_strlen(uni_s, -1);
  for (int i = 0; i < size; ++i)
  {
    uni_c = g_utf8_get_char(uni_s);
    uni_s = g_utf8_next_char(uni_s);

    if (IsBlacklistedChar(uni_c))
    {
      new_string += REPLACEMENT_CHAR;
    }
    else
    {
      int end = g_unichar_to_utf8(uni_c, utf8_buff);
      utf8_buff[end] = '\0';

      new_string += utf8_buff;
    }
  }

  return new_string;
}

void ResultRendererTile::LoadText(Result const& row)
{
  Style const& style = Style::Instance();

  nux::CairoGraphics _cairoGraphics(CAIRO_FORMAT_ARGB32,
                                    style.GetTileWidth().CP(scale()) - (PADDING.CP(scale()) * 2),
                                    style.GetTileHeight().CP(scale()) - style.GetTileImageSize().CP(scale()) - SPACING.CP(scale()));
  cairo_surface_set_device_scale(_cairoGraphics.GetSurface(), scale(), scale());

  cairo_t* cr = _cairoGraphics.GetInternalContext();

  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pango_context   = NULL;
  GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed

  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(theme::Settings::Get()->font().c_str());
  pango_font_description_set_size (desc, FONT_SIZE * FONT_MULTIPLIER);

  pango_layout_set_font_description(layout, desc);
  pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_START);
  pango_layout_set_width(layout, (style.GetTileWidth() - (PADDING * 2))* PANGO_SCALE);
  pango_layout_set_height(layout, -2);

  // FIXME bug #1239381
  std::string name = ReplaceBlacklistedChars(row.name());

  char *escaped_text = g_markup_escape_text(name.c_str(), -1);

  pango_layout_set_markup(layout, escaped_text, -1);

  g_free (escaped_text);

  pango_context = pango_layout_get_context(layout);  // is not ref'ed
  pango_cairo_context_set_font_options(pango_context, gdk_screen_get_font_options(screen));
  pango_cairo_context_set_resolution(pango_context, 96.0 * Settings::Instance().font_scaling());
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

  TextureContainer *container = row.renderer<TextureContainer*>();
  if (container)
    container->text = texture_ptr_from_cairo_graphics(_cairoGraphics);
}


}
}
