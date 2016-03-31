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

#ifndef UNITY_LOCKSCREEN_PANEL
#define UNITY_LOCKSCREEN_PANEL

#include <Nux/Nux.h>
#include <Nux/View.h>
#include "UnityCore/Indicators.h"
#include "UnityCore/GLibSource.h"
#include "UnityCore/SessionManager.h"

namespace unity
{
namespace panel
{
class PanelIndicatorsView;
}

namespace lockscreen
{

class Panel : public nux::View
{
public:
  Panel(int monitor, indicator::Indicators::Ptr const&, session::Manager::Ptr const&);

  nux::Property<bool> active;
  nux::Property<int> monitor;

  void ActivatePanel();

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw) override;
  bool InspectKeyEvent(unsigned int event_type, unsigned int keysym, const char*) override;

private:
  void ActivateFirst();
  void AddIndicator(indicator::Indicator::Ptr const&);
  void RemoveIndicator(indicator::Indicator::Ptr const&);
  void OnIndicatorViewUpdated();
  void OnEntryActivated(std::string const& panel, std::string const& entry_id, nux::Rect const& geo);
  void OnEntryShowMenu(std::string const& entry_id, unsigned xid, int x, int y, unsigned button);
  void OnEntryActivateRequest(std::string const& entry_id);

  void BuildTexture();
  std::string GetPanelName() const;

  indicator::Indicators::Ptr indicators_;
  panel::PanelIndicatorsView* indicators_view_;
  nux::ObjectPtr<nux::BaseTexture> bg_texture_;

  bool needs_geo_sync_;
  nux::Point tracked_pointer_pos_;
  glib::Source::UniquePtr track_menu_pointer_timeout_;
};

} // lockscreen namespace
} // unity namespace

#endif // UNITY_LOCKSCREEN_PANEL
