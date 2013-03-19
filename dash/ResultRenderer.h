// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */



#ifndef RESULTRENDERER_H
#define RESULTRENDERER_H

#include <Nux/Nux.h>
#include <UnityCore/Result.h>

namespace unity
{
namespace dash
{

class ResultRenderer : public nux::InitiallyUnownedObject
{
public:
  NUX_DECLARE_OBJECT_TYPE(ResultRenderer, nux::InitiallyUnownedObject);

  enum ResultRendererState
  {
    RESULT_RENDERER_NORMAL,
    RESULT_RENDERER_ACTIVE,
    RESULT_RENDERER_PRELIGHT,
    RESULT_RENDERER_SELECTED,
    RESULT_RENDERER_INSENSITIVE
  };

  ResultRenderer(NUX_FILE_LINE_PROTO);

  virtual void Render(nux::GraphicsEngine& GfxContext,
                      Result& row,
                      ResultRendererState state,
                      nux::Geometry const& geometry,
                      int x_offset, int y_offset,
                      nux::Color const& color,
                      float saturate);

  // this is just to start preloading images and text that the renderer might
  // need - can be ignored
  virtual void Preload(Result const& row);

  // unload any previous grabbed images
  virtual void Unload(Result const& row);

  // get a image to drag
  virtual nux::NBitmapData* GetDndImage(Result const& row) const;

  nux::Property<int> width;
  nux::Property<int> height;

  sigc::signal<void> NeedsRedraw;
};

}
}
#endif // RESULTRENDERER_H
