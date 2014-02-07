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

#include <Nux/Nux.h>
#include "PanelIndicatorEntryDropdownView.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/PanelStyle.h"

namespace unity
{
namespace panel
{
using namespace indicator;

namespace
{
const std::string ICON_NAME = "go-down-symbolic";
}

PanelIndicatorEntryDropdownView::PanelIndicatorEntryDropdownView(std::string const& indicator, Indicators::Ptr const& indicators)
  : PanelIndicatorEntryView(std::make_shared<Entry>(indicator+"-dropdown"), 5, IndicatorEntryType::DROP_DOWN)
  , indicators_(indicators)
{
  proxy_->set_priority(std::numeric_limits<int>::max());

  // This is a workaround we need to compute the dropdown size on construction
  SetProxyVisibility(true);
  SetProxyVisibility(false);
}

void PanelIndicatorEntryDropdownView::SetProxyVisibility(bool visible)
{
  if (proxy_->visible() == visible)
    return;

  proxy_->set_image(GTK_IMAGE_ICON_NAME, ICON_NAME, visible, visible);
}

void PanelIndicatorEntryDropdownView::Push(PanelIndicatorEntryView::Ptr const& child)
{
  if (!child)
    return;

  if (std::find(children_.begin(), children_.end(), child) != children_.end())
    return;

  children_.push_back(child);
  child->GetEntry()->add_parent(proxy_);
  child->in_dropdown = true;
  debug::Introspectable::AddChild(child.GetPointer());
  SetProxyVisibility(true);
}

void PanelIndicatorEntryDropdownView::Insert(PanelIndicatorEntryView::Ptr const& child)
{
  if (!child)
    return;

  if (std::find(children_.begin(), children_.end(), child) != children_.end())
    return;

  auto it = children_.begin();
  for (; it != children_.end(); ++it)
  {
    if (child->GetEntryPriority() <= (*it)->GetEntryPriority())
      break;
  }

  children_.insert(it, child);
  child->GetEntry()->add_parent(proxy_);
  child->in_dropdown = true;
  debug::Introspectable::AddChild(child.GetPointer());
  SetProxyVisibility(true);
}

PanelIndicatorEntryView::Ptr PanelIndicatorEntryDropdownView::Pop()
{
  if (children_.empty())
    return PanelIndicatorEntryView::Ptr();

  auto child = children_.front();
  Remove(child);

  return child;
}

std::deque<PanelIndicatorEntryView::Ptr> const& PanelIndicatorEntryDropdownView::Children() const
{
  return children_;
}

PanelIndicatorEntryView::Ptr PanelIndicatorEntryDropdownView::Top() const
{
  return (!children_.empty()) ? children_.front() : PanelIndicatorEntryView::Ptr();
}

size_t PanelIndicatorEntryDropdownView::Size() const
{
  return children_.size();
}

bool PanelIndicatorEntryDropdownView::Empty() const
{
  return children_.empty();
}

void PanelIndicatorEntryDropdownView::Remove(PanelIndicatorEntryView::Ptr const& child)
{
  auto it = std::find(children_.begin(), children_.end(), child);

  if (it != children_.end())
  {
    debug::Introspectable::RemoveChild(it->GetPointer());
    child->GetEntry()->rm_parent(proxy_);
    child->in_dropdown = false;
    children_.erase(it);
  }

  if (children_.empty())
    SetProxyVisibility(false);
}

void PanelIndicatorEntryDropdownView::ShowMenu(int button)
{
  WindowManager& wm = WindowManager::Default();

  if (!children_.empty() && !wm.IsExpoActive() && !wm.IsScaleActive())
  {
    Indicator::Entries entries;

    for (auto const& entry : children_)
      entries.push_back(entry->GetEntry());

    auto const& geo = GetAbsoluteGeometry();
    indicators_->ShowEntriesDropdown(entries, active_entry_, 0, geo.x, geo.y + Style::Instance().panel_height);
  }
}

bool PanelIndicatorEntryDropdownView::ActivateChild(PanelIndicatorEntryView::Ptr const& child)
{
  for (auto const& entry : children_)
  {
    if (entry == child)
    {
      active_entry_ = entry->GetEntry();
      Activate();
      active_entry_ = nullptr;
      return true;
    }
  }

  return false;
}

} // panel namespace
} // unity namespace
