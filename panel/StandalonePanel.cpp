/*
 * Copyright 2010 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include <Nux/Nux.h>
#include <Nux/NuxTimerTickSource.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/WindowThread.h>
#include <NuxCore/AnimationController.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <NuxCore/Logger.h>
#include <UnityCore/DBusIndicators.h>
#include <gtk/gtk.h>

#include "unity-shared/MockableBaseWindow.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/KeyGrabber.h"
#include "unity-shared/PanelStyle.h"
#include "PanelView.h"

using namespace unity;
using namespace unity::panel;

struct PanelWindow
{
  PanelWindow()
    : wt(nux::CreateGUIThread("Unity Panel", 1024, 24, 0, &PanelWindow::ThreadWidgetInit, this))
    , animation_controller(tick_source)
  {}

  void Show()
  {
    wt->Run(nullptr);
  }

private:
  struct StandalonePanelView : public PanelView
  {
    // Used to sync menu geometries
    StandalonePanelView(MockableBaseWindow* window, menu::Manager::Ptr const& indicators)
      : PanelView(window, indicators)
    {}

    std::string GetName() const { return "StandalonePanel"; }
  };

  struct MockKeyGrabber : key::Grabber
  {
    CompAction::Vector& GetActions() { return actions_; }
    void AddAction(CompAction const&) {}
    void RemoveAction(CompAction const&) {}

    private:
      CompAction::Vector actions_;
  };

  void Init()
  {
    panel_window = new MockableBaseWindow("StandalonePanel");
    auto dbus_indicators = std::make_shared<indicator::DBusIndicators>();
    auto key_grabber = std::make_shared<MockKeyGrabber>();
    indicators_ = std::make_shared<menu::Manager>(dbus_indicators, key_grabber);

    PanelView* panel = new StandalonePanelView(panel_window.GetPointer(), indicators_);

    nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
    layout->AddView(panel, 1);
    layout->SetContentDistribution(nux::MAJOR_POSITION_START);

    panel_window->SetLayout(layout);
    panel_window->SetBackgroundColor(nux::color::Transparent);
    panel_window->ShowWindow(true);
    panel_window->SetWidth(1024);
    panel_window->SetXY(0, 0);
    panel_window->SetMaximumHeight(panel_style.PanelHeight());

    wt->window_configuration.connect([this] (int x, int y, int w, int h) {
      panel_window->SetWidth(w);
    });
  }

  static void ThreadWidgetInit(nux::NThread* thread, void* self)
  {
    static_cast<PanelWindow*>(self)->Init();
  }

  unity::Settings settings;
  panel::Style panel_style;
  std::shared_ptr<nux::WindowThread> wt;
  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller;
  nux::ObjectPtr<MockableBaseWindow> panel_window;
  menu::Manager::Ptr indicators_;
};

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);
  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  PanelWindow().Show();

  return 0;
}
