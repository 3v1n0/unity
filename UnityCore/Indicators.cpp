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
  typedef std::unordered_map<std::string, Indicator::Ptr> IndicatorMap;

  Impl(Indicators* owner)
    : owner_(owner)
  {}

  void ActivateEntry(std::string const& panel, std::string const& entry_id, nux::Rect const& geometry);
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
{}

Indicators::~Indicators()
{}

void Indicators::ActivateEntry(std::string const& panel, std::string const& entry_id, nux::Rect const& geometry)
{
  pimpl->ActivateEntry(panel, entry_id, geometry);
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

void Indicators::Impl::ActivateEntry(std::string const& panel, std::string const& entry_id, nux::Rect const& geometry)
{
  if (active_entry_)
  {
    active_entry_->set_geometry(nux::Rect());
    active_entry_->set_active(false);
  }

  active_entry_ = GetEntry(entry_id);

  if (active_entry_)
  {
    active_entry_->set_geometry(geometry);
    active_entry_->set_active(true);
    owner_->on_entry_activated.emit(panel, entry_id, geometry);
  }
  else
  {
    owner_->on_entry_activated.emit(std::string(), std::string(), nux::Rect());
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

  for (auto it = indicators_.begin(); it != indicators_.end(); ++it)
  {
    list.push_back(it->second);
  }

  return list;
}

Indicator::Ptr Indicators::Impl::AddIndicator(std::string const& name)
{
  Indicator::Ptr indicator(GetIndicator(name));

  if (indicator)
    return indicator;

  if (name == "libappmenu.so")
  {
    auto appmenu = std::make_shared<AppmenuIndicator>(name);
    appmenu->on_show_appmenu.connect(sigc::mem_fun(owner_, &Indicators::OnShowAppMenu));
    indicator = appmenu;
  }
  else
  {
    indicator = std::make_shared<Indicator>(name);
  }

  // The owner Indicators class is interested in the other events.
  indicator->on_show_menu.connect(sigc::mem_fun(owner_, &Indicators::OnEntryShowMenu));
  indicator->on_secondary_activate.connect(sigc::mem_fun(owner_, &Indicators::OnEntrySecondaryActivate));
  indicator->on_scroll.connect(sigc::mem_fun(owner_, &Indicators::OnEntryScroll));

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
  for (auto it = indicators_.begin(); it != indicators_.end(); ++it)
  {
    Entry::Ptr entry = it->second->GetEntry(entry_id);

    if (entry)
      return entry;
  }

  return Entry::Ptr();
}


} // namespace indicator
} // namespace unity
