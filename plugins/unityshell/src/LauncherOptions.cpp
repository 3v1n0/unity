// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010, 2011 Canonical Ltd
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
 *              Tim Penhey <tim.penhey@canonical.com>
 */

#include "LauncherOptions.h"

namespace unity
{
namespace launcher
{

Options::Options()
{
  hide_mode.changed.connect(sigc::mem_fun (this, &Options::OnHideModeChanged));
  launch_animation.changed.connect(sigc::mem_fun (this, &Options::OnLaunchAnimationChanged));
  urgent_animation.changed.connect(sigc::mem_fun (this, &Options::OnUrgentAnimationChanged));
  auto_hide_animation.changed.connect(sigc::mem_fun (this, &Options::OnAutoHideAnimationChanged));
  backlight_mode.changed.connect(sigc::mem_fun (this, &Options::OnBacklightModeChanged));
  icon_size.changed.connect(sigc::mem_fun (this, &Options::OnIconSizeChanged));
  tile_size.changed.connect(sigc::mem_fun (this, &Options::OnTileSizeChanged));
  floating.changed.connect(sigc::mem_fun (this, &Options::OnFloatingChanged));
  background_alpha.changed.connect(sigc::mem_fun (this, &Options::OnBackgroundAlphaChanged));
}

void Options::OnHideModeChanged(LauncherHideMode value)
{
  option_changed.emit();
}

void Options::OnLaunchAnimationChanged(LaunchAnimation value)
{
  option_changed.emit();
}

void Options::OnUrgentAnimationChanged(UrgentAnimation value)
{
  option_changed.emit();
}

void Options::OnAutoHideAnimationChanged(AutoHideAnimation value)
{
  option_changed.emit();
}

void Options::OnBacklightModeChanged(BacklightMode value)
{
  option_changed.emit();
}

void Options::OnIconSizeChanged(int value)
{
  option_changed.emit();
}

void Options::OnTileSizeChanged(int value)
{
  option_changed.emit();
}

void Options::OnFloatingChanged(bool value)
{
  option_changed.emit();
}

void Options::OnBackgroundAlphaChanged(float value)
{
  option_changed.emit();
}


}
}