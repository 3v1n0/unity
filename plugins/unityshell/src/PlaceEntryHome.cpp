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
#include <algorithm>

class PlaceEntryGroupHome : public PlaceEntryGroup
{
public:
  PlaceEntryGroupHome(PlaceEntry* entry)
    : _entry(entry)
  {
  }

  PlaceEntryGroupHome(const PlaceEntryGroupHome& b)
  {
    _entry = b._entry;
  }

  const void* GetId() const
  {
    return _entry;
  }

  const char* GetRenderer() const
  {
    return NULL;
  }

  const char* GetName() const
  {
    return _entry->GetName();
  }

  const char* GetIcon() const
  {
    return _entry->GetIcon();
  }

private:
  PlaceEntry* _entry;
};


PlaceEntryHome::PlaceEntryHome(PlaceFactory* factory)
  : _factory(factory),
    _n_searches_done(0)
{
  LoadExistingEntries();
  _factory->place_added.connect(sigc::mem_fun(this, &PlaceEntryHome::OnPlaceAdded));
}

PlaceEntryHome::~PlaceEntryHome()
{
}

void
PlaceEntryHome::LoadExistingEntries()
{
  std::vector<Place*> places = _factory->GetPlaces();
  std::vector<Place*>::iterator it;

  for (it = places.begin(); it != places.end(); ++it)
  {
    Place* place = static_cast<Place*>(*it);
    OnPlaceAdded(place);
  }
}

void
PlaceEntryHome::OnPlaceAdded(Place* place)
{
  std::vector<PlaceEntry*> entries = place->GetEntries();
  std::vector<PlaceEntry*>::iterator i;

  for (i = entries.begin(); i != entries.end(); ++i)
  {
    PlaceEntry* entry = static_cast<PlaceEntry*>(*i);
    OnPlaceEntryAdded(entry);
  }

  place->entry_removed.connect(sigc::mem_fun(this, &PlaceEntryHome::OnPlaceEntryRemoved));
}

void
PlaceEntryHome::OnPlaceEntryRemoved(PlaceEntry* entry)
{
  std::vector<PlaceEntry*>::iterator it;

  it = std::find(_entries.begin(), _entries.end(), entry);
  if (it != _entries.end())
  {
    _entries.erase(it);
  }
}

void
PlaceEntryHome::OnPlaceEntryAdded(PlaceEntry* entry)
{
  PlaceEntryGroupHome group(entry);

  if (!entry->ShowInGlobal())
    return;

  _entries.push_back(entry);

  entry->global_result_added.connect(sigc::mem_fun(this, &PlaceEntryHome::OnResultAdded));
  entry->global_result_removed.connect(sigc::mem_fun(this, &PlaceEntryHome::OnResultRemoved));
  entry->search_finished.connect(sigc::mem_fun(this, &PlaceEntryHome::OnSearchFinished));

  group_added.emit(this, group);
}

void
PlaceEntryHome::RefreshEntry(PlaceEntry* entry)
{
  OnPlaceEntryAdded(entry);
}

void
PlaceEntryHome::OnResultAdded(PlaceEntry* entry, PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlaceEntryGroupHome our_group(entry);

  _id_to_entry[result.GetId()] = entry;

  result_added.emit(this, our_group, result);
}


void
PlaceEntryHome::OnResultRemoved(PlaceEntry* entry, PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlaceEntryGroupHome our_group(entry);

  _id_to_entry.erase(result.GetId());

  result_removed(this, our_group, result);
}

/* Overrides */
const gchar*
PlaceEntryHome::GetId()
{
  return "PlaceEntryHome";
}

const gchar*
PlaceEntryHome::GetName()
{
  return "";
}

const gchar*
PlaceEntryHome::GetSearchHint()
{
  return _("Search");
}

const gchar*
PlaceEntryHome::GetIcon()
{
  return "folder-home";
}

const gchar*
PlaceEntryHome::GetDescription()
{
  return _("Search across all places");
}

guint64
PlaceEntryHome::GetShortcut()
{
  return 0;
}

guint32
PlaceEntryHome::GetPosition()
{
  return 0;
}

const gchar**
PlaceEntryHome::GetMimetypes()
{
  return NULL;
}

const std::map<gchar*, gchar*>&
PlaceEntryHome::GetHints()
{
  return _hints;
}

bool
PlaceEntryHome::IsSensitive()
{
  return true;
}

bool
PlaceEntryHome::IsActive()
{
  return true;
}

bool
PlaceEntryHome::ShowInLauncher()
{
  return false;
}

bool
PlaceEntryHome::ShowInGlobal()
{
  return false;
}

void
PlaceEntryHome::SetActive(bool is_active)
{
}

void
PlaceEntryHome::SetSearch(const gchar* search, std::map<gchar*, gchar*>& hints)
{
  std::vector<PlaceEntry*>::iterator it, eit = _entries.end();

  _n_searches_done = 0;
  _last_search = search;

  for (it = _entries.begin(); it != eit; ++it)
  {
    (*it)->SetGlobalSearch(search, hints);
  }
}

void
PlaceEntryHome::SetActiveSection(guint32 section_id)
{
}

void
PlaceEntryHome::SetGlobalSearch(const gchar* search, std::map<gchar*, gchar*>& hints)
{
}

void
PlaceEntryHome::ForeachGroup(GroupForeachCallback slot)
{
  std::vector<PlaceEntry*>::iterator it, eit = _entries.end();

  for (it = _entries.begin(); it != eit; ++it)
  {
    PlaceEntryGroupHome group(*it);
    slot(this, group);
  }
}

void
PlaceEntryHome::ForeachResult(ResultForeachCallback slot)
{
  std::vector<PlaceEntry*>::iterator it, eit = _entries.end();

  _foreach_callback = slot;

  for (it = _entries.begin(); it != eit; ++it)
  {
    (*it)->ForeachGlobalResult(sigc::mem_fun(this, &PlaceEntryHome::OnForeachResult));
  }
}

void
PlaceEntryHome::OnForeachResult(PlaceEntry* entry, PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlaceEntryGroupHome our_group(entry);

  _foreach_callback(this, our_group, result);
}

void
PlaceEntryHome::GetResult(const void* id, ResultForeachCallback slot)
{
  PlaceEntry* entry = _id_to_entry[id];

  _foreach_callback = slot;

  if (entry)
  {
    entry->GetGlobalResult(id, sigc::mem_fun(this, &PlaceEntryHome::OnForeachResult));
  }
}

void
PlaceEntryHome::ActivateResult(const void* id)
{
  PlaceEntry* entry = _id_to_entry[id];

  if (entry)
  {
    entry->ActivateGlobalResult(id);
  }
}

void
PlaceEntryHome::OnSearchFinished(const char*                           search_string,
                                 guint32                               section_id,
                                 std::map<const char*, const char*>& hints)
{
  if (_last_search == search_string)
  {
    _n_searches_done++;
    if (_n_searches_done == _entries.size())
    {
      search_finished.emit(search_string, section_id, hints);
    }
  }
}

