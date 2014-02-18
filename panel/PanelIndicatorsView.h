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
#include <Nux/Layout.h>

#include <UnityCore/Indicators.h>

#include "PanelIndicatorEntryDropdownView.h"
#include "unity-shared/Introspectable.h"

namespace unity
{
namespace panel
{

class PanelIndicatorsView : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(PanelIndicatorsView, nux::View);
public:
  PanelIndicatorsView();

  void AddIndicator(indicator::Indicator::Ptr const& indicator);
  void RemoveIndicator(indicator::Indicator::Ptr const& indicator);

  enum IndicatorEntryPosition {
    AUTO = -1,
    START = nux::NUX_LAYOUT_BEGIN,
    END = nux::NUX_LAYOUT_END,
  };

  typedef PanelIndicatorEntryView::IndicatorEntryType IndicatorEntryType;

  PanelIndicatorEntryView* AddEntry(indicator::Entry::Ptr const& entry,
                                    int padding = 5,
                                    IndicatorEntryPosition pos = AUTO,
                                    IndicatorEntryType type = IndicatorEntryType::INDICATOR);
  void RemoveEntry(std::string const& entry_id);

  PanelIndicatorEntryView* ActivateEntryAt(int x, int y, int button = 1);
  PanelIndicatorEntryView* ActivateEntry(std::string const& entry_id, int button = 1);
  bool ActivateIfSensitive();

  virtual void OverlayShown();
  virtual void OverlayHidden();

  void SetMaximumEntriesWidth(int max_width);
  void GetGeometryForSync(indicator::EntryLocationMap& locations);

  void EnableDropdownMenu(bool, indicator::Indicators::Ptr const& i = nullptr);

  virtual void SetMonitor(int monitor);

  nux::Property<double> opacity;

  sigc::signal<void> on_indicator_updated;
  sigc::signal<void, PanelIndicatorEntryView*> entry_added;
  sigc::signal<void, PanelIndicatorEntryView*> entry_removed;

protected:
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  typedef std::vector<indicator::Indicator::Ptr> Indicators;
  Indicators const& GetIndicators() const;

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  virtual void OnEntryAdded(indicator::Entry::Ptr const& entry);
  virtual void OnEntryRefreshed(PanelIndicatorEntryView* view);

  void AddEntryView(PanelIndicatorEntryView* view, IndicatorEntryPosition pos = AUTO);
  void RemoveEntryView(PanelIndicatorEntryView* view);

  nux::HLayout* layout_;
  typedef std::map<std::string, PanelIndicatorEntryView*> Entries;
  Entries entries_;

  int monitor_;

private:
  bool SetOpacity(double& target, double const& new_value);

  Indicators indicators_;
  PanelIndicatorEntryDropdownView::Ptr dropdown_;
  std::unordered_map<indicator::Indicator::Ptr, connection::Manager> indicators_connections_;
};

} // panel namespace
} // unity namespace

#endif // PANEL_INDICATORS_VIEW_H
