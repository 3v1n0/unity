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
#include <NuxCore/Logger.h>
#include <Nux/BaseWindow.h>

#include "unity-shared/UScreen.h"
#include "PanelView.h"
#include "unity-shared/PanelStyle.h"

namespace unity
{
namespace panel
{
DECLARE_LOGGER(logger, "unity.panel.controller");

const char* window_title = "unity-panel";

class Controller::Impl
{
public:
  Impl(ui::EdgeBarrierController::Ptr const& edge_barriers, GnomeKeyGrabber::Ptr const& grabber);
  ~Impl();

  void GrabIndicatorMnemonics(indicator::Indicator::Ptr const& indicator);
  void UngrabIndicatorMnemonics(indicator::Indicator::Ptr const& indicator);
  void GrabEntryMnemonics(indicator::Entry::Ptr const& entry);
  void UngrabEntryMnemonics(std::string const& entry_id);

  void SetMenuBarVisible(bool visible);
  void FirstMenuShow();
  void QueueRedraw();

  // NOTE: nux::Property maybe?
  void SetLauncherWidth(int width);
  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);

  float opacity() const;

  void SetMenuShowTimings(int fadein, int fadeout, int discovery,
                          int discovery_fadein, int discovery_fadeout);

  nux::ObjectPtr<PanelView> CreatePanel(Introspectable *iobj);
  void OnScreenChanged(unsigned int primary_monitor, std::vector<nux::Geometry>& monitors, Introspectable *iobj);
  void UpdatePanelGeometries();

  typedef nux::ObjectPtr<nux::BaseWindow> BaseWindowPtr;

  unity::PanelView* ViewForWindow(BaseWindowPtr const& window) const;

