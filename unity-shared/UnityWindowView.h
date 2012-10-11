// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef UNITYWINDOWVIEW_H
#define UNITYWINDOWVIEW_H

#include "unity-shared/BackgroundEffectHelper.h"
#include "Introspectable.h"
#include "UnityWindowStyle.h"
#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxCore/ObjectPtr.h>
#include <NuxCore/Property.h>

namespace unity {
namespace ui {

class UnityWindowView : public debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(UnityWindowView, nux::View)  
public:
  nux::Property<nux::Color> background_color;
  nux::Property<UnityWindowStyle::Ptr> style;

  UnityWindowView(NUX_FILE_LINE_PROTO);
  virtual ~UnityWindowView();

  void SetupBackground(bool enabled = true);

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  virtual void PreDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {};
  virtual void DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry clip) = 0;
  virtual nux::Geometry GetBackgroundGeometry() = 0;

  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  void DrawBackground(nux::GraphicsEngine& GfxContext, nux::Geometry const& geo);

  BackgroundEffectHelper bg_helper_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> bg_texture_;
};

}
}

#endif
