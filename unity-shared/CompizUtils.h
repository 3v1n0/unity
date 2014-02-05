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

#ifndef UNITY_COMPIZ_UTILS
#define UNITY_COMPIZ_UTILS

#include <opengl/opengl.h>
#include <cairo.h>
#include <memory>

namespace unity
{
namespace compiz_utils
{

struct TextureQuad
{
  CompRect box;
  GLTexture::Matrix matrix;
};

struct SimpleTexture
{
  typedef std::shared_ptr<SimpleTexture> Ptr;

  SimpleTexture() = default;
  SimpleTexture(GLTexture::List const&);
  virtual ~SimpleTexture() = default;

  GLTexture::List const& texture_list() const { return texture_; }
  GLTexture* texture() const { return texture_.empty() ? nullptr : texture_[0]; }
  int width() const { return texture_.empty() ? 0 : texture_[0]->width(); }
  int height() const { return texture_.empty() ? 0 : texture_[0]->height(); }

  operator GLTexture*() const { return texture(); }
  operator GLTexture::List() const { return texture_; }

protected:
  GLTexture::List texture_;
};

struct SimpleTextureQuad
{
  bool SetTexture(SimpleTexture::Ptr const&);
  bool SetCoords(int x, int y);
  bool SetX(int x);
  bool SetY(int y);

  operator SimpleTexture::Ptr() const { return st; }
  operator bool() const { return st && st->texture(); }
  operator GLTexture*() const { return st ? st->texture() : nullptr; }
  operator GLTexture::List() const { return st ? st->texture_list() : GLTexture::List(); }

  SimpleTexture::Ptr st;
  TextureQuad quad;
};

struct PixmapTexture : SimpleTexture
{
  typedef std::shared_ptr<PixmapTexture> Ptr;

  PixmapTexture(int width, int height);
  ~PixmapTexture();

  Pixmap pixmap() const { return pixmap_; }

private:
  Pixmap pixmap_;
};

struct CairoContext
{
  CairoContext(int width, int height);
  ~CairoContext();

  int width() const;
  int height() const;

  PixmapTexture::Ptr const& texture() const { return pixmap_texture_; }
  cairo_t* context() const { return cr_; }

  operator cairo_t*() const { return cr_; }
  operator PixmapTexture::Ptr() const { return pixmap_texture_; }
  operator SimpleTexture::Ptr() const { return pixmap_texture_; }

private:
  PixmapTexture::Ptr pixmap_texture_;
  cairo_surface_t* surface_;
  cairo_t *cr_;
};

bool IsWindowShadowDecorable(CompWindow*);
bool IsWindowFullyDecorable(CompWindow*);

} // compiz_utils namespace
} // unity namespace

std::ostream& operator<<(std::ostream &os, CompRect const& r);

#endif // UNITY_COMPIZ_UTILS
