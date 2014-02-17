// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include <Nux/Nux.h>
#include <Nux/Area.h>
#include <Nux/HLayout.h>

#include <NuxCore/Logger.h>

#include "PanelIndicatorsView.h"
#include "PanelIndicatorEntryDropdownView.h"

DECLARE_LOGGER(logger, "unity.indicators");

namespace unity
{
namespace panel
{
using namespace indicator;

NUX_IMPLEMENT_OBJECT_TYPE(PanelIndicatorsView);

PanelIndicatorsView::PanelIndicatorsView()
: View(NUX_TRACKER_LOCATION)
, opacity(1.0f, sigc::mem_fun(this, &PanelIndicatorsView::SetOpacity))
, layout_(new nux::HLayout("", NUX_TRACKER_LOCATION))
{
  opacity.DisableNotifications();
  layout_->SetContentDistribution(nux::MAJOR_POSITION_END);
  SetLayout(layout_);

  LOG_DEBUG(logger) << "Indicators View Added: ";
}

void PanelIndicatorsView::EnableDropdownMenu(bool enable, indicator::Indicators::Ptr const& indicators)
{
  if (enable && indicators)
  {
    dropdown_ = new PanelIndicatorEntryDropdownView(GetName(), indicators);
    AddEntryView(dropdown_.GetPointer());
  }
  else
  {
    RemoveEntryView(dropdown_.GetPointer());
    dropdown_.Release();
  }
}

void PanelIndicatorsView::AddIndicator(Indicator::Ptr const& indicator)
{
  LOG_DEBUG(logger) << "IndicatorAdded: " << indicator->name();
  indicators_.push_back(indicator);

  for (auto const& entry : indicator->GetEntries())
    AddEntry(entry);

  auto& conn_manager = indicators_connections_[indicator];
  conn_manager.Add(indicator->on_entry_added.connect(sigc::mem_fun(this, &PanelIndicatorsView::OnEntryAdded)));
  conn_manager.Add(indicator->on_entry_removed.connect(sigc::mem_fun(this, &PanelIndicatorsView::RemoveEntry)));
}

void PanelIndicatorsView::RemoveIndicator(Indicator::Ptr const& indicator)
{
  indicators_connections_.erase(indicator);

  for (auto const& entry : indicator->GetEntries())
    RemoveEntry(entry->id());

  for (auto i = indicators_.begin(); i != indicators_.end(); ++i)
  {
    if (*i == indicator)
    {
      indicators_.erase(i);
      break;
    }
  }

  LOG_DEBUG(logger) << "IndicatorRemoved: " << indicator->name();
}

PanelIndicatorsView::Indicators const& PanelIndicatorsView::GetIndicators() const
{
  return indicators_;
}

void PanelIndicatorsView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(GfxContext, geo);
  GfxContext.PopClippingRectangle();
}

void PanelIndicatorsView::SetMaximumEntriesWidth(int max_width)
{
  if (!dropdown_)
    return;

  int accumolated_width = dropdown_->GetBaseWidth();
  std::vector<PanelIndicatorEntryView::Ptr> to_hide;

  for (auto* area : layout_->GetChildren())
  {
    auto en = static_cast<PanelIndicatorEntryView*>(area);
    if (en == dropdown_.GetPointer())
      continue;

    accumolated_width += en->GetBaseWidth();

    if (accumolated_width > max_width)
      to_hide.push_back(PanelIndicatorEntryView::Ptr(en));
  }

  // No need to hide an item if there's space that we considered for the dropdown
  if (!dropdown_->IsVisible() && to_hide.size() == 1)
  {
    if (accumolated_width - dropdown_->GetBaseWidth() < max_width)
      to_hide.clear();
  }

  // There's just one hidden entries, it might fit in the space we have
  if (to_hide.empty() && dropdown_->Size() == 1)
    accumolated_width -= dropdown_->GetBaseWidth();

  if (accumolated_width < max_width)
  {
    while (!dropdown_->Empty() && dropdown_->Top()->GetBaseWidth() < (max_width - accumolated_width))
      AddEntryView(dropdown_->Pop().GetPointer());
  }
  else
  {
    for (auto const& hidden : to_hide)
    {
      layout_->RemoveChildObject(hidden.GetPointer());
      RemoveChild(hidden.GetPointer());
      dropdown_->Push(hidden);
    }
  }
}

PanelIndicatorEntryView* PanelIndicatorsView::ActivateEntry(std::string const& entry_id, int button)
{
  auto entry = entries_.find(entry_id);

  if (entry != entries_.end())
  {
    PanelIndicatorEntryView* view = entry->second;

    if (view->IsSensitive() && view->IsVisible())
    {
      view->Activate(button);
    }
    else if (dropdown_)
    {
      dropdown_->ActivateChild(PanelIndicatorEntryView::Ptr(view));
    }

    return view;
  }

  return nullptr;
}

bool PanelIndicatorsView::ActivateIfSensitive()
{
  for (auto* area : layout_->GetChildren())
  {
    auto* view = static_cast<PanelIndicatorEntryView*>(area);

    if (view->IsSensitive() && view->IsVisible() && view->IsFocused())
    {
      /* Use the 0 button, it means it's a keyboard activation */
      view->Activate(0);
      return true;
    }
  }

  return false;
}

void PanelIndicatorsView::GetGeometryForSync(EntryLocationMap& locations)
{
  for (auto const& entry : entries_)
    entry.second->GetGeometryForSync(locations);
}

PanelIndicatorEntryView* PanelIndicatorsView::ActivateEntryAt(int x, int y, int button)
{
  PanelIndicatorEntryView* target = nullptr;
  bool found_old_active = false;

  //
  // Change the entry active status without waiting
  // for slow inter-process communication with unity-panel-service,
  // which causes visible lag in many cases.
  //

  for (auto* area : layout_->GetChildren())
  {
    auto view = static_cast<PanelIndicatorEntryView*>(area);

    if (!view->IsVisible())
      continue;

    if (!target && view->IsFocused() &&
        view->IsSensitive() &&
        view->GetAbsoluteGeometry().IsPointInside(x, y))
    {
      view->Activate(button);
      target = view;
      break;
    }
    else if (target && view->IsActive())
    {
      view->Unactivate();
      found_old_active = true;
      break;
    }
  }

  if (target && !found_old_active)
  {
    for (auto* area : layout_->GetChildren())
    {
      auto view = static_cast<PanelIndicatorEntryView*>(area);

      if (view != target && view->IsVisible() && view->IsActive())
      {
        view->Unactivate();
        break;
      }
    }
  }

  return target;
}

void PanelIndicatorsView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());
  layout_->ProcessDraw(GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}

