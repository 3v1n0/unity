// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Marco Trevisan <marco.trevisan@canonical.com>
*/

#include "CompizUtils.h"
#include <cairo-xlib.h>
#include <cairo-xlib-xrender.h>
#include <core/screen.h>

namespace unity
{
namespace compiz_utils
{
namespace
{
  const unsigned PIXMAP_DEPTH = 32;
}

PixmapTexture::PixmapTexture(unsigned w, unsigned h)
  : pixmap_(XCreatePixmap(screen->dpy(), screen->root(), w, h, PIXMAP_DEPTH))
  , texture_(GLTexture::bindPixmapToTexture(pixmap_, w, h, PIXMAP_DEPTH))
{}

PixmapTexture::~PixmapTexture()
{
  texture_.clear();

  if (pixmap_)
    XFreePixmap(screen->dpy(), pixmap_);
}

//

CairoContext::CairoContext(unsigned w, unsigned h)
  : pixmap_texture_(std::make_shared<PixmapTexture>(w, h))
  , surface_(nullptr)
  , cr_(nullptr)
{
  Screen *xscreen = ScreenOfDisplay(screen->dpy(), screen->screenNum());
  XRenderPictFormat* format = XRenderFindStandardFormat(screen->dpy(), PictStandardARGB32);
  surface_ = cairo_xlib_surface_create_with_xrender_format(screen->dpy(), pixmap_texture_->pixmap(),
                                                           xscreen, format, w, h);

  cr_ = cairo_create(surface_);
  cairo_save(cr_);
  cairo_set_operator(cr_, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr_);
  cairo_restore(cr_);
}

CairoContext::~CairoContext()
{
  if (cr_)
    cairo_destroy(cr_);

  if (surface_)
    cairo_surface_destroy(surface_);
}

int CairoContext::width() const
{
  return cairo_xlib_surface_get_width(surface_);
}

int CairoContext::height() const
{
  return cairo_xlib_surface_get_height(surface_);
}

} // compiz_utils namespace
} // unity namespace
