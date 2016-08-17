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

#include "LockScreenPanel.h"

#include <boost/algorithm/string/trim.hpp>
#include <Nux/HLayout.h>

#include "LockScreenSettings.h"
#include "panel/PanelIndicatorsView.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/RawPixel.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const RawPixel PADDING = 5_em;
}

using namespace indicator;
using namespace panel;

Panel::Panel(int monitor_, Indicators::Ptr const& indicators, session::Manager::Ptr const& session_manager)
  : nux::View(NUX_TRACKER_LOCATION)
  , active(false)
  , monitor(monitor_)
  , indicators_(indicators)
  , needs_geo_sync_(true)
{
  double scale = unity::Settings::Instance().em(monitor)->DPIScale();
  auto* layout = new nux::HLayout();
  layout->SetLeftAndRightPadding(PADDING.CP(scale), 0);
  SetLayout(layout);

  BuildTexture();

  // Add setting
  auto *hostname = new StaticCairoText(session_manager->HostName());
  hostname->SetFont(Settings::Instance().font_name());
  hostname->SetTextColor(nux::color::White);
  hostname->SetInputEventSensitivity(false);
  hostname->SetScale(scale);
  hostname->SetVisible(Settings::Instance().show_hostname());
  layout->AddView(hostname, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  indicators_view_ = new PanelIndicatorsView();
  indicators_view_->SetMonitor(monitor);
  indicators_view_->OverlayShown();
  indicators_view_->on_indicator_updated.connect(sigc::mem_fun(this, &Panel::OnIndicatorViewUpdated));
  layout->AddView(indicators_view_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  for (auto const& indicator : indicators_->GetIndicators())
    AddIndicator(indicator);

  indicators_->on_object_added.connect(sigc::mem_fun(this, &Panel::AddIndicator));
  indicators_->on_object_removed.connect(sigc::mem_fun(this, &Panel::RemoveIndicator));
  indicators_->on_entry_show_menu.connect(sigc::mem_fun(this, &Panel::OnEntryShowMenu));
  indicators_->on_entry_activated.connect(sigc::mem_fun(this, &Panel::OnEntryActivated));
  indicators_->on_entry_activate_request.connect(sigc::mem_fun(this, &Panel::OnEntryActivateRequest));

  monitor.changed.connect([this, hostname] (int monitor) {
    double scale = unity::Settings::Instance().em(monitor)->DPIScale();
    hostname->SetScale(scale);
    static_cast<nux::HLayout*>(GetLayout())->SetLeftAndRightPadding(PADDING.CP(scale), 0);
    indicators_view_->SetMonitor(monitor);
    BuildTexture();
    QueueRelayout();
  });
}

void Panel::BuildTexture()
{
  int height = panel::Style::Instance().PanelHeight(monitor);
  nux::CairoGraphics context(CAIRO_FORMAT_ARGB32, 1, height);
  auto* cr = context.GetInternalContext();
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_paint_with_alpha(cr, 0.4);
  bg_texture_ = texture_ptr_from_cairo_graphics(context);

  view_layout_->SetMinimumHeight(height);
  view_layout_->SetMaximumHeight(height);
}

void Panel::AddIndicator(Indicator::Ptr const& indicator)
{
  if (indicator->IsAppmenu())
    return;

  indicators_view_->AddIndicator(indicator);

  if (!active)
  {
    for (auto const& entry : indicator->GetEntries())
    {
      if (entry->active())
      {
        active = true;
        indicators_view_->ActivateEntry(entry);
        OnEntryActivated(GetPanelName(), entry->id(), entry->geometry());
        break;
      }
    }
  }

  QueueRelayout();
  QueueDraw();
}

void Panel::RemoveIndicator(indicator::Indicator::Ptr const& indicator)
{
  if (active)
  {
    for (auto const& entry : indicator->GetEntries())
    {
      if (entry->active())
      {
        active = false;
        break;
      }
    }
  }

  indicators_view_->RemoveIndicator(indicator);
  QueueRelayout();
  QueueDraw();
}

std::string Panel::GetPanelName() const
{
  return "LockScreenPanel";
}

void Panel::OnIndicatorViewUpdated()
{
  needs_geo_sync_ = true;
  QueueRelayout();
  QueueDraw();
}

void Panel::OnEntryShowMenu(std::string const& entry_id, unsigned xid, int x, int y, unsigned button)
{
  if (!GetInputEventSensitivity())
    return;

  if (!active)
  {
    // This is ugly... But Nux fault!
    WindowManager::Default().UnGrabMousePointer(CurrentTime, button, x, y);
    active = true;
  }
}

void Panel::OnEntryActivateRequest(std::string const& entry_id)
{
  if (GetInputEventSensitivity())
    indicators_view_->ActivateEntry(entry_id, 0);
}

void Panel::OnEntryActivated(std::string const& panel, std::string const& entry_id, nux::Rect const&)
{
  if (!GetInputEventSensitivity() || (!panel.empty() && panel != GetPanelName()))
    return;

  bool active = !entry_id.empty();

  if (active && !WindowManager::Default().IsScreenGrabbed())
  {
    // The menu didn't grab the keyboard, let's take it back.
    nux::GetWindowCompositor().GrabKeyboardAdd(static_cast<nux::BaseWindow*>(GetTopLevelViewWindow()));
  }

  if (active && !track_menu_pointer_timeout_)
  {
    track_menu_pointer_timeout_.reset(new glib::Timeout(16));
    track_menu_pointer_timeout_->Run([this] {
      nux::Point const& mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
      if (tracked_pointer_pos_ != mouse)
      {
        if (GetAbsoluteGeometry().IsPointInside(mouse.x, mouse.y))
          indicators_view_->ActivateEntryAt(mouse.x, mouse.y);

        tracked_pointer_pos_ = mouse;
      }

      return true;
    });
  }
  else if (!active)
  {
    track_menu_pointer_timeout_.reset();
    tracked_pointer_pos_ = {-1, -1};
    this->active = false;
  }
}

void Panel::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  auto const& geo = GetGeometry();

  unsigned int alpha, src, dest = 0;
  graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
  graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  graphics_engine.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(graphics_engine, geo);

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_CLAMP);
  graphics_engine.QRP_1Tex(geo.x, geo.y, geo.width, geo.height,
                           bg_texture_->GetDeviceTexture(), texxform,
                           nux::color::White);

  view_layout_->ProcessDraw(graphics_engine, force_draw);

  graphics_engine.PopClippingRectangle();
  graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);

  if (needs_geo_sync_)
  {
    EntryLocationMap locations;
    indicators_view_->GetGeometryForSync(locations);
    indicators_->SyncGeometries(GetPanelName(), locations);
    needs_geo_sync_ = false;
  }
}

bool Panel::InspectKeyEvent(unsigned int event_type, unsigned int keysym, const char*)
{
  return true;
}

void Panel::ActivatePanel()
{
  if (GetInputEventSensitivity())
    indicators_view_->ActivateIfSensitive();
}

}
}
