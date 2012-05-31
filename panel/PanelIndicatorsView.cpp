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

#include <UnityCore/Variant.h>
#include <glib.h>

namespace
{
nux::logging::Logger logger("unity.indicators");
}

namespace unity
{
using namespace indicator;

NUX_IMPLEMENT_OBJECT_TYPE(PanelIndicatorsView);

PanelIndicatorsView::PanelIndicatorsView()
: View(NUX_TRACKER_LOCATION)
, layout_(NULL)
, opacity_(1.0f)
{
  LOG_DEBUG(logger) << "Indicators View Added: ";
  layout_ = new nux::HLayout("", NUX_TRACKER_LOCATION);
  layout_->SetContentDistribution(nux::eStackRight);

  SetLayout(layout_);
}

PanelIndicatorsView::~PanelIndicatorsView()
{
  for (auto ind : indicators_connections_)
  {
    for (auto conn : ind.second)
      conn.disconnect();
  }
}

void
PanelIndicatorsView::AddIndicator(Indicator::Ptr const& indicator)
{
  LOG_DEBUG(logger) << "IndicatorAdded: " << indicator->name();
  indicators_.push_back(indicator);

  std::vector<sigc::connection> connections;

  auto entry_added_conn = indicator->on_entry_added.connect(sigc::mem_fun(this, &PanelIndicatorsView::OnEntryAdded));
  connections.push_back(entry_added_conn);

  auto entry_removed_conn = indicator->on_entry_removed.connect(sigc::mem_fun(this, &PanelIndicatorsView::OnEntryRemoved));
  connections.push_back(entry_removed_conn);

  indicators_connections_[indicator] = connections;
}

void
PanelIndicatorsView::RemoveIndicator(Indicator::Ptr const& indicator)
{
  auto connections = indicators_connections_.find(indicator);

  if (connections != indicators_connections_.end()) {
    for (auto conn : connections->second)
      conn.disconnect();

    indicators_connections_.erase(indicator);
  }

  for (auto entry : indicator->GetEntries())
    OnEntryRemoved(entry->id());

  for (auto i = indicators_.begin(); i != indicators_.end(); i++)
  {
    if (*i == indicator)
    {
      indicators_.erase(i);
      break;
    }
  }

  LOG_DEBUG(logger) << "IndicatorRemoved: " << indicator->name();
}

PanelIndicatorsView::Indicators
PanelIndicatorsView::GetIndicators()
{
  return indicators_;
}

void
PanelIndicatorsView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(GfxContext, geo);
  GfxContext.PopClippingRectangle();
}

void
PanelIndicatorsView::QueueDraw()
{
  nux::View::QueueDraw();

  for (auto entry : entries_)
    entry.second->QueueDraw();
}

void
PanelIndicatorsView::SetMaximumEntriesWidth(int max_width)
{
  unsigned int n_entries = 0;

  for (auto entry : entries_)
    if (entry.second->IsVisible())
      n_entries++;

  if (n_entries > 0)
  {
    for (auto entry : entries_)
    {
      if (entry.second->IsVisible() && n_entries > 0)
      {
        int max_entry_width = max_width / n_entries;

        if (entry.second->GetBaseWidth() > max_entry_width)
          entry.second->SetMaximumWidth(max_entry_width);

        max_width -= entry.second->GetBaseWidth();
        --n_entries;
      }
    }
  }
}

PanelIndicatorEntryView*
PanelIndicatorsView::ActivateEntry(std::string const& entry_id, int button)
{
  auto entry = entries_.find(entry_id);

  if (entry != entries_.end())
  {
    PanelIndicatorEntryView* view = entry->second;

    if (view->IsSensitive() && view->IsVisible())
      view->Activate(button);

    return view;
  }

  return nullptr;
}

bool
PanelIndicatorsView::ActivateIfSensitive()
{
  std::map<int, PanelIndicatorEntryView*> sorted_entries;
  
  for (auto entry : entries_)
    sorted_entries[entry.second->GetEntryPriority()] = entry.second;
  
  for (auto entry : sorted_entries)
  {
    PanelIndicatorEntryView* view = entry.second;

    if (view->IsSensitive() && view->IsVisible() && view->IsFocused())
    {
      /* Use the 0 button, it means it's a keyboard activation */
      view->Activate(0);
      return true;
    }
  }

  return false;
}

