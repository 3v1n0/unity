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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#include "config.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <pango/pangocairo.h>

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <NuxGraphics/GLThread.h>
#include <UnityCore/GLibWrapper.h>

#include "IconLoader.h"
#include "IconTexture.h"
#include "TextureCache.h"

namespace unity
{
DECLARE_LOGGER(logger, "unity.icontexture");
namespace
{
const char* const DEFAULT_ICON = "text-x-preview";
}

using namespace unity;

IconTexture::IconTexture(BaseTexturePtr const& texture, unsigned width, unsigned height)
  : TextureArea(NUX_TRACKER_LOCATION),
    _accept_key_nav_focus(false),
    _size(height),
    _texture_cached(texture),
    _texture_size(width, height),
    _loading(false),
    _opacity(1.0f),
    _handle(0),
    _draw_mode(DrawMode::NORMAL)
{
  SetMinMaxSize(width, height);
}

IconTexture::IconTexture(nux::BaseTexture* texture, unsigned width, unsigned height)
  : IconTexture(BaseTexturePtr(texture), width, height)
{}

IconTexture::IconTexture(BaseTexturePtr const& texture)
  : IconTexture(texture, texture ? texture->GetWidth() : 0, texture ? texture->GetHeight() : 0)
{}

IconTexture::IconTexture(nux::BaseTexture* texture)
  : IconTexture(BaseTexturePtr(texture))
{}

IconTexture::IconTexture(std::string const& icon_name, unsigned int size, bool defer_icon_loading)
  : TextureArea(NUX_TRACKER_LOCATION),
    _accept_key_nav_focus(false),
    _icon_name(!icon_name.empty() ? icon_name : DEFAULT_ICON),
    _size(size),
    _loading(false),
    _opacity(1.0f),
    _handle(0),
    _draw_mode(DrawMode::NORMAL)
{
  if (!icon_name.empty () && !defer_icon_loading)
    LoadIcon();
}

IconTexture::~IconTexture()
{
  IconLoader::GetDefault().DisconnectHandle(_handle);
}

void IconTexture::SetByIconName(std::string const& icon_name, unsigned int size)
{
  if (_icon_name == icon_name && _size == size)
    return;

  _icon_name = icon_name;
  _size = size;

  if (_size == 0)
  {
    _texture_cached = nullptr;
    return;
  }

  LoadIcon();
}


void IconTexture::SetByFilePath(std::string const& file_path, unsigned int size)
{
  SetByIconName(file_path, size);
}

void IconTexture::LoadIcon()
{
  LOG_DEBUG(logger) << "LoadIcon called (" << _icon_name << ") - loading: " << _loading;
  static const char* const DEFAULT_GICON = ". GThemedIcon text-x-preview";

  if (_loading || _size == 0 || _handle)
    return;

  _loading = true;

  glib::Object<GIcon> icon(g_icon_new_for_string(_icon_name.empty() ?  DEFAULT_GICON : _icon_name.c_str(), NULL));

  if (icon.IsType(G_TYPE_ICON))
  {
    _handle = IconLoader::GetDefault().LoadFromGIconString(_icon_name.empty() ? DEFAULT_GICON : _icon_name.c_str(),
                                                           -1, _size,
                                                           sigc::mem_fun(this, &IconTexture::IconLoaded));
  }
  else if (_icon_name.find("http://") == 0)
  {
    _handle = IconLoader::GetDefault().LoadFromURI(_icon_name,
                                                   -1, _size, sigc::mem_fun(this, &IconTexture::IconLoaded));
  }
  else
  {
    _handle = IconLoader::GetDefault().LoadFromIconName(_icon_name,
                                                        -1, _size,
                                                        sigc::mem_fun(this, &IconTexture::IconLoaded));
  }
}

nux::BaseTexture* IconTexture::CreateTextureCallback(std::string const& texid, int width, int height)
{
  return nux::CreateTexture2DFromPixbuf(_pixbuf_cached, true);
}

void IconTexture::Refresh(glib::Object<GdkPixbuf> const& pixbuf)
{
  TextureCache& cache = TextureCache::GetDefault();
  _pixbuf_cached = pixbuf;

  // Cache the pixbuf dimensions so we scale correctly
  _texture_size.width = gdk_pixbuf_get_width(pixbuf);
  _texture_size.height = gdk_pixbuf_get_height(pixbuf);

  // Try and get a texture from the texture cache
  std::string id("IconTexture.");
  id += _icon_name.empty() ? DEFAULT_ICON : _icon_name;
  _texture_cached = cache.FindTexture(id, _texture_size.width, _texture_size.height,
                                      sigc::mem_fun(this, &IconTexture::CreateTextureCallback));
  QueueDraw();
  _loading = false;
}

void IconTexture::IconLoaded(std::string const& icon_name,
                             int max_width,
                             int max_height,
                             glib::Object<GdkPixbuf> const& pixbuf)
{
  _handle = 0;

  if (GDK_IS_PIXBUF(pixbuf.RawPtr()))
  {
    Refresh(pixbuf);
  }
  else
  {
    _pixbuf_cached = nullptr;
    _loading = false;

    // Protects against a missing default icon, we only request it if icon_name
    // doesn't match.
    if (icon_name != DEFAULT_ICON)
      SetByIconName(DEFAULT_ICON, _size);
  }

  texture_updated.emit(_texture_cached);
  QueueDraw();
}

void IconTexture::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  unsigned int current_alpha_blend;
  unsigned int current_src_blend_factor;
  unsigned int current_dest_blend_factor;
  GfxContext.GetRenderStates().GetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::Geometry geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);

  nux::GetPainter().PaintBackground(GfxContext, geo);

  if (_texture_cached)
  {
    nux::Color col(1.0f * _opacity, 1.0f * _opacity, 1.0f * _opacity, _opacity);
    nux::TexCoordXForm texxform;

    if (_draw_mode == DrawMode::STRETCH_WITH_ASPECT)
    {
      nux::Geometry imageDest = geo;
      
      float geo_apsect = float(geo.GetWidth()) / geo.GetHeight();
      float image_aspect = float(_texture_cached->GetWidth()) / _texture_cached->GetHeight();

      if (image_aspect > geo_apsect)
      {
        imageDest.SetHeight(float(imageDest.GetWidth()) / image_aspect);
      } 
      if (image_aspect < geo_apsect)
      {
        imageDest.SetWidth(image_aspect * imageDest.GetHeight());
      }
      else
      {
        imageDest = nux::Geometry(0, 0, _texture_cached->GetWidth(), _texture_cached->GetHeight());
      }

      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_SCALE_COORD);
      texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);
      texxform.SetFilter(nux::TEXFILTER_LINEAR, nux::TEXFILTER_LINEAR);

      texxform.u0 = 0;
      texxform.v0 = 0;
      texxform.u1 = imageDest.width;
      texxform.v1 = imageDest.height;

      int border_width = 1;
      GfxContext.QRP_1Tex(geo.x + (float(geo.GetWidth() - imageDest.GetWidth()) / 2) + border_width,
                          geo.y + (float(geo.GetHeight() - imageDest.GetHeight()) / 2) + border_width,
                          imageDest.width - (border_width * 2),
                          imageDest.height - (border_width * 2),
                          _texture_cached.GetPointer()->GetDeviceTexture(),
                          texxform,
                          col);
    }
    else
    {
      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

      GfxContext.QRP_1Tex(geo.x + ((geo.width - _texture_size.width) / 2),
                          geo.y + ((geo.height - _texture_size.height) / 2),
                          _texture_size.width,
                          _texture_size.height,
                          _texture_cached->GetDeviceTexture(),
                          texxform,
                          col);
    }
  }

  GfxContext.PopClippingRectangle();

  GfxContext.GetRenderStates().SetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
}

