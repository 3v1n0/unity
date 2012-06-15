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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "PanelController.h"

#include <vector>
#include <NuxCore/Logger.h>
#include <Nux/BaseWindow.h>
#include <UnityCore/Variant.h>

#include "unity-shared/UScreen.h"
#include "PanelView.h"
#include "unity-shared/PanelStyle.h"

namespace unity
{
namespace panel
{

const char window_title[] = "unity-panel";

namespace
{
nux::logging::Logger logger("unity.panel");
}

class Controller::Impl
{
public:
  Impl();

  void FirstMenuShow();
  void QueueRedraw();

  std::vector<Window> GetTrayXids() const;
  std::vector<nux::Geometry> GetGeometries() const;

  // NOTE: nux::Property maybe?
  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);

  float opacity() const;

  void SetMenuShowTimings(int fadein, int fadeout, int discovery,
                          int discovery_fadein, int discovery_fadeout);

  void OnScreenChanged(unsigned int primary_monitor, std::vector<nux::Geometry>& monitors, Introspectable *iobj);
private:
  typedef nux::ObjectPtr<nux::BaseWindow> BaseWindowPtr;

  unity::PanelView* ViewForWindow(BaseWindowPtr const& window) const;

  static void WindowConfigureCallback(int            window_width,
                                      int            window_height,
                                      nux::Geometry& geo,
                                      void*          user_data);

private:
  std::vector<BaseWindowPtr> windows_;
  float opacity_;
  bool opacity_maximized_toggle_;
  int menus_fadein_;
  int menus_fadeout_;
  int menus_discovery_;
  int menus_discovery_fadein_;
  int menus_discovery_fadeout_;
};


Controller::Impl::Impl()
  : opacity_(1.0f)
  , opacity_maximized_toggle_(false)
  , menus_fadein_(0)
  , menus_fadeout_(0)
  , menus_discovery_(0)
  , menus_discovery_fadein_(0)
  , menus_discovery_fadeout_(0)
{
}

std::vector<Window> Controller::Impl::GetTrayXids() const
{
  std::vector<Window> xids;

  for (auto window: windows_)
  {
    xids.push_back(ViewForWindow(window)->GetTrayXid());
  }

  return xids;
}

std::vector<nux::Geometry> Controller::Impl::GetGeometries() const
{
  std::vector<nux::Geometry> geometries;

  for (auto window : windows_)
  {
    geometries.push_back(window->GetAbsoluteGeometry());
  }

  return geometries;
}

void Controller::Impl::FirstMenuShow()
{
  for (auto window: windows_)
  {
    if (ViewForWindow(window)->FirstMenuShow())
      break;
  }
}

void Controller::Impl::SetOpacity(float opacity)
{
  opacity_ = opacity;

  for (auto window: windows_)
  {
    ViewForWindow(window)->SetOpacity(opacity_);
  }
}

void Controller::Impl::SetOpacityMaximizedToggle(bool enabled)
{
  opacity_maximized_toggle_ = enabled;

  for (auto window: windows_)
  {
    ViewForWindow(window)->SetOpacityMaximizedToggle(opacity_maximized_toggle_);
  }
}

void Controller::Impl::SetMenuShowTimings(int fadein, int fadeout, int discovery,
                                          int discovery_fadein, int discovery_fadeout)
{
  menus_fadein_ = fadein;
  menus_fadeout_ = fadeout;
  menus_discovery_ = discovery;
  menus_discovery_fadein_ = discovery_fadein;
  menus_discovery_fadeout_ = discovery_fadeout;

  for (auto window: windows_)
  {
    ViewForWindow(window)->SetMenuShowTimings(fadein, fadeout, discovery,
                                              discovery_fadein, discovery_fadeout);
  }
}

void Controller::Impl::QueueRedraw()
{
  for (auto window: windows_)
  {
    window->QueueDraw();
  }
}

PanelView* Controller::Impl::ViewForWindow(BaseWindowPtr const& window) const
{
  nux::Layout* layout = window->GetLayout();
  auto it = layout->GetChildren().begin();

  return static_cast<PanelView*>(*it);
}

// We need to put a panel on every monitor, and try and re-use the panels we already have
void Controller::Impl::OnScreenChanged(unsigned int primary_monitor,
                                       std::vector<nux::Geometry>& monitors,
                                       Introspectable *iobj)
{
  std::vector<BaseWindowPtr>::iterator it;
  unsigned n_monitors = monitors.size();
  unsigned int i = 0;

  for (it = windows_.begin(); it != windows_.end(); ++it)
  {
    if (i < n_monitors)
    {
      PanelView* view;

      (*it)->EnableInputWindow(false);
      (*it)->InputWindowEnableStruts(false);

      nux::Geometry geo = monitors[i];
      geo.height = panel::Style::Instance().panel_height;
      (*it)->SetGeometry(geo);
      (*it)->SetMinMaxSize(geo.width, geo.height);

      view = ViewForWindow(*it);
      view->SetPrimary(i == primary_monitor);
      view->SetMonitor(i);

      (*it)->EnableInputWindow(true);
      (*it)->InputWindowEnableStruts(true);

      LOG_DEBUG(logger) << "Updated Panel for Monitor " << i;

      i++;
    }
    else
      break;
  }

  // Add new ones if needed
  if (i < n_monitors)
  {
    for (i = i; i < n_monitors; i++)
    {
      nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);

      PanelView* view = new PanelView();
      view->SetMaximumHeight(panel::Style::Instance().panel_height);
      view->SetOpacity(opacity_);
      view->SetOpacityMaximizedToggle(opacity_maximized_toggle_);
      view->SetMenuShowTimings(menus_fadein_, menus_fadeout_, menus_discovery_,
                               menus_discovery_fadein_, menus_discovery_fadeout_);
      view->SetPrimary(i == primary_monitor);
      view->SetMonitor(i);

      layout->AddView(view, 1);
      layout->SetContentDistribution(nux::eStackLeft);
      layout->SetVerticalExternalMargin(0);
      layout->SetHorizontalExternalMargin(0);

      BaseWindowPtr window(new nux::BaseWindow());
      nux::Geometry geo = monitors[i];
      geo.height = panel::Style::Instance().panel_height;

      window->SetConfigureNotifyCallback(&Impl::WindowConfigureCallback, window.GetPointer());
      window->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
      window->ShowWindow(true);
      window->EnableInputWindow(true, panel::window_title, false, false);
      window->InputWindowEnableStruts(true);
      window->SetGeometry(geo);
      window->SetMinMaxSize(geo.width, geo.height);
      window->SetLayout(layout);

      windows_.push_back(window);

      // add to introspectable tree:
      iobj->AddChild(view);

      LOG_DEBUG(logger) << "Added Panel for Monitor " << i;
    }
  }

