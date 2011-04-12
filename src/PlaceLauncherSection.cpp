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

#include "PlaceLauncherIcon.h"

#include "PlaceLauncherSection.h"

PlaceLauncherSection::PlaceLauncherSection (Launcher *launcher)
: _launcher (launcher),
  _priority (10000)
{
  _factory = PlaceFactory::GetDefault ();
  _on_place_added_connection = (sigc::connection) _factory->place_added.connect (sigc::mem_fun (this, 
                                                                                                &PlaceLauncherSection::OnPlaceAdded));

  PopulateEntries ();
}

PlaceLauncherSection::~PlaceLauncherSection ()
{
  if (_on_place_added_connection.connected ())
    _on_place_added_connection.disconnect ();
}

void
PlaceLauncherSection::OnPlaceAdded (Place *place)
{
  std::vector<PlaceEntry *> entries = place->GetEntries ();
  std::vector<PlaceEntry *>::iterator i;

  for (i = entries.begin (); i != entries.end (); ++i)
  {
    PlaceEntry *entry = static_cast<PlaceEntry *> (*i);
    
    if (entry->ShowInLauncher ())
    {
      PlaceLauncherIcon *icon = new PlaceLauncherIcon (_launcher, entry);
      icon->SetSortPriority (_priority++);
      IconAdded.emit (icon);
    }
  }
}

void
PlaceLauncherSection::PopulateEntries ()
{
  std::vector<Place *> places = _factory->GetPlaces ();
  std::vector<Place *>::iterator it;

  for (it = places.begin (); it != places.end (); ++it)
  {
    Place *place = static_cast<Place *> (*it);
    std::vector<PlaceEntry *> entries = place->GetEntries ();
    std::vector<PlaceEntry *>::iterator i;

    for (i = entries.begin (); i != entries.end (); ++i)
    {
      PlaceEntry *entry = static_cast<PlaceEntry *> (*i);

      if (entry->ShowInLauncher ())
      {
        PlaceLauncherIcon *icon = new PlaceLauncherIcon (_launcher, entry);
        icon->SetSortPriority (_priority++);
        IconAdded.emit (icon);
      }
    }
  }
}
