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
   : _show_tooltips(false)
   , _hovered(false)
   , _icon(nullptr)
   , _icon_clicked(false)
{}

void TooltipManager::SetIcon(AbstractLauncherIcon::Ptr const& newIcon) { 
  if (_icon == newIcon)
    return;

  // Unlock hover timer, in case the previous icon had no valid tooltip
  _icon_clicked = false;
 
  if (_show_tooltips) {
    // Show new tooltip, get rid of the old olne
    if (_icon)
      _icon->HideTooltip();
    if (newIcon)
      newIcon->ShowTooltip();
  }
  else if (!newIcon) {
    // Stop the hover timer for null launcher space
    StopTimer();
  }
  else { 
    AbstractLauncherIcon::IconType type = newIcon->GetIconType();
    if ((type == AbstractLauncherIcon::IconType::HOME ||
         type == AbstractLauncherIcon::IconType::HUD) &&
         newIcon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE)) {
      // Lock the hover timer for no valid tooltip cases
      _icon_clicked = true;
      StopTimer();
    }
  }

  _icon = newIcon;
}

void TooltipManager::SetHover(bool on_launcher) { 
  if (_hovered == on_launcher) {
    return;
  }
  _hovered = on_launcher;

  if (_show_tooltips && !_hovered) {
    _show_tooltips = false;
    if (_icon)
      _icon->HideTooltip();
  }
}

void TooltipManager::MouseMoved() {
  if (!_icon || _show_tooltips)
    return;

  ResetTimer();
}

void TooltipManager::IconClicked() {
  StopTimer();
  if (_show_tooltips && _icon)
    _icon->HideTooltip();

  _show_tooltips = false;
  _icon_clicked = true;
}

void TooltipManager::ResetTimer() {
  if (_icon_clicked)
    return;

  StopTimer();
  _hover_timer->Run([&] () {
    _show_tooltips = true;
    _icon->ShowTooltip();
    return false;
  });
}

void TooltipManager::StopTimer() {
  _hover_timer.reset(new glib::Timeout(TOOLTIPS_SHOW_TIMEOUT_LENGTH));
}

} // namespace launcher
} // namespace unity