void
PanelIndicatorsView::GetGeometryForSync(EntryLocationMap& locations)
{
  for (auto entry : entries_)
    entry.second->GetGeometryForSync(locations);
}

PanelIndicatorEntryView*
PanelIndicatorsView::ActivateEntryAt(int x, int y, int button)
{
  PanelIndicatorEntryView* target = nullptr;
  bool found_old_active = false;

  //
  // Change the entry active status without waiting
  // for slow inter-process communication with unity-panel-service,
  // which causes visible lag in many cases.
  //

  for (auto entry : entries_)
  {
    PanelIndicatorEntryView* view = entry.second;

    if (!target && view->IsVisible() && view->IsFocused() &&
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
    for (auto entry : entries_)
    {
      PanelIndicatorEntryView* view = entry.second;

      if (view != target && view->IsActive())
      {
        view->Unactivate();
        break;
      }
    }
  }

  return target;
}

void
PanelIndicatorsView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());
  layout_->ProcessDraw(GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}

void
PanelIndicatorsView::AddEntryView(PanelIndicatorEntryView* view,
                                  IndicatorEntryPosition pos)
{
  if (!view)
    return;

  int entry_pos = pos;

  view->SetOpacity(opacity_);
  view->refreshed.connect(sigc::mem_fun(this, &PanelIndicatorsView::OnEntryRefreshed));

  if (entry_pos == IndicatorEntryPosition::AUTO)
  {
    entry_pos = nux::NUX_LAYOUT_BEGIN;

    if (view->GetEntryPriority() > -1)
    {
      for (auto area : layout_->GetChildren())
      {
        auto en = dynamic_cast<PanelIndicatorEntryView*>(area);

        if (en)
        {
          if (en && view->GetEntryPriority() <= en->GetEntryPriority())
            break;

          entry_pos++;
        }
      }
    }
  }

  layout_->AddView(view, 0, nux::eCenter, nux::eFull, 1.0, (nux::LayoutPosition) entry_pos);

  entries_[view->GetEntryID()] = view;

  AddChild(view);
  QueueRelayout();
  QueueDraw();

  on_indicator_updated.emit(view);
}

PanelIndicatorEntryView *
PanelIndicatorsView::AddEntry(Entry::Ptr const& entry, int padding,
                              IndicatorEntryPosition pos, IndicatorEntryType type)
{
  auto view = new PanelIndicatorEntryView(entry, padding, type);
  AddEntryView(view, pos);

  return view;
}

void
PanelIndicatorsView::OnEntryAdded(Entry::Ptr const& entry)
{
  AddEntry(entry);
}

void
PanelIndicatorsView::OnEntryRefreshed(PanelIndicatorEntryView* view)
{
  QueueRelayout();
  QueueDraw();

  on_indicator_updated.emit(view);
}

void
PanelIndicatorsView::RemoveEntryView(PanelIndicatorEntryView* view)
{
  if (!view)
    return;

  std::string const& entry_id = view->GetEntryID();
  RemoveChild(view);
  on_indicator_updated.emit(view);
  entries_.erase(entry_id);
  layout_->RemoveChildObject(view);

  QueueRelayout();
  QueueDraw();
}

void
PanelIndicatorsView::RemoveEntry(std::string const& entry_id)
{
  RemoveEntryView(entries_[entry_id]);
}

void
PanelIndicatorsView::OnEntryRemoved(std::string const& entry_id)
{
  RemoveEntry(entry_id);
}

void
PanelIndicatorsView::OverlayShown()
{
  for (auto entry: entries_)
    entry.second->OverlayShown();
}

void
PanelIndicatorsView::OverlayHidden()
{
  for (auto entry: entries_)
    entry.second->OverlayHidden();
}

double
PanelIndicatorsView::GetOpacity()
{
  return opacity_;
}

void
PanelIndicatorsView::SetOpacity(double opacity)
{
  opacity = CLAMP(opacity, 0.0f, 1.0f);

  for (auto entry: entries_)
    entry.second->SetOpacity(opacity);

  if (opacity_ != opacity)
  {
    opacity_ = opacity;
    NeedRedraw();
  }
}

std::string PanelIndicatorsView::GetName() const
{
  return "Indicators";
}

void
PanelIndicatorsView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
  .add(GetAbsoluteGeometry())
  .add("entries", entries_.size())
  .add("opacity", opacity_);
}

} // namespace unity
