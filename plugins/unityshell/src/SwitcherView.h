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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef SWITCHERVIEW_H
#define SWITCHERVIEW_H

#include "SwitcherModel.h"
#include "AbstractIconRenderer.h"
#include "StaticCairoText.h"

#include <boost/shared_ptr.hpp>
#include <sigc++/sigc++.h>

#include <Nux/View.h>
#include <NuxCore/Property.h>

using namespace unity::ui;

namespace unity {
namespace switcher {

class SwitcherView : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE (SwitcherView, nux::View);
public:
  SwitcherView(NUX_FILE_LINE_PROTO);
  virtual ~SwitcherView();

  void SetModel (SwitcherModel::Ptr model);
  SwitcherModel::Ptr GetModel ();

  nux::Property<int> border_size;
  nux::Property<int> flat_spacing;
  nux::Property<int> icon_size;
  nux::Property<int> minimum_spacing;
  nux::Property<int> tile_size;
  nux::Property<int> vertical_size;
  nux::Property<int> text_size;

protected:
  long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  void Draw (nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);

  RenderArg InterpolateRenderArgs (RenderArg const& start, RenderArg const& end, float progress);

  std::list<RenderArg> RenderArgsMechanical (nux::Geometry& background_geo, AbstractLauncherIcon *selection, timespec const& current);
  
  std::list<RenderArg> RenderArgsFlat (nux::Geometry& background_geo, int selection, timespec const& current);

  RenderArg CreateBaseArgForIcon (AbstractLauncherIcon *icon);
private:
  void OnSelectionChanged (AbstractLauncherIcon *selection);

  static gboolean OnDrawTimeout (gpointer data);

  AbstractIconRenderer::Ptr icon_renderer_;
  SwitcherModel::Ptr model_;
  bool target_sizes_set_;

  guint redraw_handle_;

  nux::BaseTexture * background_texture_;

  nux::StaticCairoText * text_view_;
};

}
}

#endif // SWITCHERVIEW_H

