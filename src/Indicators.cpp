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

#include <vector>

namespace unity {
namespace indicator {

class IndicatorsImpl
{
public:
  typedef std::vector<Indicator::Ptr> Collection;
  typedef std::map<std::string, Entry::Ptr> EntryMap;

  IndicatorsImpl(Indicators* owner)
    : owner_(owner)
    {}

  void ActivateEntry(std::string const& entry_id);
  void SetEntryShowNow(std::string const& entry_id, bool show_now);

private:
  Indicators* owner_;
  Collection indicators_;
  EntryMap entries_;
  Entry::Ptr active_entry_;
};


Indicators::Indicators()
  : pimpl_(new IndicatorsImpl(this))
{

}

void Indicators::ActivateEntry(std::string const& entry_id)
{
  pimpl_->ActivateEntry(entry_id);
  on_entry_activated.emit(entry_id);
}

void Indicators::SetEntryShowNow(std::string const& entry_id, bool show_now)
{
  pimpl->SetEntryShowNow(std::string const& entry_id, bool show_now);
}


  std::vector<IndicatorObjectProxy*>::iterator it;
  
  for (it = _indicators.begin(); it != _indicators.end(); ++it)
  {
    IndicatorObjectProxyRemote *object = static_cast<IndicatorObjectProxyRemote *> (*it);
    std::vector<IndicatorObjectEntryProxy*>::iterator it2;
  
    for (it2 = object->GetEntries ().begin(); it2 != object->GetEntries ().end(); ++it2)
    {
      IndicatorObjectEntryProxyRemote *entry = static_cast<IndicatorObjectEntryProxyRemote *> (*it2);

      if (g_strcmp0 (entry_id, entry->GetId ()) == 0)
      {
        entry->OnShowNowChanged (show_now_state);
        return;
      }
    }
  }
}


  // For adding factory-specific properties
  virtual void AddProperties(GVariantBuilder *builder) = 0;

  // Signals
  sigc::signal<void, Indicator::Ptr const&> on_object_added;
  sigc::signal<void, Indicator::Ptr const&> on_object_removed;
  sigc::signal<void, int, int> on_menu_pointer_moved;
  sigc::signal<void, std::string const&> on_entry_activate_request;
  sigc::signal<void, std::string const&> on_entry_activated;
  sigc::signal<void> on_synced;

private:
  boost::scoped_ptr<IndicatorsImpl> pimpl_;
};


void IndicatorsImpl::ActivateEntry(std::string const& entry_id)
{
  if (active_entry_)
    active_entry_->SetActive(false);
  EntryMap::iterator i = entries_.find(entry_id);
  if (i != entries_.end())
  {
    active_entry_ = i->second;
    active_entry_->set_active(true);
  }
}

void IndicatorsImpl::SetEntryShowNow(std::string const& entry_id, bool show_now)
{
  EntryMap::iterator i = entries_.find(entry_id);
  if (i != entries_.end())
  {
    i->second->set_show_now(show_now);
  }
}

} // namespace indicator
} // namespace unity
