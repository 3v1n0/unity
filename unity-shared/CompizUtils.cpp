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

SimpleTexture::SimpleTexture(GLTexture::List const& tex)
  : texture_(tex)
{}

//

void SimpleTextureQuad::SetTexture(SimpleTexture::Ptr const& simple_texture)
{
  st = simple_texture;

  if (st && st->texture())
  {
    auto* tex = st->texture();
    quad.box.setWidth(tex->width());
    quad.box.setHeight(tex->height());
  }
}

void SimpleTextureQuad::SetCoords(int x, int y)
{
  quad.box.setX(x);
  quad.box.setY(y);
  quad.matrix = (st && st->texture()) ? st->texture()->matrix() : GLTexture::Matrix();
  quad.matrix.x0 = 0.0f - COMP_TEX_COORD_X(quad.matrix, x);
  quad.matrix.y0 = 0.0f - COMP_TEX_COORD_Y(quad.matrix, y);
}

void SimpleTextureQuad::SetX(int x)
{
  SetCoords(x, quad.box.y());
}

void SimpleTextureQuad::SetY(int y)
{
  SetCoords(quad.box.x(), y);
}

//

PixmapTexture::PixmapTexture(int w, int h)
  : pixmap_(XCreatePixmap(screen->dpy(), screen->root(), w, h, PIXMAP_DEPTH))
{
  texture_ = GLTexture::bindPixmapToTexture(pixmap_, w, h, PIXMAP_DEPTH);
}

PixmapTexture::~PixmapTexture()
{
  texture_.clear();

  if (pixmap_)
    XFreePixmap(screen->dpy(), pixmap_);
}

//

CairoContext::CairoContext(int w, int h)
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

std::ostream& operator<<(std::ostream &os, CompRect const& r)
{
  return os << "CompRect: coords = " << r.x() << "x" << r.y() << ", size = " << r.width() << "x" << r.height();
}
