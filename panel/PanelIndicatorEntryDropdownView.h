// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef PANEL_INDICATOR_ENTRY_EXPANDER_VIEW_H
#define PANEL_INDICATOR_ENTRY_EXPANDER_VIEW_H

#include <deque>
#include <UnityCore/Indicators.h>
#include "PanelIndicatorEntryView.h"

namespace unity
{
namespace panel
{
class PanelIndicatorEntryDropdownView : public PanelIndicatorEntryView
{
public:
  typedef nux::ObjectPtr<PanelIndicatorEntryDropdownView> Ptr;

  PanelIndicatorEntryDropdownView(std::string const& id, indicator::Indicators::Ptr const&);

  void Push(PanelIndicatorEntryView::Ptr const&);
  void Insert(PanelIndicatorEntryView::Ptr const&);
  void Remove(PanelIndicatorEntryView::Ptr const&);
  PanelIndicatorEntryView::Ptr Pop();

  PanelIndicatorEntryView::Ptr Top() const;
  std::deque<PanelIndicatorEntryView::Ptr> const& Children() const;
  size_t Size() const;
  bool Empty() const;

  void ShowMenu(int button = 1) override;
  bool ActivateChild(PanelIndicatorEntryView::Ptr const& child);

protected:
  std::string GetName() const { return "IndicatorEntryDropdownView"; };

private:
  void SetProxyVisibility(bool);

  indicator::Entry::Ptr active_entry_;
  indicator::Indicators::Ptr indicators_;
  std::deque<PanelIndicatorEntryView::Ptr> children_;
};

} // panel namespace
} // unity namespace

#endif // PANEL_INDICATOR_ENTRY_EXPANDER_VIEW_H
