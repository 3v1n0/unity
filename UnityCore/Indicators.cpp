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
#include "AppmenuIndicator.h"

namespace unity
{
namespace indicator
{

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

  IndicatorsList GetIndicators() const;

  Indicator::Ptr GetIndicator(std::string const& name);
  Indicator::Ptr AddIndicator(std::string const& name);
  void RemoveIndicator(std::string const& name);

  void OnEntryAdded(Entry::Ptr const& entry);

  Entry::Ptr GetEntry(std::string const& entry_id);

private:
  Indicators* owner_;
  IndicatorMap indicators_;
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

Indicators::IndicatorsList Indicators::GetIndicators() const
{
  return pimpl->GetIndicators();
}

Indicator::Ptr Indicators::AddIndicator(std::string const& name)
{
  return pimpl->AddIndicator(name);
}

Indicator::Ptr Indicators::GetIndicator(std::string const& name)
{
  return pimpl->GetIndicator(name);
}

void Indicators::RemoveIndicator(std::string const& name)
{
  return pimpl->RemoveIndicator(name);
}

void Indicators::Impl::ActivateEntry(std::string const& entry_id)
{
  if (active_entry_)
    active_entry_->set_active(false);
  active_entry_ = GetEntry(entry_id);
  if (active_entry_)
  {
    active_entry_->set_active(true);
  }
}

void Indicators::Impl::SetEntryShowNow(std::string const& entry_id,
                                       bool show_now)
{
  Entry::Ptr entry = GetEntry(entry_id);
  if (entry)
  {
    entry->set_show_now(show_now);
  }
}

Indicators::IndicatorsList Indicators::Impl::GetIndicators() const
{
  Indicators::IndicatorsList list;

  for (auto it = indicators_.begin(); it != indicators_.end(); it++)
  {
    list.push_back(it->second);
  }

  return list;
}

Indicator::Ptr Indicators::Impl::AddIndicator(std::string const& name)
{
  Indicator* indptr;

  if (name == "libappmenu.so")
    indptr = new AppmenuIndicator(name);
  else
    indptr = new Indicator(name);

  Indicator::Ptr indicator(indptr);

  // The owner Indicators class is interested in the other events.
  indicator->on_show_menu.connect(sigc::mem_fun(owner_, &Indicators::OnEntryShowMenu));
  indicator->on_secondary_activate.connect(sigc::mem_fun(owner_, &Indicators::OnEntrySecondaryActivate));
  indicator->on_scroll.connect(sigc::mem_fun(owner_, &Indicators::OnEntryScroll));

  if (indicator->IsAppmenu())
  {
    AppmenuIndicator *appmenu = dynamic_cast<AppmenuIndicator*>(indicator.get());

    if (appmenu)
      appmenu->on_show_appmenu.connect(sigc::mem_fun(owner_, &Indicators::OnShowAppMenu));
  }

  indicators_[name] = indicator;
  owner_->on_object_added.emit(indicator);

  return indicator;
}

Indicator::Ptr Indicators::Impl::GetIndicator(std::string const& name)
{
  IndicatorMap::iterator i = indicators_.find(name);
  if (i != indicators_.end())
    return i->second;

  return Indicator::Ptr();
}

void Indicators::Impl::RemoveIndicator(std::string const& name)
{
  auto indicator = GetIndicator(name);

  if (indicator)
  {
    owner_->on_object_removed.emit(indicator);
    indicators_.erase(name);
  }
}

Entry::Ptr Indicators::Impl::GetEntry(std::string const& entry_id)
{
  // Since we reuse Entry objects but change the value that they are keyed on,
  // we need to traverse through the Indicators asking them if they have this
  // entry_id.
  Entry::Ptr result;
  for (IndicatorMap::iterator i = indicators_.begin(), end = indicators_.end();
       i != end && !result; ++i)
  {
    result = i->second->GetEntry(entry_id);
  }
  return result;
}


} // namespace indicator
} // namespace unity
