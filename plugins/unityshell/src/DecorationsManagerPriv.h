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

#include "DecorationsManager.h"
#include "unityshell.h"
#include <X11/Xlib.h>
#include <Nux/BaseWindow.h>

class CompRegion;

namespace unity
{
namespace decoration
{

class PixmapTexture;

struct Quads
{
  struct Quad
  {
    CompRect box;
    GLTexture::Matrix matrix;
  };

  enum class Pos
  {
    TOP_LEFT = 0,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    LAST
  };

  Quad& operator[](Pos position) { return inner_vector_[unsigned(position)]; }
  Quad const& operator[](Pos position) const { return inner_vector_[unsigned(position)]; }

private:
  Quad inner_vector_[unsigned(Pos::LAST)];
};

struct Window::Impl
{
  Impl(decoration::Window*, UnityWindow*);
  ~Impl();

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

  void ComputeShadowQuads();
  void Draw(GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);

  friend class Window;
  friend struct Manager::Impl;

  decoration::Window *parent_;
  ::UnityWindow* uwin_;
  ::Window frame_;

  CompRect last_shadow_rect_;
  Quads shadow_quads_;
  nux::Geometry frame_geo_;
  CompRegion frame_region_;
};

struct Manager::Impl
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
  void BuildShadowTexture();
  void OnShadowOptionsChanged();

  ::UnityScreen* uscreen_;
  ::Window active_window_;
  std::shared_ptr<PixmapTexture> shadow_pixmap_;

  std::map<UnityWindow*, decoration::Window::Ptr> windows_;
};

} // decoration namespace
} // unity namespace

#endif
