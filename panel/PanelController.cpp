// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-13 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "PanelController.h"

#include <vector>
#include <Nux/BaseWindow.h>

#include "unity-shared/UScreen.h"
#include "PanelView.h"

namespace unity
{
namespace panel
{

const char* window_title = "unity-panel";

class Controller::Impl
{
public:
  Impl(Controller*, menu::Manager::Ptr const&, ui::EdgeBarrierController::Ptr const&);
  ~Impl();

  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);

  float opacity() const;

  nux::ObjectPtr<PanelView> CreatePanel();
  void OnScreenChanged(unsigned int primary_monitor, std::vector<nux::Geometry> const& monitors);
  void UpdatePanelGeometries();

  typedef nux::ObjectPtr<nux::BaseWindow> BaseWindowPtr;

  PanelView* ViewForWindow(BaseWindowPtr const& window) const;

  Controller* parent_;
  menu::Manager::Ptr indicators_;
  ui::EdgeBarrierController::Ptr edge_barriers_;
  PanelVector panels_;
  std::vector<nux::Geometry> panel_geometries_;
  std::vector<Window> tray_xids_;
  float opacity_;
  bool opacity_maximized_toggle_;
};


Controller::Impl::Impl(Controller* parent, menu::Manager::Ptr const& indicators, ui::EdgeBarrierController::Ptr const& edge_barriers)
  : parent_(parent)
  , indicators_(indicators)
  , edge_barriers_(edge_barriers)
  , opacity_(1.0f)
  , opacity_maximized_toggle_(false)
{
  UScreen* screen = UScreen::GetDefault();
  screen->changed.connect(sigc::mem_fun(this, &Impl::OnScreenChanged));
  OnScreenChanged(screen->GetPrimaryMonitor(), screen->GetMonitors());
}

Controller::Impl::~Impl()
{
  // Since the panels are in a window which adds a reference to the
  // panel, we need to make sure the base windows are unreferenced
  // otherwise the pnales never die.
  for (auto const& panel_ptr : panels_)
  {
    if (panel_ptr)
      panel_ptr->GetParent()->UnReference();
  }
}

void Controller::Impl::UpdatePanelGeometries()
{
  panel_geometries_.reserve(panels_.size());

  for (auto const& panel : panels_)
  {
    panel_geometries_.push_back(panel->GetAbsoluteGeometry());
  }
}

void Controller::Impl::SetOpacity(float opacity)
{
  opacity_ = opacity;

  for (auto const& panel: panels_)
  {
    panel->SetOpacity(opacity_);
  }
}

void Controller::Impl::SetOpacityMaximizedToggle(bool enabled)
{
  opacity_maximized_toggle_ = enabled;

  for (auto const& panel: panels_)
  {
    panel->SetOpacityMaximizedToggle(opacity_maximized_toggle_);
  }
}

PanelView* Controller::Impl::ViewForWindow(BaseWindowPtr const& window) const
{
  nux::Layout* layout = window->GetLayout();
  auto it = layout->GetChildren().begin();

  return static_cast<PanelView*>(*it);
}

// We need to put a panel on every monitor, and try and re-use the panels we already have
void Controller::Impl::OnScreenChanged(unsigned int primary_monitor, std::vector<nux::Geometry> const& monitors)
{
  unsigned int num_monitors = monitors.size();
  unsigned int num_panels = num_monitors;
  unsigned int panels_size = panels_.size();
  unsigned int last_panel = 0;

  tray_xids_.resize(num_panels);

  for (unsigned int i = 0; i < num_panels; ++i, ++last_panel)
  {
    if (i >= panels_size)
    {
      panels_.push_back(CreatePanel());
    }
    else if (!panels_[i])
    {
      panels_[i] = CreatePanel();
    }

    if (panels_[i]->GetMonitor() != static_cast<int>(i))
    {
      edge_barriers_->RemoveHorizontalSubscriber(panels_[i].GetPointer(), panels_[i]->GetMonitor());
    }

    panels_[i]->SetMonitor(i);
    panels_[i]->geometry_changed.connect([this] (nux::Area*, nux::Geometry&) { UpdatePanelGeometries(); });
    tray_xids_[i] = panels_[i]->GetTrayXid();

    edge_barriers_->AddHorizontalSubscriber(panels_[i].GetPointer(), panels_[i]->GetMonitor());
  }

  for (unsigned int i = last_panel; i < panels_size; ++i)
  {
    auto const& panel = panels_[i];
    if (panel)
    {
      parent_->RemoveChild(panel.GetPointer());
      panel->GetParent()->UnReference();
      edge_barriers_->RemoveHorizontalSubscriber(panel.GetPointer(), panel->GetMonitor());
    }
  }

  panels_.resize(num_panels);
  UpdatePanelGeometries();
}


nux::ObjectPtr<PanelView> Controller::Impl::CreatePanel()
{
  auto* panel_window = new MockableBaseWindow(TEXT("PanelWindow"));

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  PanelView* view = new PanelView(panel_window, indicators_);
  view->SetOpacity(opacity_);
  view->SetOpacityMaximizedToggle(opacity_maximized_toggle_);

  layout->AddView(view, 1);
  layout->SetContentDistribution(nux::MAJOR_POSITION_START);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  panel_window->SetLayout(layout);
  panel_window->SetBackgroundColor(nux::color::Transparent);
  panel_window->ShowWindow(true);

  if (nux::GetWindowThread()->IsEmbeddedWindow())
    panel_window->EnableInputWindow(true, panel::window_title, false, false);

  panel_window->InputWindowEnableStruts(true);
  parent_->AddChild(view);

  return nux::ObjectPtr<PanelView>(view);
}

bool Controller::IsMouseInsideIndicator(nux::Point const& mouse_position) const
{
  for (auto panel : pimpl->panels_)
  {
    if (panel->IsMouseInsideIndicator(mouse_position))
      return true;
  }

  return false;
}

float Controller::Impl::opacity() const
{
  return opacity_;
}

Controller::Controller(menu::Manager::Ptr const& menus, ui::EdgeBarrierController::Ptr const& edge_barriers)
  : pimpl(new Impl(this, menus, edge_barriers))
{
}

Controller::~Controller()
{}

void Controller::SetOpacity(float opacity)
{
  pimpl->SetOpacity(opacity);
}

void Controller::SetOpacityMaximizedToggle(bool enabled)
{
  pimpl->SetOpacityMaximizedToggle(enabled);
}

std::vector<Window> const& Controller::GetTrayXids() const
{
  return pimpl->tray_xids_;
}

Controller::PanelVector& Controller::panels() const
{
  return pimpl->panels_;
}

std::vector<nux::Geometry> const& Controller::GetGeometries() const
{
  return pimpl->panel_geometries_;
}

float Controller::opacity() const
{
  return pimpl->opacity();
}

std::string Controller::GetName() const
{
  return "PanelController";
}

void Controller::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("opacity", pimpl->opacity());
}

} // namespace panel
} // namespace unity
