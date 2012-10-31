// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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


#include "HudIconTextureSource.h"
#include "config.h"

#include <glib.h>

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

namespace unity
{
namespace hud
{
DECLARE_LOGGER(logger, "unity.hud.icon.texturesource");

HudIconTextureSource::HudIconTextureSource(nux::ObjectPtr<nux::BaseTexture> texture)
  : unity::ui::IconTextureSource()
  , icon_texture_(texture)
{
}

HudIconTextureSource::~HudIconTextureSource()
{
}

void HudIconTextureSource::ColorForIcon(GdkPixbuf* pixbuf)
{
  if (GDK_IS_PIXBUF(pixbuf))
  {
    unsigned int width = gdk_pixbuf_get_width(pixbuf);
    unsigned int height = gdk_pixbuf_get_height(pixbuf);
    unsigned int row_bytes = gdk_pixbuf_get_rowstride(pixbuf);
    
    long int rtotal = 0, gtotal = 0, btotal = 0;
    float total = 0.0f;
    
    guchar* img = gdk_pixbuf_get_pixels(pixbuf);
    
    for (unsigned int i = 0; i < width; i++)
    {
      for (unsigned int j = 0; j < height; j++)
      {
        guchar* pixels = img + (j * row_bytes + i * 4);
        guchar r = *(pixels + 0);
        guchar g = *(pixels + 1);
        guchar b = *(pixels + 2);
        guchar a = *(pixels + 3);
        
        float saturation = (MAX(r, MAX(g, b)) - MIN(r, MIN(g, b))) / 255.0f;
        float relevance = .1 + .9 * (a / 255.0f) * saturation;
        
        rtotal += (guchar)(r * relevance);
        gtotal += (guchar)(g * relevance);
        btotal += (guchar)(b * relevance);
        
        total += relevance * 255;
      }
    }
    
    nux::color::RedGreenBlue rgb(rtotal / total,
                                 gtotal / total,
                                 btotal / total);
    nux::color::HueSaturationValue hsv(rgb);
    
    if (hsv.saturation > 0.15f)
      hsv.saturation = 0.65f;
    
    hsv.value = 0.90f;
    bg_color = nux::Color(nux::color::RedGreenBlue(hsv));
  }
  else
  {
    LOG_ERROR(logger) << "Pixbuf (" << pixbuf << ") passed is non valid";
    bg_color = nux::Color(255,255,255,255);
  }
}

nux::Color HudIconTextureSource::BackgroundColor() const
{
  return bg_color;
}

nux::BaseTexture* HudIconTextureSource::TextureForSize(int size)
{
  return icon_texture_.GetPointer();
}

nux::Color HudIconTextureSource::GlowColor()
{
 return nux::Color(0.0f, 0.0f, 0.0f, 0.0f); 
}

nux::BaseTexture* HudIconTextureSource::Emblem()
{
  return nullptr;
}

}
}

