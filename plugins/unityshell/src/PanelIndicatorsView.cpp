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
#include <Nux/VLayout.h>

#include <NuxCore/Logger.h>

#include <NuxGraphics/GLThread.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

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
{
  LOG_DEBUG(logger) << "Indicators View Added: ";
  layout_ = new nux::HLayout("", NUX_TRACKER_LOCATION);

  SetCompositionLayout(layout_);
}

PanelIndicatorsView::~PanelIndicatorsView()
{
  for (auto it = indicators_connections_.begin(); it != indicators_connections_.end(); it++)
  {
    for (auto conn : it->second)
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

long
PanelIndicatorsView::ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;

  if (layout_)
    ret = layout_->ProcessEvent(ievent, ret, ProcessEventInfo);
  return ret;
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
  for (auto i = entries_.begin(), end = entries_.end(); i != end; ++i)
  {
    if (i->second)
      i->second->QueueDraw();
  }
}

bool
PanelIndicatorsView::ActivateEntry(std::string const& entry_id)
{
  auto entry = entries_.find(entry_id);

  if (entry != entries_.end() && entry->second->IsEntryValid())
  {
    LOG_DEBUG(logger) << "Activating: " << entry_id;
    entry->second->Activate();
    return true;
  }

  return false;
}

bool
PanelIndicatorsView::ActivateIfSensitive()
{
  for (auto i = entries_.begin(), end = entries_.end(); i != end; ++i)
  {
    PanelIndicatorEntryView* view = i->second;
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
  for (auto i = entries_.begin(), end = entries_.end(); i != end; ++i)
  {
    if (i->second)
      i->second->GetGeometryForSync(locations);
  }
}

bool
PanelIndicatorsView::OnPointerMoved(int x, int y)
{
  PanelIndicatorEntryView* target = NULL;
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

  return (target != NULL);
}

void
PanelIndicatorsView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());
  if (layout_)
    layout_->ProcessDraw(GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}

void
PanelIndicatorsView::OnEntryAdded(indicator::Entry::Ptr const& entry)
{
  auto view = new PanelIndicatorEntryView(entry);
  view->refreshed.connect(sigc::mem_fun(this, &PanelIndicatorsView::OnEntryRefreshed));

  int indicator_pos = nux::NUX_LAYOUT_BEGIN;

  if (entry->priority() > -1)
  {
    for (nux::Area* &area : layout_->GetChildren())
    {
      auto en = dynamic_cast<PanelIndicatorEntryView*>(area);

      if (en)
      {
        if (en && entry->priority() <= en->GetEntryPriority())
          break;

        indicator_pos++;
      }
    }
  }

  nux::LayoutPosition pos = (nux::LayoutPosition) indicator_pos;
  layout_->AddView(view, 0, nux::eCenter, nux::eFull, 1.0, pos);
  layout_->SetContentDistribution(nux::eStackRight);
  entries_[entry->id()] = view;

  AddChild(view);
  QueueRelayout();
  QueueDraw();

  on_indicator_updated.emit(view);
}

void
PanelIndicatorsView::OnEntryRefreshed(PanelIndicatorEntryView* view)
{
  QueueRelayout();
  QueueDraw();

  on_indicator_updated.emit(view);
}

void
PanelIndicatorsView::OnEntryRemoved(std::string const& entry_id)
{
  PanelIndicatorEntryView* view = entries_[entry_id];

  if (view)
  {
    on_indicator_updated.emit(view);
    layout_->RemoveChildObject(view);
    entries_.erase(entry_id);

    QueueRelayout();
    QueueDraw();
  }
}

void PanelIndicatorsView::DashShown()
{
  for (auto entry: entries_)
    entry.second->DashShown();
}

void PanelIndicatorsView::DashHidden()
{
  for (auto entry: entries_)
    entry.second->DashHidden();
}

const gchar* PanelIndicatorsView::GetName()
{
  return "IndicatorsView";
}

const gchar*
PanelIndicatorsView::GetChildsName()
{
  return "entries";
}

void
PanelIndicatorsView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder).add(GetGeometry());
}

} // namespace unity
