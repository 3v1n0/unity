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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 */
#include "Indicators.h"

#include <map>
#include <vector>

#include <iostream>
using std::cout;
using std::endl;

namespace unity {
namespace indicator {

class Indicators::Impl
{
public:
  typedef std::map<std::string, Indicator::Ptr> IndicatorMap;
  typedef std::map<std::string, Entry::Ptr> EntryMap;

  Impl(Indicators* owner)
    : owner_(owner)
    {}

  void ActivateEntry(std::string const& entry_id);
  void SetEntryShowNow(std::string const& entry_id, bool show_now);

  Indicator& GetIndicator(std::string const& name);

  void OnEntryAdded(Entry::Ptr const& entry);

private:
  Indicators* owner_;
  IndicatorMap indicators_;
  EntryMap entries_;
  Entry::Ptr active_entry_;
};


Indicators::Indicators()
  : pimpl(new Impl(this))
{
}

Indicators::~Indicators()
{
  delete pimpl;
}

void Indicators::ActivateEntry(std::string const& entry_id)
{
  pimpl->ActivateEntry(entry_id);
  on_entry_activated.emit(entry_id);
}

void Indicators::SetEntryShowNow(std::string const& entry_id, bool show_now)
{
  pimpl->SetEntryShowNow(entry_id, show_now);
}

Indicator& Indicators::GetIndicator(std::string const& name)
{
  return pimpl->GetIndicator(name);
}

void Indicators::Impl::ActivateEntry(std::string const& entry_id)
{
  if (active_entry_)
    active_entry_->set_active(false);
  EntryMap::iterator i = entries_.find(entry_id);
  if (i != entries_.end())
  {
    active_entry_ = i->second;
    active_entry_->set_active(true);
  }
}

void Indicators::Impl::SetEntryShowNow(std::string const& entry_id, bool show_now)
{
  EntryMap::iterator i = entries_.find(entry_id);
  if (i != entries_.end())
  {
    i->second->set_show_now(show_now);
  }
}

Indicator& Indicators::Impl::GetIndicator(std::string const& name)
{
  IndicatorMap::iterator i = indicators_.find(name);
  if (i != indicators_.end())
    return *(i->second);

  // Make a new one
  Indicator::Ptr indicator(new Indicator(name));
  // The implementation class is interested in new entries.
  indicator->on_entry_added.connect(sigc::mem_fun(this, &Indicators::Impl::OnEntryAdded));
  // The owner Indicators class is interested in the other events.
  indicator->on_show_menu.connect(sigc::mem_fun(owner_, &Indicators::OnEntryShowMenu));
  indicator->on_scroll.connect(sigc::mem_fun(owner_, &Indicators::OnEntryScroll));
  indicators_[name] = indicator;
  owner_->on_object_added.emit(indicator);
  return *indicator;
}

void Indicators::Impl::OnEntryAdded(Entry::Ptr const& entry)
{
  cout << "Indicators::Impl::OnEntryAdded: " << entry->id() << endl;
  entries_[entry->id()] = entry;
}


} // namespace indicator
} // namespace unity
