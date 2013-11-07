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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "XdndCollectionWindowImp.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

namespace unity {
namespace {

class PrivateWindow : public nux::BaseWindow
{
public:
  PrivateWindow(XdndCollectionWindowImp* parent)
    : nux::BaseWindow("")
    , parent_(parent)
  {
    // Make it invisible...
    SetBackgroundColor(nux::color::Transparent);
    SetOpacity(0.0f);
    // ... and as big as the whole screen.
    auto uscreen = UScreen::GetDefault();
    SetGeometry(uscreen->GetScreenGeometry());

    // We are not calling ShowWindow () as this window
    // isn't really visible
    PushToBack();

    if (nux::GetWindowThread()->IsEmbeddedWindow())
    {
      // Hack to create the X Window as soon as possible.
      EnableInputWindow(true, "XdndCollectionWindowImp");
      EnableInputWindow(false, "XdndCollectionWindowImp");
    }

    SetDndEnabled(false, true);

    uscreen->changed.connect(sigc::mem_fun(this, &PrivateWindow::OnScreenChanged));
    WindowManager::Default().window_moved.connect(sigc::mem_fun(this, &PrivateWindow::OnWindowMoved));
  }

  void OnScreenChanged(int /*primary*/, std::vector<nux::Geometry>& /*monitors*/)
  {
    auto uscreen = UScreen::GetDefault();
    SetGeometry(uscreen->GetScreenGeometry());
  }

  /**
   * EnableInputWindow doesn't show the window immediately.
   * Since nux::EnableInputWindow uses XMoveResizeWindow the best way to know if
   * the X Window is really on/off screen is receiving WindowManager::window_moved
   * signal. Please don't hate me!
   **/
  void OnWindowMoved(Window window_id)
  {
    if (G_LIKELY(window_id != GetInputWindowId()))
      return;

    // Create a fake mouse move because sometimes an extra one is required.
    auto display = nux::GetGraphicsDisplay()->GetX11Display();
    XWarpPointer(display, None, None, 0, 0, 0, 0, 0, 0);
    XFlush(display);
  }

  void ProcessDndMove(int x, int y, std::list<char*> mimes)
  {
    // Hide the window as soon as possible.
    PushToBack();
    EnableInputWindow(false, "XdndCollectionWindowImp");

    std::vector<std::string> data;
    for (auto mime : mimes)
      if (mime) data.push_back(mime);

    parent_->collected.emit(data);
  }

  XdndCollectionWindowImp* parent_;
};

}

XdndCollectionWindowImp::XdndCollectionWindowImp()
  : window_(new PrivateWindow(this))
{}

void XdndCollectionWindowImp::Collect()
{
  // Using PushToFront we're sure that the window is shown over the panel window,
  // the launcher window and the dash window. Don't forget to call PushToBack as
  // soon as possible.
  window_->ShowWindow(true);
  window_->PushToFront();

  if (nux::GetWindowThread()->IsEmbeddedWindow())
    window_->EnableInputWindow(true, "XdndCollectionWindowImp");
}

void XdndCollectionWindowImp::Deactivate()
{
  window_->ShowWindow(false);
  window_->PushToBack();

  if (nux::GetWindowThread()->IsEmbeddedWindow())
    window_->EnableInputWindow(false, "XdndCollectionWindowImp");
}

std::string XdndCollectionWindowImp::GetData(std::string const& type)
{
  auto& gp_display = nux::GetWindowThread()->GetGraphicsDisplay();
  return glib::String(gp_display.GetDndData(const_cast<char*>(type.c_str()))).Str();
}

}
