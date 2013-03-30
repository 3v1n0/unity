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


#ifndef OVERLAYRENDERER_H
#define OVERLAYRENDERER_H

#include <memory>

#include <Nux/Nux.h>
#include <Nux/View.h>

namespace unity 
{

class OverlayRendererImpl;
class OverlayRenderer
{
public:
  // We only ever want one OverlayRenderer per view, so if you must take a pointer, take this unique one that will die 
  // when it goes out of scope
  typedef std::unique_ptr<OverlayRenderer> Ptr;

  nux::Property<int> x_offset;
  nux::Property<int> y_offset;
  
  OverlayRenderer();
  ~OverlayRenderer();
  
  /*
   * Call when we are about to show, gets the blur ready for rendering
   */
  void AboutToShow();
  
  /*
   * Call when the interface is hiding, saves on resources
   */
  void AboutToHide();
  
  /*
   * Call this whenever the interface size changes
   */
  void UpdateBlurBackgroundSize (nux::Geometry const& content_geo,
                                 nux::Geometry const& absolute_geo,
                                 bool                 force_edges);

  /*
   * Disables the blur, if you need it disabled. can not re-enable it.
   */
  void DisableBlur();
  
  /*
   * Needed internally, should be called with yourself as the owner as soon as possible
   */
  void SetOwner(nux::View *owner);
  
  /*
   * Draws the entire stack of visuals using direct rendering, use in the Draw() call, not DrawContent()
   * 
   * content_geo: the geometry of the content we are renderering, should be smaller than geo
   * absolute_geo: your views GetAbsoluteGeometry()
   * geo: your views GetGeometry()
   */
  void DrawFull(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo, bool force_edges=false);
  
  /*
   * Draws just the stack that is overlay behind the inner_geometry using push/pop layers, call in DrawContent() before drawing your content
   */
  void DrawInner(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo);
  
  /* 
   * Call after calling DrawInner and drawing your own content
   */
  void DrawInnerCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo);
  
  sigc::signal<void> need_redraw;

private:
  friend class OverlayRendererImpl;
  OverlayRendererImpl *pimpl_;
};

}
#endif // OVERLAYRENDERER_H
