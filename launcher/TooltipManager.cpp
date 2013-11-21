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
 * Authored by: Jacob Edwards <j.johan.edwards@gmail.com>
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "TooltipManager.h"

namespace unity
{
namespace launcher
{
namespace
{
const unsigned int TOOLTIPS_SHOW_TIMEOUT_LENGTH = 500;
}

TooltipManager::TooltipManager()
  : skip_timeout_(false)
{}

void TooltipManager::MouseMoved(AbstractLauncherIcon::Ptr const& icon_under_mouse)
{
  if (icon_ == icon_under_mouse)
    return;

  StopTimer();
  if (icon_)
    icon_->HideTooltip();

  icon_ = icon_under_mouse;

  if (!icon_)
    return;

  AbstractLauncherIcon::IconType type = icon_->GetIconType();
  if ((type == AbstractLauncherIcon::IconType::HOME || type == AbstractLauncherIcon::IconType::HUD) &&
      icon_->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE))
  {
    Reset();
    return;
  }
  
  if (!skip_timeout_)
    ResetTimer(icon_);
  else if (skip_timeout_)
    icon_->ShowTooltip();
}

void TooltipManager::IconClicked()
{
  Reset();
}

void TooltipManager::SetHover(bool hovered)
{
  if (!hovered)
    Reset();
}

void TooltipManager::Reset()
{
  StopTimer();

  if (icon_)
    icon_->HideTooltip();

  icon_ = AbstractLauncherIcon::Ptr();
  skip_timeout_ = false;
}

void TooltipManager::ResetTimer(AbstractLauncherIcon::Ptr const& icon_under_mouse)
{
  hover_timer_.reset(new glib::Timeout(TOOLTIPS_SHOW_TIMEOUT_LENGTH));
  hover_timer_->Run([this, icon_under_mouse] () {
    skip_timeout_ = true;
    icon_under_mouse->ShowTooltip();
    return false;
  });
 }
 
void TooltipManager::StopTimer()
{
  hover_timer_.reset(); 
}

}
}
