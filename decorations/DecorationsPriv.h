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

#include <unordered_map>
#include <NuxCore/NuxCore.h>
#include <NuxCore/Rect.h>
#include <core/core.h>
#include <opengl/opengl.h>
#include <composite/composite.h>
#include <X11/extensions/shape.h>

#include "DecorationsDataPool.h"
#include "DecorationsManager.h"
#include "DecorationsTitle.h"
#include "DecorationsInputMixer.h"

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
    BOTTOM_RIGHT,
    Size
  };

  cu::TextureQuad& operator[](Pos position) { return inner_vector_[unsigned(position)]; }
  cu::TextureQuad const& operator[](Pos position) const { return inner_vector_[unsigned(position)]; }
  std::size_t size() const { return inner_vector_.size(); }

private:
  std::array<cu::TextureQuad, size_t(Pos::Size)> inner_vector_;
};

struct Window::Impl
{
  Impl(decoration::Window*, CompWindow*);
  ~Impl();

  nux::Property<bool> active;
  sigc::signal<void, bool, ::Window> framed;

  void Update();
  void Decorate();
  void Undecorate();
  bool FullyDecorated() const;
  bool ShadowDecorated() const;

private:
  void UnsetExtents();
  void SetupExtents();
  void UnsetFrame();
  void UpdateFrame();
  void SetupTopLayout();
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
  ::CompWindow* win_;
  ::CompositeWindow* cwin_;
  ::GLWindow* glwin_;
  ::Window frame_;
  bool dirty_geo_;

  CompRect last_shadow_rect_;
  Quads shadow_quads_;
  nux::Geometry frame_geo_;
  CompRegion frame_region_;
  std::vector<cu::SimpleTextureQuad> bg_textures_;
  InputMixer::Ptr input_mixer_;
  Layout::Ptr top_layout_;
  Title::Ptr title_;
  Item::Ptr edge_borders_;
};

struct Manager::Impl : sigc::trackable
{
  Impl(decoration::Manager*);
  ~Impl();

  Window::Ptr HandleWindow(CompWindow* cwin);
  bool HandleEventBefore(XEvent*);
  bool HandleEventAfter(XEvent*);
  bool HandleFrameEvent(XEvent*);

private:
  Window::Ptr GetWindowByXid(::Window) const;
  Window::Ptr GetWindowByFrame(::Window) const;

  bool UpdateWindow(::Window);
  void UpdateWindowsExtents();

  void BuildActiveShadowTexture();
  void BuildInactiveShadowTexture();
  cu::PixmapTexture::Ptr BuildShadowTexture(unsigned radius, nux::Color const&);
  void OnShadowOptionsChanged(bool active);
  void OnWindowFrameChanged(bool, ::Window, std::weak_ptr<decoration::Window> const&);

  friend class Manager;
  friend struct Window::Impl;

  ::Window active_window_;
  bool enable_add_supported_atoms_;

  DataPool::Ptr data_pool_;
  cu::PixmapTexture::Ptr active_shadow_pixmap_;
  cu::PixmapTexture::Ptr inactive_shadow_pixmap_;

  std::weak_ptr<InputMixer> last_mouse_owner_;
  std::map<CompWindow*, decoration::Window::Ptr> windows_;
  std::unordered_map<::Window, std::weak_ptr<decoration::Window>> framed_windows_;
};

} // decoration namespace
} // unity namespace

#endif
