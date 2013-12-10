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

#ifndef UNITY_DECORATION_MANAGER_PRIV
#define UNITY_DECORATION_MANAGER_PRIV

#include <X11/Xlib.h>
#include <Nux/BaseWindow.h>
#include "DecorationsManager.h"
#include "DecorationStyle.h"
#include "CompizUtils.h"
#include "unityshell.h"

class CompRegion;

namespace unity
{
namespace decoration
{
namespace cu = compiz_utils;

extern Display* dpy;
extern Manager* manager_;

struct Quads
{
  enum class Pos
  {
    TOP_LEFT = 0,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
  };

  cu::TextureQuad& operator[](Pos position) { return inner_vector_[unsigned(position)]; }
  cu::TextureQuad const& operator[](Pos position) const { return inner_vector_[unsigned(position)]; }
  std::size_t size() const { return inner_vector_.size(); }

private:
  std::array<cu::TextureQuad, 4> inner_vector_;
};

struct Window::Impl
{
  Impl(decoration::Window*, UnityWindow*);
  ~Impl();

  nux::Property<bool> active;

  void Update();
  void Undecorate();
  bool FullyDecorated() const;
  bool ShadowDecorated() const;

private:
  void UnsetExtents();
  void SetupExtents();
  void UnsetFrame();
  void UpdateFrame();
  void SyncXShapeWithFrameRegion();
  bool ShouldBeDecorated() const;
  GLTexture* ShadowTexture() const;
  unsigned ShadowRadius() const;

  void ComputeShadowQuads();
  void UpdateDecorationTextures();
  void RenderDecorationTexture(Side, nux::Geometry const&);
  void Draw(GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);

  friend class Window;
  friend struct Manager::Impl;

  decoration::Window *parent_;
  ::UnityWindow* uwin_;
  ::Window frame_;
  bool dirty_geo_;

  CompRect last_shadow_rect_;
  Quads shadow_quads_;
  nux::Geometry frame_geo_;
  CompRegion frame_region_;
  std::vector<cu::SimpleTextureQuad> bg_textures_;
};

struct Manager::Impl : sigc::trackable
{
  Impl(decoration::Manager*, UnityScreen*);
  ~Impl();

private:
  friend class Manager;
  friend struct Window::Impl;

  bool HandleEventBefore(XEvent*);
  bool HandleEventAfter(XEvent*);

  Window::Ptr GetWindowByXid(::Window);
  Window::Ptr GetWindowByFrame(::Window);

  bool UpdateWindow(::Window);
  void UpdateWindowsExtents();

  void BuildActiveShadowTexture();
  void BuildInactiveShadowTexture();
  cu::PixmapTexture::Ptr BuildShadowTexture(unsigned radius, nux::Color const&);
  void OnShadowOptionsChanged(bool active);

  ::UnityScreen* uscreen_;
  ::Window active_window_;
  cu::PixmapTexture::Ptr active_shadow_pixmap_;
  cu::PixmapTexture::Ptr inactive_shadow_pixmap_;

  std::map<UnityWindow*, decoration::Window::Ptr> windows_;
};

} // decoration namespace
} // unity namespace

#endif