void IconTexture::GetTextureSize(int* width, int* height)
{
  if (width)
    *width = _texture_size.width;
  if (height)
    *height = _texture_size.height;
}

void IconTexture::SetOpacity(float opacity)
{
  _opacity = opacity;

  QueueDraw();
}

void IconTexture::SetTexture(BaseTexturePtr const& texture)
{
  if (_texture_cached == texture)
    return;

  _texture_cached = texture;

  if (texture)
  {
    _texture_size.width = texture->GetWidth();
    _texture_size.height = texture->GetHeight();
    _size = _texture_size.height;
    SetMinMaxSize(_texture_size.width, _texture_size.height);
  }

  texture_updated.emit(_texture_cached);
}

void IconTexture::SetTexture(nux::BaseTexture* texture)
{
  SetTexture(BaseTexturePtr(texture));
}

IconTexture::BaseTexturePtr IconTexture::texture()
{
  return _texture_cached;
}

bool IconTexture::DoCanFocus()
{
  return false;
}

std::string IconTexture::GetName() const
{
  return "IconTexture";
}


void IconTexture::AddProperties(debug::IntrospectionData& introspection)
{
  introspection.add(GetAbsoluteGeometry())
               .add("icon_name", _icon_name);
}

//
// Key navigation
//

void IconTexture::SetAcceptKeyNavFocus(bool accept)
{
  _accept_key_nav_focus = accept;
}

bool IconTexture::AcceptKeyNavFocus()
{
  return _accept_key_nav_focus;
}

void IconTexture::SetDrawMode(DrawMode mode)
{
  _draw_mode = mode;
  QueueDraw();
}
}
