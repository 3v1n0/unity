// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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

#ifndef ABSTRACTICONRENDERER_H
#define ABSTRACTICONRENDERER_H

namespace unity {
namespace ui {

class RenderArg
{
public:
  LauncherIcon *icon;
  nux::Point3   render_center;
  nux::Point3   logical_center;
  float         x_rotation;
  float         y_rotation;
  float         z_rotation;
  float         alpha;
  float         saturation;
  float         backlight_intensity;
  float         glow_intensity;
  float         shimmer_progress;
  float         progress;
  float         progress_bias;
  bool          running_arrow;
  bool          running_colored;
  bool          running_on_viewport;
  bool          active_arrow;
  bool          active_colored;
  bool          skip;
  bool          stick_thingy;
  bool          keyboard_nav_hl;
  int           window_indicators;
};

class AbstractIconRenderer
{
public:
  virtual ~AbstractIconRenderer();

  virtual void PreprocessIcons (std::list<RenderArg> &args, nux::Geometry target_window) = 0;

  virtual void RenderIcon (nux::GraphicsEngine& GfxContext, RenderArg const &arg, nux::Geometry anchor_geo) = 0;
};

}
}

#endif // ABSTRACTICONRENDERER_H

