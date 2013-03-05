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
 */

#include "TooltipManager.h"

namespace unity
{
namespace launcher
{

namespace
{
const unsigned int TOOLTIPS_SHOW_TIMEOUT_LENGTH = 1000;
}

TooltipManager::TooltipManager()
  : show_tooltips_(false)
  , hovered_(false)
  , icon_clicked_(false)
{}

void TooltipManager::SetIcon(AbstractLauncherIcon::Ptr const& newIcon)
{ 
  if (icon_ == newIcon)
    return;

  // Unlock hover timer, in case the previous icon had no valid tooltip
  icon_clicked_ = false;
 
  if (show_tooltips_)
  {
    // Show new tooltip, get rid of the old olne
    if (icon_)
      icon_->HideTooltip();
    if (newIcon)
      newIcon->ShowTooltip();
  }
  else if (!newIcon)
  {
    // Stop the hover timer for null launcher space
    StopTimer();
  }
  else
  { 
    AbstractLauncherIcon::IconType type = newIcon->GetIconType();
    if ((type == AbstractLauncherIcon::IconType::HOME ||
         type == AbstractLauncherIcon::IconType::HUD) &&
         newIcon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE))
    {
      // Lock the hover timer for no valid tooltip cases
      icon_clicked_ = true;
      StopTimer();
    }
  }

  icon_ = newIcon;
}

void TooltipManager::SetHover(bool on_launcher)
{ 
  if (hovered_ == on_launcher)
    return;
  hovered_ = on_launcher;

  if (show_tooltips_ && !hovered_)
  {
    show_tooltips_ = false;
    if (icon_)
      icon_->HideTooltip();
  }
}

void TooltipManager::MouseMoved()
{
  if (!icon_ || show_tooltips_)
    return;

  ResetTimer();
}

void TooltipManager::IconClicked()
{
  StopTimer();
  if (show_tooltips_ && icon_)
    icon_->HideTooltip();

  show_tooltips_ = false;
  icon_clicked_ = true;
}

void TooltipManager::ResetTimer()
{
  if (icon_clicked_)
    return;

  hover_timer_.reset(new glib::Timeout(TOOLTIPS_SHOW_TIMEOUT_LENGTH));
  hover_timer_->Run([&] () {
    show_tooltips_ = true;
    icon_->ShowTooltip();
    return false;
  });
}

void TooltipManager::StopTimer()
{
  hover_timer_.reset();
}

} // namespace launcher
} // namespace unity