  if (windows_.size() > n_monitors)
  {
    LOG_DEBUG(logger) << "Removed extra Panels";
    windows_.erase(it, windows_.end());
  }
}

void Controller::Impl::WindowConfigureCallback(int window_width,
                                               int window_height,
                                               nux::Geometry& geo,
                                               void* user_data)
{
  nux::BaseWindow* window = static_cast<nux::BaseWindow*>(user_data);
  geo = window->GetGeometry();
}

float Controller::Impl::opacity() const
{
  return opacity_;
}

Controller::Controller()
  : pimpl(new Impl())
{
  UScreen* screen = UScreen::GetDefault();
  screen->changed.connect(sigc::mem_fun(this, &Controller::OnScreenChanged));
  OnScreenChanged(screen->GetPrimaryMonitor(), screen->GetMonitors());
}

Controller::~Controller()
{
  delete pimpl;
}

void Controller::FirstMenuShow()
{
  pimpl->FirstMenuShow();
}

void Controller::SetOpacity(float opacity)
{
  pimpl->SetOpacity(opacity);
}

void Controller::SetOpacityMaximizedToggle(bool enabled)
{
  pimpl->SetOpacityMaximizedToggle(enabled);
}

void Controller::SetMenuShowTimings(int fadein, int fadeout, int discovery,
                                    int discovery_fadein, int discovery_fadeout)
{
  pimpl->SetMenuShowTimings(fadein, fadeout, discovery, discovery_fadein, discovery_fadeout);
}

void Controller::QueueRedraw()
{
  pimpl->QueueRedraw();
}

std::vector<Window> Controller::GetTrayXids() const
{
  return pimpl->GetTrayXids();
}

std::vector<nux::Geometry> Controller::GetGeometries() const
{
  return pimpl->GetGeometries();
}

float Controller::opacity() const
{
  return pimpl->opacity();
}

std::string Controller::GetName() const
{
  return "PanelController";
}

void Controller::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add("opacity", pimpl->opacity());
}

void Controller::OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors)
{
  pimpl->OnScreenChanged(primary_monitor, monitors, this);
}

} // namespace panel
} // namespace unity
