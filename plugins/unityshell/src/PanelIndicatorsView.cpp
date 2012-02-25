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
 * Authored by: Marco Trevisan (Treviño) <mail@3v1n0.net>
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
NUX_IMPLEMENT_OBJECT_TYPE(PanelIndicatorsView);

PanelIndicatorsView::PanelIndicatorsView()
: View(NUX_TRACKER_LOCATION)
, layout_(NULL)
, opacity_(1.0f)
{
  LOG_DEBUG(logger) << "Indicators View Added: ";
  layout_ = new nux::HLayout("", NUX_TRACKER_LOCATION);

  SetCompositionLayout(layout_);
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
PanelIndicatorsView::AddIndicator(indicator::Indicator::Ptr const& indicator)
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
PanelIndicatorsView::RemoveIndicator(indicator::Indicator::Ptr const& indicator)
{
  auto connections = indicators_connections_.find(indicator);

  if (connections != indicators_connections_.end()) {
    for (auto conn : connections->second)
      conn.disconnect();

    indicators_connections_.erase(indicator);
  }

  for (auto entry : indicator->GetEntries())
    OnEntryRemoved (entry->id());

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

PanelIndicatorEntryView*
PanelIndicatorsView::ActivateEntry(std::string const& entry_id)
{
  auto entry = entries_.find(entry_id);

  if (entry != entries_.end() && entry->second->IsEntryValid())
  {
    PanelIndicatorEntryView* view = entry->second;
    LOG_DEBUG(logger) << "Activating: " << entry_id;
    view->Activate();
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
    if (view->IsSensitive())
    {
      view->Activate();
      return true;
    }
  }
  return false;
}

void
PanelIndicatorsView::GetGeometryForSync(indicator::EntryLocationMap& locations)
{
  for (auto entry : entries_)
    entry.second->GetGeometryForSync(locations);
}

PanelIndicatorEntryView*
PanelIndicatorsView::ActivateEntryAt(int x, int y)
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

    if (!target && view->GetAbsoluteGeometry().IsPointInside(x, y))
    {
      view->Activate(0);
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

PanelIndicatorEntryView *
PanelIndicatorsView::AddEntry(indicator::Entry::Ptr const& entry, int padding,
                              IndicatorEntryPosition pos, IndicatorEntryType type)
{
  auto view = new PanelIndicatorEntryView(entry, padding, type);
  int entry_pos = pos;

  view->SetOpacity(opacity_);
  view->refreshed.connect(sigc::mem_fun(this, &PanelIndicatorsView::OnEntryRefreshed));

  if (entry_pos == IndicatorEntryPosition::AUTO)
  {
    entry_pos = nux::NUX_LAYOUT_BEGIN;

    if (entry->priority() > -1)
    {
      for (auto area : layout_->GetChildren())
      {
        auto en = dynamic_cast<PanelIndicatorEntryView*>(area);

        if (en)
        {
          if (en && entry->priority() <= en->GetEntryPriority())
            break;

          entry_pos++;
        }
      }
    }
  }

  layout_->AddView(view, 0, nux::eCenter, nux::eFull, 1.0, (nux::LayoutPosition) entry_pos);
  layout_->SetContentDistribution(nux::eStackRight);
  entries_[entry->id()] = view;

  AddChild(view);
  QueueRelayout();
  QueueDraw();

  on_indicator_updated.emit(view);

  return view;
}

void
PanelIndicatorsView::OnEntryAdded(indicator::Entry::Ptr const& entry)
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
PanelIndicatorsView::RemoveEntry(std::string const& entry_id)
{
  PanelIndicatorEntryView* view = entries_[entry_id];

  if (view)
  {
    layout_->RemoveChildObject(view);
    entries_.erase(entry_id);
    on_indicator_updated.emit(view);

    QueueRelayout();
    QueueDraw();
  }
}

void
PanelIndicatorsView::OnEntryRemoved(std::string const& entry_id)
{
  RemoveEntry(entry_id);
}

void
PanelIndicatorsView::DashShown()
{
  for (auto entry: entries_)
    entry.second->DashShown();
}

void
PanelIndicatorsView::DashHidden()
{
  for (auto entry: entries_)
    entry.second->DashHidden();
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
  return "IndicatorsView";
}

void
PanelIndicatorsView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder).add(GetGeometry());
}

} // namespace unity
