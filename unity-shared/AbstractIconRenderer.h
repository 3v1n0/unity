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

#ifndef ABSTRACTICONRENDERER_H
#define ABSTRACTICONRENDERER_H

#include <Nux/Nux.h>

#include "IconTextureSource.h"

namespace unity
{
namespace ui
{

enum PipRenderStyle 
{
  OUTSIDE_TILE,
  OVER_TILE,
};

class RenderArg
{
public:
  RenderArg()
    : icon(0)
    , colorify(nux::color::White)
    , alpha(1.0f)
    , saturation(1.0f)
    , backlight_intensity(0.0f)
    , glow_intensity(0.0f)
    , shimmer_progress(0.0f)
    , progress(0.0f)
    , progress_bias(-1.0f)
    , running_arrow(false)
    , running_colored(false)
    , running_on_viewport(false)
    , draw_edge_only(false)
    , active_arrow(false)
    , active_colored(false)
    , skip(false)
    , stick_thingy(false)
    , keyboard_nav_hl(false)
    , draw_shortcut(false)
    , system_item(false)
    , colorify_background(false)
    , window_indicators(0)
    , shortcut_label(0)
  {
  }

  IconTextureSource* icon;
  nux::Point3   render_center;
  nux::Point3   logical_center;
  nux::Vector3  rotation;
  nux::Color    colorify;
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
  bool          draw_edge_only;
  bool          active_arrow;
  bool          active_colored;
  bool          skip;
  bool          stick_thingy;
  bool          keyboard_nav_hl;
  bool          draw_shortcut;
  bool          system_item;
  bool          colorify_background;
  int           window_indicators;
  char          shortcut_label;
};

class AbstractIconRenderer
{
public:
  typedef std::shared_ptr<AbstractIconRenderer> Ptr;

  virtual ~AbstractIconRenderer() {}

  nux::Property<PipRenderStyle> pip_style;
  nux::Property<int> monitor;

  // RenderArgs not const in case processor needs to modify positions to do a perspective correct.
  virtual void PreprocessIcons(std::list<RenderArg>& args, nux::Geometry const& target_window) = 0;

  virtual void RenderIcon(nux::GraphicsEngine& GfxContext, RenderArg const& arg, nux::Geometry const& anchor_geo, nux::Geometry const& owner_geo) = 0;

  virtual void SetTargetSize(int tile_size, int image_size, int spacing) = 0;
};

}
}

#endif // ABSTRACTICONRENDERER_H

