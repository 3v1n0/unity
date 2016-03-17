// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013-2014 Canonical Ltd
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
  const unsigned PIXMAP_DEPTH  = 32;
  const float    DEFAULT_SCALE = 1.0f;
  const unsigned DECORABLE_WINDOW_TYPES = CompWindowTypeDialogMask |
                                          CompWindowTypeModalDialogMask |
                                          CompWindowTypeUtilMask |
                                          CompWindowTypeMenuMask |
                                          CompWindowTypeNormalMask;
}

SimpleTexture::SimpleTexture(GLTexture::List const& tex)
  : texture_(tex)
{}

//

SimpleTextureQuad::SimpleTextureQuad()
  : scale_(DEFAULT_SCALE)
{}

bool SimpleTextureQuad::SetTexture(SimpleTexture::Ptr const& simple_texture)
{
  if (st == simple_texture)
    return false;

  st = simple_texture;

  if (st && st->texture())
  {
    auto* tex = st->texture();
    CompSize size(tex->width() * scale_, tex->height() * scale_);

    if (quad.box.width() != size.width() || quad.box.height() != size.height())
    {
      quad.box.setSize(size);
      UpdateMatrix();
    }
  }

  return true;
}

bool SimpleTextureQuad::SetScale(double s)
{
  if (!st || scale_ == s)
    return false;

  scale_ = s;
  auto* tex = st->texture();
  quad.box.setWidth(tex->width() * scale_);
  quad.box.setHeight(tex->height() * scale_);
  UpdateMatrix();
  return true;
}

bool SimpleTextureQuad::SetCoords(int x, int y)
{
  if (x == quad.box.x() && y == quad.box.y())
    return false;

  quad.box.setX(x);
  quad.box.setY(y);
  UpdateMatrix();
  return true;
}

void SimpleTextureQuad::UpdateMatrix()
{
  int x = quad.box.x();
  int y = quad.box.y();

  quad.matrix = (st && st->texture()) ? st->texture()->matrix() : GLTexture::Matrix();
  quad.matrix.xx /= scale_;
  quad.matrix.yy /= scale_;
  quad.matrix.x0 = 0.0f - COMP_TEX_COORD_X(quad.matrix, x);
  quad.matrix.y0 = 0.0f - COMP_TEX_COORD_Y(quad.matrix, y);
}

bool SimpleTextureQuad::SetX(int x)
{
  return SetCoords(x, quad.box.y());
}

bool SimpleTextureQuad::SetY(int y)
{
  return SetCoords(quad.box.x(), y);
}

//

PixmapTexture::PixmapTexture(int w, int h)
 : pixmap_(0)
{
  if (w > 0 && h > 0)
  {
    pixmap_ = XCreatePixmap(screen->dpy(), screen->root(), w, h, PIXMAP_DEPTH);
    texture_ = GLTexture::bindPixmapToTexture(pixmap_, w, h, PIXMAP_DEPTH);
  }
}

PixmapTexture::~PixmapTexture()
{
  texture_.clear();

  if (pixmap_)
    XFreePixmap(screen->dpy(), pixmap_);
}

//

CairoContext::CairoContext(int w, int h, double scale)
  : pixmap_texture_(std::make_shared<PixmapTexture>(w, h))
  , surface_(nullptr)
  , cr_(nullptr)
{
  Screen *xscreen = ScreenOfDisplay(screen->dpy(), screen->screenNum());
  XRenderPictFormat* format = XRenderFindStandardFormat(screen->dpy(), PictStandardARGB32);
  surface_ = cairo_xlib_surface_create_with_xrender_format(screen->dpy(), pixmap_texture_->pixmap(),
                                                           xscreen, format, w, h);
  cairo_surface_set_device_scale(surface_, scale, scale);

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

//
//

unsigned WindowDecorationElements(CompWindow* win, WindowFilter wf)
{
  unsigned elements = DecorationElement::NONE;

  if (!win)
    return elements;

  if (!win->isViewable() && wf == WindowFilter::NONE)
    return elements;

  if (win->wmType() & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
    return elements;

  auto const& region = win->region();
  bool rectangular = (region.numRects() == 1);
  bool alpha = win->alpha();

  if (alpha)
  {
    if (wf == WindowFilter::CLIENTSIDE_DECORATED)
    {
      elements = DecorationElement::SHADOW;

      if (win->actions() & CompWindowActionResizeMask)
        elements |= DecorationElement::EDGE;

      return elements;
    }
    else if (!rectangular) // Non-rectangular windows with alpha channel
    {
      return elements;
    }
  }

  if (region.boundingRect() != win->geometry()) // Shaped windows
    return elements;

  if (rectangular)
    elements |= DecorationElement::SHADOW;

  if (!win->overrideRedirect() &&
      (win->type() & DECORABLE_WINDOW_TYPES) &&
      (win->frame() || win->hasUnmapReference() || wf == WindowFilter::UNMAPPED))
  {
    if (win->actions() & CompWindowActionResizeMask)
      elements |= DecorationElement::EDGE;

    if (rectangular && (win->mwmDecor() & (MwmDecorAll | MwmDecorTitle)))
      elements |= DecorationElement::BORDER;
  }

  if (alpha && !(elements & DecorationElement::BORDER) && !(win->mwmDecor() & MwmDecorBorder))
    elements &= ~DecorationElement::SHADOW;

  return elements;
}

bool IsWindowEdgeDecorable(CompWindow* win)
{
  return WindowDecorationElements(win) & DecorationElement::EDGE;
}

bool IsWindowShadowDecorable(CompWindow* win)
{
  return WindowDecorationElements(win) & DecorationElement::SHADOW;
}

bool IsWindowFullyDecorable(CompWindow* win)
{
  return WindowDecorationElements(win) & DecorationElement::BORDER;
}

} // compiz_utils namespace
} // unity namespace

std::ostream& operator<<(std::ostream &os, CompRect const& r)
{
  return os << "CompRect: coords = " << r.x() << "x" << r.y() << ", size = " << r.width() << "x" << r.height();
}
