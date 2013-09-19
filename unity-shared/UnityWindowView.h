// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2013 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITYWINDOWVIEW_H
#define UNITYWINDOWVIEW_H

#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/IconTexture.h"
#include "Introspectable.h"
#include "UnityWindowStyle.h"
#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <NuxCore/Property.h>

namespace unity {
namespace ui {

class UnityWindowView : public debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(UnityWindowView, nux::View)
public:
  nux::RWProperty<bool> live_background;
  nux::Property<nux::Color> background_color;
  nux::Property<UnityWindowStyle::Ptr> style;
  nux::Property<bool> closable;

  UnityWindowView(NUX_FILE_LINE_PROTO);
  virtual ~UnityWindowView();

  bool SetLayout(nux::Layout* layout) override;
  nux::Layout* GetLayout() override;

  nux::ObjectPtr<nux::InputArea> GetBoundingArea();

  sigc::signal<void> request_close;

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);
  nux::Area* FindKeyFocusArea(unsigned etype, unsigned long key_code, unsigned long modifiers);

  virtual void PreDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {};
  virtual void DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry const& clip) = 0;
  nux::Geometry GetInternalBackground();
  virtual nux::Geometry GetBackgroundGeometry() = 0;

  // Introspectable methods
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  friend class TestUnityWindowView;

  void OnClosableChanged(bool closable);
  void DrawBackground(nux::GraphicsEngine& GfxContext, nux::Geometry const& geo);

  nux::Layout *internal_layout_;
  BackgroundEffectHelper bg_helper_;
  nux::ObjectPtr<nux::InputArea> bounding_area_;
  nux::ObjectPtr<IconTexture> close_button_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> bg_texture_;
};

}
}

#endif
