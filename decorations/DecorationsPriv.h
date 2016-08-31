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

#ifndef UNITY_DECORATION_MANAGER_PRIV
#define UNITY_DECORATION_MANAGER_PRIV

#include <unordered_map>
#include <NuxCore/NuxCore.h>
#include <NuxCore/Rect.h>
#include <NuxGraphics/CairoGraphics.h>
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/Indicators.h>
#include <core/core.h>
#include <opengl/opengl.h>
#include <composite/composite.h>
#include <X11/extensions/shape.h>

#include "DecorationsShape.h"
#include "DecorationsDataPool.h"
#include "DecorationsManager.h"
#include "DecorationsInputMixer.h"
#include "EMConverter.h"

class CompRegion;

namespace unity
{
namespace decoration
{
extern Manager* manager_;

class Title;
class MenuLayout;
class SlidingLayout;
class ForceQuitDialog;
class WindowButton;

namespace cu = compiz_utils;

namespace atom
{
extern Atom _UNITY_GTK_BORDER_RADIUS;
}

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
  bool IsMaximized() const;
  bool FullyDecorated() const;
  bool ShadowDecorated() const;
  bool ShapedShadowDecorated() const;
  void RedrawDecorations();
  void Damage();
  void SetupAppMenu();
  bool ActivateMenu(std::string const&);
  void ShowForceQuitDialog(bool, Time);
  void SendFrameExtents();

private:
  void UnsetExtents();
  void SetupExtents();
  void ComputeBorderExtent(CompWindowExtents &border);
  void UpdateElements(cu::WindowFilter wf = cu::WindowFilter::NONE);
  void UpdateWindowState(unsigned old_state);
  void UpdateClientDecorationsState();
  void UpdateMonitor();
  void UpdateFrame();
  void CreateFrame(nux::Geometry const&);
  void UpdateFrameGeo(nux::Geometry const&);
  void UpdateFrameActions();
  void UnsetFrame();
  void SetupWindowEdges();
  void CleanupWindowEdges();
  void SetupWindowControls();
  void CleanupWindowControls();
  void UnsetAppMenu();
  void UpdateAppMenuVisibility();
  void SyncXShapeWithFrameRegion();
  void SyncMenusGeometries() const;
  bool ShouldBeDecorated() const;
  GLTexture* ShadowTexture() const;
  GLTexture* SharedShadowTexture() const;
  unsigned ShadowRadius() const;
  std::string const& GetMenusPanelID() const;

  void ComputeShadowQuads();
  void ComputeGenericShadowQuads();
  void ComputeShapedShadowQuad();
  void UpdateDecorationTextures();
  void UpdateWindowEdgesGeo();
  void UpdateForceQuitDialogPosition();
  void RenderDecorationTexture(Side, nux::Geometry const&);
  cu::PixmapTexture::Ptr BuildShapedShadowTexture(nux::Size const&, unsigned radius, nux::Color const&, Shape const&);
  void Paint(GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);
  void Draw(GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);

  friend class Window;
  friend struct Manager::Impl;

  decoration::Window *parent_;
  ::CompWindow* win_;
  ::CompositeWindow* cwin_;
  ::GLWindow* glwin_;
  ::Window frame_;
  int monitor_;
  bool dirty_geo_;
  bool dirty_frame_;
  bool client_decorated_;
  unsigned deco_elements_;
  unsigned last_mwm_decor_;
  unsigned last_actions_;

  CompRect last_shadow_rect_;
  Quads shadow_quads_;
  nux::Geometry frame_geo_;
  CompRegion frame_region_;
  CompWindowExtents client_borders_;
  connection::Wrapper theme_changed_;
  connection::Wrapper dpi_changed_;
  connection::Wrapper grab_mouse_changed_;
  std::string last_title_;
  std::string panel_id_;
  std::vector<cu::SimpleTextureQuad> bg_textures_;
  cu::PixmapTexture::Ptr shaped_shadow_pixmap_;
  std::shared_ptr<ForceQuitDialog> force_quit_;
  InputMixer::Ptr input_mixer_;
  Layout::Ptr top_layout_;
  uweak_ptr<WindowButton> state_change_button_;
  uweak_ptr<MenuLayout> menus_;
  uweak_ptr<Title> title_;
  uweak_ptr<SlidingLayout> sliding_layout_;
  Item::WeakPtr grab_edge_;
  Item::Ptr edge_borders_;

  EMConverter::Ptr cv_;
};

struct Manager::Impl : sigc::trackable
{
  Impl(decoration::Manager*, menu::Manager::Ptr const&);

  Window::Ptr HandleWindow(CompWindow* cwin);
  bool HandleEventBefore(XEvent*);
  bool HandleEventAfter(XEvent*);
  bool HandleFrameEvent(XEvent*);

private:
  Window::Ptr GetWindowByXid(::Window) const;
  Window::Ptr GetWindowByFrame(::Window) const;

  bool UpdateWindow(::Window);
  void UpdateWindowsExtents();

  void SetupIntegratedMenus();
  void SetupAppMenu();
  void UnsetAppMenu();
  void BuildActiveShadowTexture();
  void BuildInactiveShadowTexture();
  cu::PixmapTexture::Ptr BuildShadowTexture(unsigned radius, nux::Color const&);
  void OnShadowOptionsChanged(bool active);
  void OnWindowFrameChanged(bool, ::Window, std::weak_ptr<decoration::Window> const&);
  bool OnMenuKeyActivated(std::string const&);

  friend class Manager;
  friend struct Window::Impl;

  DataPool::Ptr data_pool_;
  cu::PixmapTexture::Ptr active_shadow_pixmap_;
  cu::PixmapTexture::Ptr inactive_shadow_pixmap_;

  uweak_ptr<decoration::Window> active_deco_win_;
  uweak_ptr<InputMixer> last_mouse_owner_;
  std::unordered_map<CompWindow*, decoration::Window::Ptr> windows_;
  std::unordered_map<::Window, std::weak_ptr<decoration::Window>> framed_windows_;

  menu::Manager::Ptr menu_manager_;
  connection::Manager menu_connections_;
  connection::handle appmenu_connection_;
};

} // decoration namespace
} // unity namespace

#endif