void PanelIndicatorsView::AddEntryView(PanelIndicatorEntryView* view, IndicatorEntryPosition pos)
{
  if (!view)
    return;

  int entry_pos = pos;
  auto const& entry_id = view->GetEntryID();
  view->SetOpacity(opacity());

  if (entry_pos == IndicatorEntryPosition::AUTO)
  {
    entry_pos = nux::NUX_LAYOUT_BEGIN;

    if (view->GetEntryPriority() > -1)
    {
      for (auto area : layout_->GetChildren())
      {
        auto en = static_cast<PanelIndicatorEntryView*>(area);
        if (view->GetEntryPriority() <= en->GetEntryPriority())
          break;

        ++entry_pos;
      }
    }
  }

  layout_->AddView(view, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 1.0, (nux::LayoutPosition) entry_pos);
  AddChild(view);

  QueueRelayout();
  QueueDraw();

  if (entries_.find(entry_id) == entries_.end())
  {
    view->refreshed.connect(sigc::mem_fun(this, &PanelIndicatorsView::OnEntryRefreshed));
    entries_.insert({entry_id, view});
    on_indicator_updated.emit();
    entry_added.emit(view);
  }
}

PanelIndicatorEntryView *PanelIndicatorsView::AddEntry(Entry::Ptr const& entry, int padding, IndicatorEntryPosition pos, IndicatorEntryType type)
{
  auto view = new PanelIndicatorEntryView(entry, padding, type);
  AddEntryView(view, pos);

  return view;
}

void PanelIndicatorsView::OnEntryAdded(Entry::Ptr const& entry)
{
  AddEntry(entry);
}

void PanelIndicatorsView::OnEntryRefreshed(PanelIndicatorEntryView* view)
{
  QueueRelayout();
  QueueDraw();
  on_indicator_updated.emit();
}

void PanelIndicatorsView::RemoveEntryView(PanelIndicatorEntryView* view)
{
  if (!view)
    return;

  entry_removed.emit(view);

  if (dropdown_)
    dropdown_->Remove(PanelIndicatorEntryView::Ptr(view));

  RemoveChild(view);
  entries_.erase(view->GetEntryID());
  layout_->RemoveChildObject(view);
  on_indicator_updated.emit();

  QueueRelayout();
  QueueDraw();
}

void PanelIndicatorsView::RemoveEntry(std::string const& entry_id)
{
  RemoveEntryView(entries_[entry_id]);
}

void PanelIndicatorsView::OverlayShown()
{
  for (auto const& entry: entries_)
    entry.second->OverlayShown();
}

void PanelIndicatorsView::OverlayHidden()
{
  for (auto const& entry: entries_)
    entry.second->OverlayHidden();
}

bool PanelIndicatorsView::SetOpacity(double& target, double const& new_value)
{
  double opacity = CLAMP(new_value, 0.0f, 1.0f);

  for (auto const& entry : entries_)
    entry.second->SetOpacity(opacity);

  if (opacity != target)
  {
    target = opacity;
    QueueDraw();

    return true;
  }

  return false;
}

std::string PanelIndicatorsView::GetName() const
{
  return "Indicators";
}

void PanelIndicatorsView::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
  .add(GetAbsoluteGeometry())
  .add("entries", entries_.size())
  .add("opacity", opacity);
}

} // namespace panel
} // namespace unity
