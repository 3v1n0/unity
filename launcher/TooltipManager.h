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

#ifndef TOOLTIPMANAGER
#define TOOLTIPMANAGER

#include <boost/noncopyable.hpp>
#include <UnityCore/GLibSource.h>

#include "AbstractLauncherIcon.h"

namespace unity
{
namespace launcher
{

class TooltipManager : public boost::noncopyable
{
public:
  TooltipManager();

  void SetHover(bool on_launcher);
  void SetIcon(AbstractLauncherIcon::Ptr const& newIcon);
  void MouseMoved();
  void IconClicked();

private:
  void ResetTimer();
  void StopTimer();

  bool                      show_tooltips_;
  bool                      hovered_;
  AbstractLauncherIcon::Ptr icon_;
  glib::Source::UniquePtr   hover_timer_;
  bool                      timer_locked_;
};

} // namespace launcher
} // namespace unity 

#endif
