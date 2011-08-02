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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef _PLACE_LAUNCHER_SECTION_H_
#define _PLACE_LAUNCHER_SECTION_H_

#include <sigc++/sigc++.h>
#include <sigc++/signal.h>

#include "PlaceFactory.h"

#include "Launcher.h"
#include "LauncherIcon.h"


class PlaceLauncherSection : public sigc::trackable
{
public:
  PlaceLauncherSection(Launcher* launcher);
  ~PlaceLauncherSection();

  sigc::signal<void, LauncherIcon*> IconAdded;

private:
  void PopulateEntries();
  void OnPlaceAdded(Place* place);

  Launcher*     _launcher;
  PlaceFactory* _factory;

  guint32 _priority;
};

#endif // _PLACE_LAUNCHER_SECTION_H_
