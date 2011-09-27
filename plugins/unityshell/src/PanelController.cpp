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

#include <NuxCore/Logger.h>

#include "UScreen.h"

#include "unitya11y.h"
#include "unity-util-accessible.h"

using namespace unity;

namespace
{
nux::logging::Logger logger("unity.panel");
}

PanelController::PanelController()
  : _bfb_size(66),
    _opacity(1.0f),
    _open_menu_start_received(false)
{
  UScreen* screen = UScreen::GetDefault();
  screen->changed.connect(sigc::mem_fun(this, &PanelController::OnScreenChanged));
  OnScreenChanged(screen->GetPrimaryMonitor(), screen->GetMonitors());
}

PanelController::~PanelController()
{
  for (auto win : _windows)
  {
    win->UnReference();
  }
}

unsigned int PanelController::GetTrayXid ()
{
  if (!_windows.empty ())
    return ViewForWindow (_windows.front ())->GetTrayXid ();
  else
    return 0;
}

void
PanelController::StartFirstMenuShow()
{
  for (auto win : _windows)
  {
    PanelView* view = ViewForWindow(win);
    view->StartFirstMenuShow();
  }

  _open_menu_start_received = true;
}

void
PanelController::EndFirstMenuShow()
{
  if (!_open_menu_start_received)
    return;
  _open_menu_start_received = false;

  for (auto win : _windows)
  {
    PanelView* view = ViewForWindow(win);
    view->EndFirstMenuShow();
  }
}

void
PanelController::SetOpacity(float opacity)
{
  for (auto win : _windows)
  {
    ViewForWindow(win)->SetOpacity(_opacity);
  }
  _opacity = opacity;
}

void
PanelController::SetOpacityMaximizedToggle(bool enabled)
{
  for (auto win : _windows)
  {
    ViewForWindow(win)->SetOpacityMaximizedToggle(enabled);
  }
}

void PanelController::QueueRedraw()
{
  for (auto win : _windows)
  {
    win->QueueDraw();
  }
}

PanelView*
PanelController::ViewForWindow(nux::BaseWindow* window)
{
  nux::Layout* layout = window->GetLayout();
  std::list<nux::Area*>::iterator it = layout->GetChildren().begin();

  return static_cast<PanelView*>(*it);
}

// We need to put a panel on every monitor, and try and re-use the panels we already have
void
PanelController::OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors)
{
  std::vector<nux::BaseWindow*>::iterator it, eit = _windows.end();
  int n_monitors = monitors.size();
  int i = 0;

  for (it = _windows.begin(); it != eit; ++it)
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
      nux::BaseWindow* window;
      PanelView*       view;
      nux::HLayout*    layout;

      // FIXME(loicm): Several objects created here are leaked.

      layout = new nux::HLayout(NUX_TRACKER_LOCATION);

      view = new PanelView();
      view->SetMaximumHeight(24);
      view->SetOpacity(_opacity);
      view->SetPrimary(i == primary_monitor);
      view->SetMonitor(i);
      AddChild(view);

      layout->AddView(view, 1);
      layout->SetContentDistribution(nux::eStackLeft);
      layout->SetVerticalExternalMargin(0);
      layout->SetHorizontalExternalMargin(0);

      window = new nux::BaseWindow("");
      window->SinkReference();
      window->SetConfigureNotifyCallback(&PanelController::WindowConfigureCallback, window);
      window->SetLayout(layout);
      window->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
      window->ShowWindow(true);
      window->EnableInputWindow(true, "panel", false, false);
      window->InputWindowEnableStruts(true);

      nux::Geometry geo = monitors[i];
      geo.height = 24;
      window->SetGeometry(geo);

      /* FIXME: this should not be manual, should be managed with a
         show/hide callback like in GAIL*/
      if (unity_a11y_initialized() == TRUE)
        unity_util_accessible_add_window(window);

      _windows.push_back(window);

      LOG_DEBUG(logger) << "Added Panel for Monitor " << i;
    }
  }

  if ((int)_windows.size() > n_monitors)
  {
    std::vector<nux::BaseWindow*>::iterator sit;
    for (sit = it; sit != eit; ++sit)
    {
      (*sit)->UnReference();
      LOG_DEBUG(logger) << "Removed extra Panel";
    }

    _windows.erase(it, _windows.end());
  }
}

void
PanelController::WindowConfigureCallback(int            window_width,
                                         int            window_height,
                                         nux::Geometry& geo,
                                         void*          user_data)
{
  nux::BaseWindow* window = static_cast<nux::BaseWindow*>(user_data);
  geo = window->GetGeometry();
}

float PanelController::opacity() const
{
  return _opacity;
}

const gchar*
PanelController::GetName()
{
  return "PanelController";
}

void
PanelController::AddProperties(GVariantBuilder* builder)
{

}
