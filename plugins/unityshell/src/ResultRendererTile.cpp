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
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "IconLoader.h"
#include "IconTexture.h"
#include "TextureCache.h"

//~ namespace
//~ {
  //~ nux::logging::Logger logger("unity.dash.ResultRendererTile");
//~ }

namespace unity
{
namespace dash
{

ResultRendererTile::ResultRendererTile(NUX_FILE_LINE_DECL)
  : ResultRenderer(NUX_FILE_LINE_PARAM)
  , testing_texture_(NULL)
  , testing_text_(NULL)
{
  width = 150;
  height = 48 + 25 + 6;
}

ResultRendererTile::~ResultRendererTile()
{
}

void ResultRendererTile::Render (nux::GraphicsEngine& GfxContext,
                             Result& row,
                             ResultRendererState state,
                             nux::Geometry& geometry)
{
  std::string row_text = row.name;
  std::string row_iconhint = row.icon_hint;

  std::map<std::string, nux::BaseTexture *>::iterator it;
  it = text_cache_.find(row_text);

  if (it != text_cache_.end())
  {
    nux::BaseTexture* text_texture = it->second;

    nux::TexCoordXForm texxform;
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

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

    GfxContext.QRP_1Tex(geometry.x,
                        geometry.y + geometry.height - 25,
                        150,
                        25,
                        text_texture->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

    GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
  }

  // draw the icon
  it = icon_cache_.find(row_iconhint);
  if (it != icon_cache_.end())
  {
    nux::BaseTexture* icon_texture = it->second;

    nux::TexCoordXForm texxform;
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

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

    GfxContext.QRP_1Tex(geometry.x + ((geometry.width - 48) / 2),
                        geometry.y + 6,
                        48,
                        48,
                        icon_texture->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

    GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
  }

}

void ResultRendererTile::Preload (Result& row)
{
  std::string wtfthissucks = row.icon_hint;
  LoadIcon (wtfthissucks);

  wtfthissucks = row.name;
  LoadText (wtfthissucks);
}

void ResultRendererTile::Unload (Result& row)
{

  std::string icon = row.icon_hint;
  std::string text = row.name;

  if (icon_cache_.find(icon) != icon_cache_.end())
  {
    nux::BaseTexture *texture = icon_cache_.find(icon)->second;
    texture->UnReference ();
  }
  else
  {
    //~ LOG_WARNING(logger) << "Tried to unload " << icon << " but was not loaded";
  }

  if (text_cache_.find(text) != text_cache_.end())
  {
    nux::BaseTexture *texture = text_cache_.find(text)->second;
    texture->UnReference ();
  }
  else
  {
    //~ LOG_WARNING(logger) << "Tried to unload " << icon << " but was not loaded";
  }
}

void ResultRendererTile::LoadIcon (std::string& icon_hint)
{
#define DEFAULT_GICON ". GThemedIcon text-x-preview"
  std::string icon_name = !icon_hint.empty() ? icon_hint : DEFAULT_GICON;

  if (icon_cache_.find(icon_hint) == icon_cache_.end())
  {
    // if the icon is already loading, just add a note to increase the reference count
    if (currently_loading_icons_.count (icon_name) > 0)
    {
      currently_loading_icons_[icon_name] += 1;
    }
    else
    {
      GIcon*  icon;
      icon = g_icon_new_for_string(icon_name.c_str(), NULL);

      if (G_IS_ICON(icon))
      {
        IconLoader::GetDefault()->LoadFromGIconString(icon_name.c_str(), 48,
                                                      sigc::bind(sigc::mem_fun(this, &ResultRendererTile::IconLoaded), icon_name));
        g_object_unref(icon);
      }
      else if (g_str_has_prefix(icon_name.c_str(), "http://"))
      {
        IconLoader::GetDefault()->LoadFromURI(icon_name.c_str(), 48,
                                              sigc::bind(sigc::mem_fun(this, &ResultRendererTile::IconLoaded), icon_name));
      }
      else
      {
        IconLoader::GetDefault()->LoadFromIconName(icon_name.c_str(), 48,
                                                   sigc::bind(sigc::mem_fun(this, &ResultRendererTile::IconLoaded), icon_name));
      }

      currently_loading_icons_[icon_name] = 1;
    }
  }
  else
  {
    nux::BaseTexture *texture = icon_cache_.find(icon_name)->second;
    texture->Reference ();
  }
}

void
ResultRendererTile::CreateTextureCallback(const char* texid,
                                          int width,
                                          int height,
                                          nux::BaseTexture** texture,
                                          GdkPixbuf *pixbuf)
{
  nux::BaseTexture* texture2D = nux::CreateTexture2DFromPixbuf(pixbuf, true);
  *texture = texture2D;
}

void
ResultRendererTile::IconLoaded(const char* texid, guint size, GdkPixbuf* pixbuf, std::string icon_name)
{
  if (pixbuf)
  {
    TextureCache* cache = TextureCache::GetDefault();
    nux::BaseTexture *texture = cache->FindTexture(texid, 48, 48,
                                                   sigc::bind(sigc::mem_fun(this, &ResultRendererTile::CreateTextureCallback), pixbuf));
    icon_cache_[icon_name] = texture;
    for (uint i = 0; i < currently_loading_icons_[icon_name]; i++)
    {
      texture->Reference();
    }
    currently_loading_icons_.erase (icon_name);

    NeedsRedraw.emit();
  }
}

void ResultRendererTile::LoadText (std::string& text)
{
  if (text_cache_.find(text) == text_cache_.end())
  {
    nux::CairoGraphics _cairoGraphics (CAIRO_FORMAT_ARGB32,
                                       150,
                                       25);

    cairo_t* cr = cairo_reference(_cairoGraphics.GetContext());

    PangoLayout*          layout     = NULL;
    PangoFontDescription* desc       = NULL;
    PangoContext*         pangoCtx   = NULL;
    GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed


    cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
    layout = pango_cairo_create_layout(cr);
    desc = pango_font_description_from_string("Ubuntu 10");

    pango_layout_set_font_description(layout, desc);

    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_markup(layout, text.c_str(), -1);
    pango_layout_set_width(layout, 150 * PANGO_SCALE);

    pango_layout_set_height(layout, 2);
    pangoCtx = pango_layout_get_context(layout);  // is not ref'ed
    pango_cairo_context_set_font_options(pangoCtx,
                                         gdk_screen_get_font_options(screen));
    // use some default DPI-value
    pango_cairo_context_set_resolution(pangoCtx, 96.0f);

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);

    pango_layout_context_changed(layout);

    cairo_move_to(cr, 0.0f, 0.0f);
    pango_cairo_show_layout(cr, layout);

    // clean up
    pango_font_description_free(desc);
    g_object_unref(layout);

    cairo_destroy(cr);

    nux::NBitmapData* bitmap = _cairoGraphics.GetBitmap();

    nux::BaseTexture *texture;

    texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
    texture->Update(bitmap);

    texture->SinkReference();

    text_cache_[text] = texture;

    delete bitmap;

    cairo_destroy(cr);
  }
  else
  {
    nux::BaseTexture *texture = text_cache_.find(text)->second;
    texture->Reference ();
  }
}


}
}