  ui::EdgeBarrierController::Ptr edge_barriers_;
  GnomeKeyGrabber::Ptr grabber_;
  PanelVector panels_;
  std::vector<nux::Geometry> panel_geometries_;
  std::vector<Window> tray_xids_;
  float opacity_;
  bool opacity_maximized_toggle_;
  int menus_fadein_;
  int menus_fadeout_;
  int menus_discovery_;
  int menus_discovery_fadein_;
  int menus_discovery_fadeout_;
  indicator::DBusIndicators::Ptr dbus_indicators_;
  std::map<std::string, std::shared_ptr<CompAction>> entry_actions_;
  connection::Manager dbus_indicators_connections_;
  std::map<indicator::Indicator::Ptr, connection::Manager> indicator_connections_;
};


Controller::Impl::Impl(ui::EdgeBarrierController::Ptr const& edge_barriers, GnomeKeyGrabber::Ptr const& grabber)
  : edge_barriers_(edge_barriers)
  , grabber_(grabber)
  , opacity_(1.0f)
  , opacity_maximized_toggle_(false)
  , menus_fadein_(0)
  , menus_fadeout_(0)
  , menus_discovery_(0)
  , menus_discovery_fadein_(0)
  , menus_discovery_fadeout_(0)
  , dbus_indicators_(std::make_shared<indicator::DBusIndicators>())
{
  for (auto const& indicator : dbus_indicators_->GetIndicators())
    GrabIndicatorMnemonics(indicator);

  auto& connections(dbus_indicators_connections_);
  connections.Add(dbus_indicators_->on_object_added.connect(sigc::mem_fun(this, &Impl::GrabIndicatorMnemonics)));
  connections.Add(dbus_indicators_->on_object_removed.connect(sigc::mem_fun(this, &Impl::UngrabIndicatorMnemonics)));
}

Controller::Impl::~Impl()
{
  dbus_indicators_connections_.Clear();

  for (auto const& indicator : dbus_indicators_->GetIndicators())
    UngrabIndicatorMnemonics(indicator);

  // Since the panels are in a window which adds a reference to the
  // panel, we need to make sure the base windows are unreferenced
  // otherwise the pnales never die.
  for (auto const& panel_ptr : panels_)
  {
    if (panel_ptr)
      panel_ptr->GetParent()->UnReference();
  }
}

void Controller::Impl::GrabIndicatorMnemonics(indicator::Indicator::Ptr const& indicator)
{
  if (indicator->IsAppmenu())
  {
    for (auto const& entry : indicator->GetEntries())
      GrabEntryMnemonics(entry);

    auto& connections(indicator_connections_[indicator]);
    connections.Add(indicator->on_entry_added.connect(sigc::mem_fun(this, &Impl::GrabEntryMnemonics)));
    connections.Add(indicator->on_entry_removed.connect(sigc::mem_fun(this, &Impl::UngrabEntryMnemonics)));
  }
}

void Controller::Impl::UngrabIndicatorMnemonics(indicator::Indicator::Ptr const& indicator)
{
  if (indicator->IsAppmenu())
  {
    indicator_connections_.erase(indicator);

    for (auto const& entry : indicator->GetEntries())
      UngrabEntryMnemonics(entry->id());
  }
}

void Controller::Impl::GrabEntryMnemonics(indicator::Entry::Ptr const& entry)
{
  gunichar mnemonic;

  if (pango_parse_markup(entry->label().c_str(), -1, '_', NULL, NULL, &mnemonic, NULL) && mnemonic)
  {
    auto accelerator(gtk_accelerator_name(gdk_keyval_to_lower(gdk_unicode_to_keyval(mnemonic)), GDK_MOD1_MASK));
    auto action(std::make_shared<CompAction>());

    action->keyFromString(accelerator);
    action->setState(CompAction::StateInitKey);
    action->setInitiate([=](CompAction* action, CompAction::State state, CompOption::Vector& options)
    {
      for (auto const& panel : panels_)
      {
        if (panel->ActivateEntry(entry->id()))
        {
          return true;
        }
      }

      return false;
    });

    entry_actions_[entry->id()] = action;
    grabber_->addAction(*action);

    g_free (accelerator);
  }
}

void Controller::Impl::UngrabEntryMnemonics(std::string const& entry_id)
{
  auto i(entry_actions_.find(entry_id));

  if (i != entry_actions_.end())
  {
    grabber_->removeAction(*i->second);
    entry_actions_.erase(i);
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

void Controller::Impl::SetMenuBarVisible(bool visible)
{
  for (auto const& panel : panels_)
  {
    if (panel->SetMenuBarVisible(visible) && visible)
      break;
  }
}

void Controller::Impl::FirstMenuShow()
{
  for (auto const& panel: panels_)
  {
    if (panel->FirstMenuShow())
      break;
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

void Controller::Impl::SetLauncherWidth(int width)
{
  for (auto const& panel: panels_)
  {
    panel->SetLauncherWidth(width);
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

void Controller::Impl::SetMenuShowTimings(int fadein, int fadeout, int discovery,
                                          int discovery_fadein, int discovery_fadeout)
{
  menus_fadein_ = fadein;
  menus_fadeout_ = fadeout;
  menus_discovery_ = discovery;
  menus_discovery_fadein_ = discovery_fadein;
  menus_discovery_fadeout_ = discovery_fadeout;

  for (auto const& panel: panels_)
  {
    panel->SetMenuShowTimings(fadein, fadeout, discovery,
                              discovery_fadein, discovery_fadeout);
  }
}

void Controller::Impl::QueueRedraw()
{
  for (auto const& panel: panels_)
  {
    panel->QueueDraw();
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
  unsigned int num_monitors = monitors.size();
  unsigned int num_panels = num_monitors;
  unsigned int panels_size = panels_.size();
  unsigned int last_panel = 0;

  tray_xids_.resize(num_panels);

  for (unsigned int i = 0; i < num_panels; ++i, ++last_panel)
  {
    if (i >= panels_size)
    {
      panels_.push_back(CreatePanel(iobj));
    }
    else if (!panels_[i])
    {
      panels_[i] = CreatePanel(iobj);
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
      iobj->RemoveChild(panel.GetPointer());
      panel->GetParent()->UnReference();
      edge_barriers_->RemoveHorizontalSubscriber(panel.GetPointer(), panel->GetMonitor());
    }
  }

  panels_.resize(num_panels);
  UpdatePanelGeometries();
}


nux::ObjectPtr<PanelView> Controller::Impl::CreatePanel(Introspectable *iobj)
{
  auto* panel_window = new MockableBaseWindow(TEXT("PanelWindow"));

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  PanelView* view = new PanelView(panel_window, dbus_indicators_);
  view->SetMaximumHeight(panel::Style::Instance().panel_height);
  view->SetOpacity(opacity_);
  view->SetOpacityMaximizedToggle(opacity_maximized_toggle_);
  view->SetMenuShowTimings(menus_fadein_, menus_fadeout_, menus_discovery_,
                         menus_discovery_fadein_, menus_discovery_fadeout_);

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
  iobj->AddChild(view);

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

Controller::Controller(ui::EdgeBarrierController::Ptr const& edge_barriers, GnomeKeyGrabber::Ptr const& grabber)
  : launcher_width(64)
  , pimpl(new Impl(edge_barriers, grabber))
{
  UScreen* screen = UScreen::GetDefault();
  screen->changed.connect(sigc::mem_fun(this, &Controller::OnScreenChanged));
  OnScreenChanged(screen->GetPrimaryMonitor(), screen->GetMonitors());

  launcher_width.changed.connect([this] (int width)
  {
    pimpl->SetLauncherWidth(width);
  });
}

Controller::Controller(ui::EdgeBarrierController::Ptr const& edge_barriers)
  : Controller(edge_barriers, GnomeKeyGrabber::Ptr())
{
}

Controller::~Controller()
{
  delete pimpl;
}

void Controller::ShowMenuBar()
{
  pimpl->SetMenuBarVisible(true);
}

void Controller::HideMenuBar()
{
  pimpl->SetMenuBarVisible(false);
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

void Controller::OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors)
{
  pimpl->OnScreenChanged(primary_monitor, monitors, this);
}

} // namespace panel
} // namespace unity
