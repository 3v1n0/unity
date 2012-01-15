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

#include "UScreen.h"
#include "PanelView.h"

namespace unity
{
namespace panel
{

namespace
{
nux::logging::Logger logger("unity.panel");
}

class Controller::Impl : public sigc::trackable
{
public:
  Impl();
  ~Impl();

  void StartFirstMenuShow();
  void EndFirstMenuShow();
  void QueueRedraw();

  unsigned int GetTrayXid();
  std::list <nux::Geometry> GetGeometries();

  // NOTE: nux::Property maybe?
  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);

  float opacity() const;

  void SetMenuShowTimings(int fadein, int fadeout, int discovery,
                          int discovery_fadein, int discovery_fadeout);

private:
  unity::PanelView* ViewForWindow(nux::BaseWindow* window);
  void OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors);

  static void WindowConfigureCallback(int            window_width,
                                      int            window_height,
                                      nux::Geometry& geo,
                                      void*          user_data);
private:
  std::vector<nux::BaseWindow*> windows_;
  float opacity_;
  bool opacity_maximized_toggle_;
  bool open_menu_start_received_;
  int menus_fadein_;
  int menus_fadeout_;
  int menus_discovery_;
  int menus_discovery_fadein_;
  int menus_discovery_fadeout_;
};


Controller::Impl::Impl()
  : opacity_(1.0f)
  , opacity_maximized_toggle_(false)
  , open_menu_start_received_(false)
{
  UScreen* screen = UScreen::GetDefault();
  screen->changed.connect(sigc::mem_fun(this, &Impl::OnScreenChanged));
  OnScreenChanged(screen->GetPrimaryMonitor(), screen->GetMonitors());
}

Controller::Impl::~Impl()
{
  for (auto window : windows_)
  {
    window->UnReference();
  }
}

unsigned int Controller::Impl::GetTrayXid()
{
  if (!windows_.empty())
    return ViewForWindow(windows_.front())->GetTrayXid();
  else
    return 0;
}

std::list<nux::Geometry> Controller::Impl::GetGeometries()
{
  std::list<nux::Geometry> geometries;

  for (auto window : windows_)
  {
    geometries.push_back(window->GetAbsoluteGeometry());
  }

  return geometries;
}

void Controller::Impl::StartFirstMenuShow()
{
  for (auto window: windows_)
  {
    PanelView* view = ViewForWindow(window);
    view->StartFirstMenuShow();
  }

  open_menu_start_received_ = true;
}

void Controller::Impl::EndFirstMenuShow()
{
  if (!open_menu_start_received_)
    return;
  open_menu_start_received_ = false;

  for (auto window: windows_)
  {
    PanelView* view = ViewForWindow(window);
    view->EndFirstMenuShow();
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

PanelView* Controller::Impl::ViewForWindow(nux::BaseWindow* window)
{
  nux::Layout* layout = window->GetLayout();
  std::list<nux::Area*>::iterator it = layout->GetChildren().begin();

  return static_cast<PanelView*>(*it);
}

// We need to put a panel on every monitor, and try and re-use the panels we already have
void Controller::Impl::OnScreenChanged(int primary_monitor,
                                       std::vector<nux::Geometry>& monitors)
{
  std::vector<nux::BaseWindow*>::iterator it, eit = windows_.end();
  int n_monitors = monitors.size();
  int i = 0;

  for (it = windows_.begin(); it != eit; ++it)
  {
    if (i < n_monitors)
    {
      PanelView* view;

      (*it)->EnableInputWindow(false);
      (*it)->InputWindowEnableStruts(false);

      nux::Geometry geo = monitors[i];
      geo.height = 24;
      (*it)->SetGeometry(geo);

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
      view->SetMaximumHeight(24);
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

      nux::BaseWindow* window = new nux::BaseWindow("");
      window->SinkReference();
      window->SetConfigureNotifyCallback(&Impl::WindowConfigureCallback, window);
      window->SetLayout(layout);
      window->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
      window->ShowWindow(true);
      window->EnableInputWindow(true, "panel", false, false);
      window->InputWindowEnableStruts(true);

      nux::Geometry geo = monitors[i];
      geo.height = 24;
      window->SetGeometry(geo);

      windows_.push_back(window);

      LOG_DEBUG(logger) << "Added Panel for Monitor " << i;
    }
  }

  if ((int)windows_.size() > n_monitors)
  {
    std::vector<nux::BaseWindow*>::iterator sit;
    for (sit = it; sit != eit; ++sit)
    {
      (*sit)->UnReference();
      LOG_DEBUG(logger) << "Removed extra Panel";
    }

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
}

Controller::~Controller()
{
  delete pimpl;
}

void Controller::StartFirstMenuShow()
{
  pimpl->StartFirstMenuShow();
}

void Controller::EndFirstMenuShow()
{
  pimpl->EndFirstMenuShow();
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

unsigned int Controller::GetTrayXid()
{
  return pimpl->GetTrayXid();
}

std::list<nux::Geometry> Controller::GetGeometries()
{
  return pimpl->GetGeometries();
}

float Controller::opacity() const
{
  return pimpl->opacity();
}


} // namespace panel
} // namespace unity
