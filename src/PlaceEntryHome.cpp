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

#include "config.h"

#include "PlaceEntryHome.h"

#include <glib/gi18n-lib.h>

PlaceEntryHome::PlaceEntryHome (PlaceFactory *factory)
: _factory (factory),
  _sections_model (NULL),
  _groups_model (NULL),
  _results_model (NULL)
{
  _sections_model = dee_sequence_model_new ();
  dee_model_set_schema (_sections_model, "s", "s", NULL);

  _groups_model = dee_sequence_model_new ();
  dee_model_set_schema (_groups_model, "s", "s", "s", NULL);

  _results_model = dee_sequence_model_new ();
  dee_model_set_schema (_results_model, "s", "s", "u", "s", "s", "s", NULL);

  LoadExistingEntries ();
  _factory->place_added.connect (sigc::mem_fun (this, &PlaceEntryHome::OnPlaceAdded));
}

PlaceEntryHome::~PlaceEntryHome ()
{
  g_object_unref (_sections_model);
  g_object_unref (_groups_model);
  g_object_unref (_results_model);
}

void
PlaceEntryHome::LoadExistingEntries ()
{
  std::vector<Place *> places = _factory->GetPlaces ();
  std::vector<Place *>::iterator it;

  for (it = places.begin (); it != places.end (); ++it)
  {
    Place *place = static_cast<Place *> (*it);
    OnPlaceAdded (place);
  }
}

void
PlaceEntryHome::OnPlaceAdded (Place *place)
{
  std::vector<PlaceEntry *> entries = place->GetEntries ();
  std::vector<PlaceEntry *>::iterator i;

  for (i = entries.begin (); i != entries.end (); ++i)
  {
    PlaceEntry *entry = static_cast<PlaceEntry *> (*i);
    OnPlaceEntryAdded (entry);
  }
}

void
PlaceEntryHome::OnPlaceEntryAdded (PlaceEntry *entry)
{
  _entries.push_back (entry);
  _model_to_group[entry->GetGlobalResultsModel ()] = _entries.size ();

  g_signal_connect (entry->GetGlobalResultsModel (), "row-added",
                    (GCallback)&PlaceEntryHome::OnResultAdded, this);
  g_signal_connect (entry->GetGlobalResultsModel (), "row-removed",
                    (GCallback)&PlaceEntryHome::OnResultRemoved, this);
}

void
PlaceEntryHome::OnResultAdded (DeeModel *model, DeeModelIter *iter, PlaceEntryHome *self)
{
  g_debug ("Result added");
}
 

void
PlaceEntryHome::OnResultRemoved (DeeModel *model, DeeModelIter *iter, PlaceEntryHome *self)
{
  g_debug ("Result removed");
}

/* Overrides */
const gchar *
PlaceEntryHome::GetId ()
{
  return "PlaceEntryHome";
}

const gchar *
PlaceEntryHome::GetName ()
{
  return _("Global Search");
}

const gchar *
PlaceEntryHome::GetIcon ()
{
  return "folder-home";
}

const gchar *
PlaceEntryHome::GetDescription ()
{
  return _("Search across all places");
}

guint32
PlaceEntryHome::GetPosition  ()
{
  return 0;
}

const gchar **
PlaceEntryHome::GetMimetypes ()
{
  return NULL;
}

const std::map<gchar *, gchar *>&
PlaceEntryHome::GetHints ()
{
  return _hints;
}

bool
PlaceEntryHome::IsSensitive ()
{
  return true;
}

bool
PlaceEntryHome::IsActive ()
{
  return true;
}

bool
PlaceEntryHome::ShowInLauncher ()
{
  return false;
}

bool
PlaceEntryHome::ShowInGlobal ()
{
  return false;
}

void
PlaceEntryHome::SetActive (bool is_active)
{
}

void
PlaceEntryHome::SetSearch (const gchar *search, std::map<gchar*, gchar*>& hints)
{
  std::vector<PlaceEntry *>::iterator it, eit = _entries.end ();

  for (it = _entries.begin (); it != eit; ++it)
  {
    (*it)->SetGlobalSearch (search, hints);
  }
}

void
PlaceEntryHome::SetActiveSection (guint32 section_id)
{
}

void
PlaceEntryHome::SetGlobalSearch (const gchar *search, std::map<gchar*, gchar*>& hints)
{
}

DeeModel *
PlaceEntryHome::GetSectionsModel ()
{
  return _sections_model;
}

DeeModel *
PlaceEntryHome::GetGroupsModel ()
{
  return _groups_model;
}

DeeModel *
PlaceEntryHome::GetResultsModel ()
{
  return _results_model;
}
