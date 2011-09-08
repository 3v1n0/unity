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

#ifndef PANEL_INDICATORS_VIEW_H
#define PANEL_INDICATORS_VIEW_H

#include <Nux/View.h>

#include <UnityCore/Indicator.h>

#include "PanelIndicatorEntryView.h"
#include "Introspectable.h"

namespace unity
{

class PanelIndicatorsView : public nux::View, public unity::Introspectable
{
public:
  PanelIndicatorsView();
  ~PanelIndicatorsView();

  void AddIndicator(indicator::Indicator::Ptr const& indicator);
  void RemoveIndicator(indicator::Indicator::Ptr const& indicator);

  bool OnPointerMoved(int x, int y);
  bool ActivateEntry(std::string const& entry_id);
  bool ActivateIfSensitive();
  void GetGeometryForSync(indicator::EntryLocationMap& locations);

  virtual long ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void QueueDraw();

  virtual void OnEntryAdded(indicator::Entry::Ptr const& proxy);
  virtual void OnEntryRefreshed(PanelIndicatorEntryView* view);
  virtual void OnEntryRemoved(std::string const& entry_id);

  sigc::signal<void, PanelIndicatorEntryView*> on_indicator_updated;

protected:
  nux::HLayout* layout_;
  typedef std::map<std::string, PanelIndicatorEntryView*> Entries;
  Entries entries_;

  const gchar* GetName();
  const gchar* GetChildsName();
  void         AddProperties(GVariantBuilder* builder);

private:
  typedef std::vector<indicator::Indicator::Ptr> Indicators;
  Indicators indicators_;

  std::map<indicator::Indicator::Ptr, std::vector<sigc::connection>> indicators_connections_;
};

}

#endif // PANEL_INDICATORS_VIEW_H
