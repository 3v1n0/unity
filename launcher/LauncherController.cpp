// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <NuxCore/Logger.h>
#include <UnityCore/DesktopUtilities.h>

#include "LauncherOptions.h"
#include "ApplicationLauncherIcon.h"
#include "DesktopLauncherIcon.h"
#include "VolumeLauncherIcon.h"
#include "FavoriteStore.h"
#include "HudLauncherIcon.h"
#include "LauncherController.h"
#include "LauncherControllerPrivate.h"
#include "SoftwareCenterLauncherIcon.h"
#include "ExpoLauncherIcon.h"
#include "TrashLauncherIcon.h"
#include "BFBLauncherIcon.h"
#include "unity-shared/IconRenderer.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/TimeUtil.h"
#include "unity-shared/PanelStyle.h"

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher.controller");
namespace
{
const std::string DBUS_NAME = "com.canonical.Unity.Launcher";
const std::string DBUS_PATH = "/com/canonical/Unity/Launcher";
const std::string DBUS_INTROSPECTION =
  "<node>"
  "  <interface name='com.canonical.Unity.Launcher'>"
  ""
  "    <method name='AddLauncherItemFromPosition'>"
  "      <arg type='s' name='title' direction='in'/>"
  "      <arg type='s' name='icon' direction='in'/>"
  "      <arg type='i' name='icon_x' direction='in'/>"
  "      <arg type='i' name='icon_y' direction='in'/>"
  "      <arg type='i' name='icon_size' direction='in'/>"
  "      <arg type='s' name='desktop_file' direction='in'/>"
  "      <arg type='s' name='aptdaemon_task' direction='in'/>"
  "    </method>"
  ""
  "    <method name='UpdateLauncherIconFavoriteState'>"
  "      <arg type='s' name='icon_uri' direction='in'/>"
  "      <arg type='b' name='is_sticky' direction='in'/>"
  "    </method>"
  ""
  "  </interface>"
  "</node>";
}

namespace local
{
namespace
{
  const int launcher_minimum_show_duration = 1250;
  const int shortcuts_show_delay = 750;

  const std::string KEYPRESS_TIMEOUT = "keypress-timeout";
  const std::string LABELS_TIMEOUT = "label-show-timeout";
  const std::string HIDE_TIMEOUT = "hide-timeout";

  const std::string SOFTWARE_CENTER_AGENT = "software-center-agent";

  const std::string RUNNING_APPS_URI = FavoriteStore::URI_PREFIX_UNITY + "running-apps";
  const std::string DEVICES_URI = FavoriteStore::URI_PREFIX_UNITY + "devices";
}

std::string CreateAppUriNameFromDesktopPath(const std::string &desktop_path)
{
  if (desktop_path.empty())
    return "";

  return FavoriteStore::URI_PREFIX_APP + DesktopUtilities::GetDesktopID(desktop_path);
}

}

Controller::Impl::Impl(Controller* parent, XdndManager::Ptr const& xdnd_manager, ui::EdgeBarrierController::Ptr const& edge_barriers)
  : parent_(parent)
  , model_(std::make_shared<LauncherModel>())
  , xdnd_manager_(xdnd_manager)
  , expo_icon_(new ExpoLauncherIcon())
  , desktop_icon_(new DesktopLauncherIcon())
  , edge_barriers_(edge_barriers)
  , sort_priority_(AbstractLauncherIcon::DefaultPriority(AbstractLauncherIcon::IconType::APPLICATION))
  , launcher_open(false)
  , launcher_keynav(false)
  , launcher_grabbed(false)
  , reactivate_keynav(false)
  , keynav_restore_window_(true)
  , launcher_key_press_time_(0)
  , dbus_server_(DBUS_NAME)
{
#ifdef USE_X11
  edge_barriers_->options = parent_->options();
#endif

  UScreen* uscreen = UScreen::GetDefault();
  EnsureLaunchers(uscreen->GetPrimaryMonitor(), uscreen->GetMonitors());

  SetupIcons();

  remote_model_.entry_added.connect(sigc::mem_fun(this, &Impl::OnLauncherEntryRemoteAdded));
  remote_model_.entry_removed.connect(sigc::mem_fun(this, &Impl::OnLauncherEntryRemoteRemoved));

  LauncherHideMode hide_mode = parent_->options()->hide_mode;
  BFBLauncherIcon* bfb = new BFBLauncherIcon(hide_mode);
  RegisterIcon(AbstractLauncherIcon::Ptr(bfb));

  HudLauncherIcon* hud = new HudLauncherIcon(hide_mode);
  RegisterIcon(AbstractLauncherIcon::Ptr(hud));

  TrashLauncherIcon* trash = new TrashLauncherIcon();
  RegisterIcon(AbstractLauncherIcon::Ptr(trash));

  parent_->options()->hide_mode.changed.connect([bfb, hud](LauncherHideMode mode) {
    bfb->SetHideMode(mode);
    hud->SetHideMode(mode);
  });

  uscreen->changed.connect(sigc::mem_fun(this, &Controller::Impl::OnScreenChanged));

  WindowManager& wm = WindowManager::Default();
  wm.window_focus_changed.connect(sigc::mem_fun(this, &Controller::Impl::OnWindowFocusChanged));
  wm.viewport_layout_changed.connect(sigc::group(sigc::mem_fun(this, &Controller::Impl::UpdateNumWorkspaces), sigc::_1 * sigc::_2));
  average_color_connection_ = wm.average_color.changed.connect([this] (nux::Color const& color) {
    parent_->options()->background_color = color;
  });

  ubus.RegisterInterest(UBUS_QUICKLIST_END_KEY_NAV, [this](GVariant * args) {
    if (reactivate_keynav)
      parent_->KeyNavGrab();

    model_->SetSelection(reactivate_index);
    AbstractLauncherIcon::Ptr const& selected = model_->Selection();

    if (selected)
    {
      ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED, glib::Variant(selected->tooltip_text()));
    }
  });

  panel::Style::Instance().panel_height_changed.connect([this, uscreen] (int height)
  {
    for (auto const& launcher_ptr : launchers)
    {
      if (launcher_ptr)
      {
        nux::Geometry const& parent_geo = launcher_ptr->GetParent()->GetGeometry();
        int diff = height - parent_geo.y;
        launcher_ptr->Resize(nux::Point(parent_geo.x, parent_geo.y + diff), parent_geo.height - diff);
      }
    }
  });

  parent_->AddChild(model_.get());

  xdnd_manager_->dnd_started.connect(sigc::mem_fun(this, &Impl::OnDndStarted));
  xdnd_manager_->dnd_finished.connect(sigc::mem_fun(this, &Impl::OnDndFinished));
  xdnd_manager_->monitor_changed.connect(sigc::mem_fun(this, &Impl::OnDndMonitorChanged));

  dbus_server_.AddObjects(DBUS_INTROSPECTION, DBUS_PATH);
  for (auto const& obj : dbus_server_.GetObjects())
    obj->SetMethodsCallsHandler(sigc::mem_fun(this, &Impl::OnDBusMethodCall));
}

Controller::Impl::~Impl()
{
  // Since the launchers are in a window which adds a reference to the
  // launcher, we need to make sure the base windows are unreferenced
  // otherwise the launchers never die.
  for (auto const& launcher_ptr : launchers)
  {
    if (launcher_ptr)
      launcher_ptr->GetParent()->UnReference();
  }
}

void Controller::Impl::EnsureLaunchers(int primary, std::vector<nux::Geometry> const& monitors)
{
  unsigned int num_monitors = monitors.size();
  unsigned int num_launchers = parent_->multiple_launchers ? num_monitors : 1;
  unsigned int launchers_size = launchers.size();
  unsigned int last_launcher = 0;

  // Reset the icon centers: only the used icon centers must contain valid values
  for (auto const& icon : *model_)
    icon->ResetCenters();

  for (unsigned int i = 0; i < num_launchers; ++i, ++last_launcher)
  {
    if (i >= launchers_size)
    {
      launchers.push_back(nux::ObjectPtr<Launcher>(CreateLauncher()));
    }
    else if (!launchers[i])
    {
      launchers[i] = nux::ObjectPtr<Launcher>(CreateLauncher());
    }

    int monitor = (num_launchers == 1 && num_monitors > 1) ? primary : i;

    if (launchers[i]->monitor() != monitor)
    {
#ifdef USE_X11
      edge_barriers_->RemoveVerticalSubscriber(launchers[i].GetPointer(), launchers[i]->monitor);
#endif
      launchers[i]->monitor = monitor;
    }
    else
    {
      launchers[i]->monitor.changed(monitor);
    }

#ifdef USE_X11
    edge_barriers_->AddVerticalSubscriber(launchers[i].GetPointer(), launchers[i]->monitor);
#endif
  }

  for (unsigned int i = last_launcher; i < launchers_size; ++i)
  {
    auto const& launcher = launchers[i];
    if (launcher)
    {
      parent_->RemoveChild(launcher.GetPointer());
      launcher->GetParent()->UnReference();
#ifdef USE_X11
      edge_barriers_->RemoveVerticalSubscriber(launcher.GetPointer(), launcher->monitor);
#endif
    }
  }

  launcher_ = launchers[0];
  launchers.resize(num_launchers);
}

void Controller::Impl::OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors)
{
  EnsureLaunchers(primary_monitor, monitors);
}

void Controller::Impl::OnWindowFocusChanged(guint32 xid)
{
  static bool keynav_first_focus = false;

  if (parent_->IsOverlayOpen() || launcher_->GetParent()->GetInputWindowId() == xid)
    keynav_first_focus = false;

  if (keynav_first_focus)
  {
    keynav_first_focus = false;
    keynav_restore_window_ = false;
    parent_->KeyNavTerminate(false);
  }
  else if (launcher_keynav)
  {
    keynav_first_focus = true;
  }
}

void Controller::Impl::OnDndStarted(std::string const& data, int monitor)
{
  if (parent_->multiple_launchers)
  {
    launchers[monitor]->DndStarted(data);
  }
  else
  {
    launcher_->DndStarted(data);
  }
}

void Controller::Impl::OnDndFinished()
{
  if (parent_->multiple_launchers)
  {
    if (xdnd_manager_->Monitor() >= 0)
      launchers[xdnd_manager_->Monitor()]->DndFinished();
  }
  else
  {
    launcher_->DndFinished();
  }
}

void Controller::Impl::OnDndMonitorChanged(std::string const& data, int old_monitor, int new_monitor)
{
  if (parent_->multiple_launchers)
  {
    if (old_monitor >= 0)
      launchers[old_monitor]->UnsetDndQuirk();

    launchers[new_monitor]->DndStarted(data);
  }
}

Launcher* Controller::Impl::CreateLauncher()
{
  auto* launcher_window = new MockableBaseWindow(TEXT("LauncherWindow"));

  Launcher* launcher = new Launcher(launcher_window);
  launcher->options = parent_->options();
  launcher->SetModel(model_);

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->AddView(launcher, 1);
  layout->SetContentDistribution(nux::MAJOR_POSITION_START);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  launcher_window->SetLayout(layout);
  launcher_window->SetBackgroundColor(nux::color::Transparent);
  launcher_window->ShowWindow(true);

  if (nux::GetWindowThread()->IsEmbeddedWindow())
    launcher_window->EnableInputWindow(true, launcher::window_title, false, false);

  launcher_window->InputWindowEnableStruts(parent_->options()->hide_mode == LAUNCHER_HIDE_NEVER);
  launcher_window->SetEnterFocusInputArea(launcher);

  launcher->add_request.connect(sigc::mem_fun(this, &Impl::OnLauncherAddRequest));
  launcher->remove_request.connect(sigc::mem_fun(this, &Impl::OnLauncherRemoveRequest));

  parent_->AddChild(launcher);

  return launcher;
}

void Controller::Impl::OnLauncherAddRequest(std::string const& icon_uri, AbstractLauncherIcon::Ptr const& icon_before)
{
  std::string app_uri;

  if (icon_uri.find(FavoriteStore::URI_PREFIX_FILE) == 0)
  {
    auto const& desktop_path = icon_uri.substr(FavoriteStore::URI_PREFIX_FILE.length());
    app_uri = local::CreateAppUriNameFromDesktopPath(desktop_path);
  }

  auto const& icon = GetIconByUri(app_uri.empty() ? icon_uri : app_uri);

  if (icon)
  {
    model_->ReorderAfter(icon, icon_before);
    icon->Stick(true);
  }
  else
  {
    if (icon_before)
      RegisterIcon(CreateFavoriteIcon(icon_uri, true), icon_before->SortPriority());
    else
      RegisterIcon(CreateFavoriteIcon(icon_uri, true));

    SaveIconsOrder();
  }
}

void Controller::Impl::AddFavoriteKeepingOldPosition(FavoriteList& icons, std::string const& icon_uri) const
{
  auto const& favorites = FavoriteStore::Instance().GetFavorites();
  auto it = std::find(favorites.rbegin(), favorites.rend(), icon_uri);

  FavoriteList::reverse_iterator icons_it = icons.rbegin();

  while (it != favorites.rend())
  {
    icons_it = std::find(icons.rbegin(), icons.rend(), *it);

    if (icons_it != icons.rend())
      break;

    ++it;
  }

  icons.insert(icons_it.base(), icon_uri);
}

void Controller::Impl::SaveIconsOrder()
{
  FavoriteList icons;
  bool found_first_running_app = false;
  bool found_first_device = false;

  for (auto const& icon : *model_)
  {
    if (!icon->IsSticky())
    {
      if (!icon->IsVisible())
        continue;

      if (!found_first_running_app && icon->GetIconType() == AbstractLauncherIcon::IconType::APPLICATION)
      {
        found_first_running_app = true;
        icons.push_back(local::RUNNING_APPS_URI);
      }

      if (!found_first_device && icon->GetIconType() == AbstractLauncherIcon::IconType::DEVICE)
      {
        found_first_device = true;
        icons.push_back(local::DEVICES_URI);
      }

      continue;
    }

    std::string const& remote_uri = icon->RemoteUri();

    if (!remote_uri.empty())
      icons.push_back(remote_uri);
  }

  if (!found_first_running_app)
    AddFavoriteKeepingOldPosition(icons, local::RUNNING_APPS_URI);

  if (!found_first_device)
    AddFavoriteKeepingOldPosition(icons, local::DEVICES_URI);

  FavoriteStore::Instance().SetFavorites(icons);
}

void
Controller::Impl::OnLauncherUpdateIconStickyState(std::string const& icon_uri, bool sticky)
{
  if (icon_uri.empty())
    return;

  std::string target_uri = icon_uri;
  if (icon_uri.find(FavoriteStore::URI_PREFIX_FILE) == 0)
  {
    auto const& desktop_path =
      icon_uri.substr(FavoriteStore::URI_PREFIX_FILE.length());

    // app uri instead
    target_uri = local::CreateAppUriNameFromDesktopPath(desktop_path);
  }
  auto const& existing_icon_entry =
    GetIconByUri(target_uri);

  if (existing_icon_entry)
    {
      // use the backgroung mechanism of model updates & propagation
      bool should_update = (existing_icon_entry->IsSticky() != sticky);
      if (should_update)
        {
          if (sticky)
          {
            existing_icon_entry->Stick(true);
          }
          else
          {
            existing_icon_entry->UnStick();
          }
        }
    }
    else
    {
      FavoriteStore& favorite_store = FavoriteStore::Instance();

      bool should_update = (favorite_store.IsFavorite(target_uri) != sticky);
      if (should_update)
        {
          if (sticky)
            {
              auto prio = GetLastIconPriority<ApplicationLauncherIcon>("", true);
              RegisterIcon(CreateFavoriteIcon(target_uri, true), prio);
              SaveIconsOrder();
            }
          else
            {
              favorite_store.RemoveFavorite(target_uri);
            }
        }
    }
}

void
Controller::Impl::OnLauncherAddRequestSpecial(std::string const& path,
                                              std::string const& aptdaemon_trans_id,
                                              std::string const& icon_path,
                                              int icon_x,
                                              int icon_y,
                                              int icon_size)
{
  // Check if desktop file was supplied, or if it's set to SC's agent
  // See https://bugs.launchpad.net/unity/+bug/1002440
  if (path.empty() || path == local::SOFTWARE_CENTER_AGENT)
    return;

  auto const& icon = std::find_if(model_->begin(), model_->end(),
    [&path](AbstractLauncherIcon::Ptr const& i) { return (i->DesktopFile() == path); });

  if (icon != model_->end())
    return;

  auto const& result = CreateSCLauncherIcon(path, aptdaemon_trans_id, icon_path);

  if (result)
  {
    // Setting the icon position and adding it to the model, makes the launcher
    // to compute its center
    RegisterIcon(result, GetLastIconPriority<ApplicationLauncherIcon>("", true));

    if (icon_x > 0 || icon_y > 0)
    {
      // This will ensure that the center of the new icon is set, so that
      // the animation could be done properly.
      sources_.AddIdle([this, icon_x, icon_y, result] {
        return !result->Animate(CurrentLauncher(), icon_x, icon_y);
      });
    }
    else
    {
      result->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true);
    }
  }
}

void Controller::Impl::SortAndUpdate()
{
  unsigned shortcut = 1;

  for (auto const& icon : model_->GetSublist<ApplicationLauncherIcon>())
  {
    if (shortcut <= 10 && icon->IsVisible())
    {
      icon->SetShortcut(std::to_string(shortcut % 10)[0]);
      ++shortcut;
    }
    else
    {
      // reset shortcut
      icon->SetShortcut(0);
    }
  }
}

void Controller::Impl::OnIconRemoved(AbstractLauncherIcon::Ptr const& icon)
{
  SortAndUpdate();
}

void Controller::Impl::OnLauncherRemoveRequest(AbstractLauncherIcon::Ptr const& icon)
{
  icon->AboutToRemove();
}

void Controller::Impl::OnLauncherEntryRemoteAdded(LauncherEntryRemote::Ptr const& entry)
{
  if (entry->AppUri().empty())
    return;

  auto const& apps_icons = model_->GetSublist<ApplicationLauncherIcon>();
  auto const& icon = std::find_if(apps_icons.begin(), apps_icons.end(),
    [&entry](AbstractLauncherIcon::Ptr const& i) { return (i->RemoteUri() == entry->AppUri()); });

  if (icon != apps_icons.end())
    (*icon)->InsertEntryRemote(entry);
}

void Controller::Impl::OnLauncherEntryRemoteRemoved(LauncherEntryRemote::Ptr const& entry)
{
  for (auto const& icon : *model_)
    icon->RemoveEntryRemote(entry);
}

void Controller::Impl::OnFavoriteStoreFavoriteAdded(std::string const& entry, std::string const& pos, bool before)
{
  if (entry == local::RUNNING_APPS_URI || entry == local::DEVICES_URI)
  {
    // Since the running apps and the devices are always shown, when added to
    // the model, we only have to re-order them
    ResetIconPriorities();
    return;
  }

  AbstractLauncherIcon::Ptr other = *(model_->begin());

  if (!pos.empty())
  {
    for (auto const& it : *model_)
    {
      if (it->IsVisible() && pos == it->RemoteUri())
        other = it;
    }
  }

  AbstractLauncherIcon::Ptr const& fav = GetIconByUri(entry);
  if (fav)
  {
    fav->Stick(false);

    if (before)
      model_->ReorderBefore(fav, other, false);
    else
      model_->ReorderAfter(fav, other);
  }
  else
  {
    AbstractLauncherIcon::Ptr const& result = CreateFavoriteIcon(entry);
    RegisterIcon(result);

    if (before)
      model_->ReorderBefore(result, other, false);
    else
      model_->ReorderAfter(result, other);
  }

  SortAndUpdate();
}

void Controller::Impl::OnFavoriteStoreFavoriteRemoved(std::string const& entry)
{
  if (entry == local::RUNNING_APPS_URI || entry == local::DEVICES_URI)
  {
    // Since the running apps and the devices are always shown, when added to
    // the model, we only have to re-order them
    ResetIconPriorities();
    SaveIconsOrder();
    return;
  }

  auto const& icon = GetIconByUri(entry);
  if (icon)
  {
    icon->UnStick();

    // When devices are removed from favorites, they should be re-ordered (not removed)
    if (icon->GetIconType() == AbstractLauncherIcon::IconType::DEVICE)
      ResetIconPriorities();
  }
}

void Controller::Impl::ResetIconPriorities()
{
  FavoriteList const& favs = FavoriteStore::Instance().GetFavorites();
  auto const& apps_icons = model_->GetSublist<ApplicationLauncherIcon>();
  auto const& volumes_icons = model_->GetSublist<VolumeLauncherIcon>();
  bool running_apps_found = false;
  bool volumes_found = false;

  for (auto const& fav : favs)
  {
    if (fav == local::RUNNING_APPS_URI)
    {
      for (auto const& ico : apps_icons)
      {
        if (!ico->IsSticky())
          ico->SetSortPriority(++sort_priority_);
      }

      running_apps_found = true;
      continue;
    }
    else if (fav == local::DEVICES_URI)
    {
      for (auto const& ico : volumes_icons)
      {
        if (!ico->IsSticky())
          ico->SetSortPriority(++sort_priority_);
      }

      volumes_found = true;
      continue;
    }

    auto const& icon = GetIconByUri(fav);

    if (icon)
      icon->SetSortPriority(++sort_priority_);
  }

  if (!running_apps_found)
  {
    for (auto const& ico : apps_icons)
    {
      if (!ico->IsSticky())
        ico->SetSortPriority(++sort_priority_);
    }
  }

  if (!volumes_found)
  {
    for (auto const& ico : volumes_icons)
    {
      if (!ico->IsSticky())
        ico->SetSortPriority(++sort_priority_);
    }
  }

  model_->Sort();
}

void Controller::Impl::UpdateNumWorkspaces(int workspaces)
{
  bool visible = expo_icon_->IsVisible();
  bool wp_enabled = (workspaces > 1);

  if (wp_enabled && !visible)
  {
    if (FavoriteStore::Instance().IsFavorite(expo_icon_->RemoteUri()))
    {
      expo_icon_->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true);
    }
  }
  else if (!wp_enabled && visible)
  {
    expo_icon_->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, false);
  }
}

void Controller::Impl::RegisterIcon(AbstractLauncherIcon::Ptr const& icon, int priority)
{
  if (!icon)
    return;

  std::string const& icon_uri = icon->RemoteUri();

  if (model_->IconIndex(icon) >= 0)
  {
    if (!icon_uri.empty())
    {
      LOG_ERROR(logger) << "Impossible to add icon '" << icon_uri
                        << "': it's already on the launcher!";
    }

    return;
  }

  if (priority != std::numeric_limits<int>::min())
    icon->SetSortPriority(priority);

  icon->position_saved.connect([this] {
    // These calls must be done in order: first we save the new sticky icons
    // then we re-order the model so that there won't be two icons with the same
    // priority
    SaveIconsOrder();
    ResetIconPriorities();
  });

  auto uri_ptr = std::make_shared<std::string>(icon_uri);
  icon->position_forgot.connect([this, uri_ptr] {
    FavoriteStore::Instance().RemoveFavorite(*uri_ptr);
  });

  icon->uri_changed.connect([this, uri_ptr] (std::string const& new_uri) {
    *uri_ptr = new_uri;
  });

  if (icon->GetIconType() == AbstractLauncherIcon::IconType::APPLICATION)
  {
    icon->visibility_changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::SortAndUpdate)));
    SortAndUpdate();
  }

  model_->AddIcon(icon);

  std::string const& path = icon->DesktopFile();

  if (!path.empty())
  {
    LauncherEntryRemote::Ptr const& entry = remote_model_.LookupByDesktopFile(path);

    if (entry)
      icon->InsertEntryRemote(entry);
  }
}

template<typename IconType>
int Controller::Impl::GetLastIconPriority(std::string const& favorite_uri, bool sticky)
{
  auto const& icons = model_->GetSublist<IconType>();
  int icon_prio = std::numeric_limits<int>::min();

  AbstractLauncherIcon::Ptr last_icon;

  // Get the last (non)-sticky icon position (if available)
  for (auto it = icons.rbegin(); it != icons.rend(); ++it)
  {
    auto const& icon = *it;
    bool update_last_icon = ((!last_icon && !sticky) || sticky);

    if (update_last_icon || icon->IsSticky() == sticky)
    {
      last_icon = icon;

      if (icon->IsSticky() == sticky)
        break;
    }
  }

  if (last_icon)
  {
    icon_prio = last_icon->SortPriority();

    if (sticky && last_icon->IsSticky() != sticky)
      icon_prio -= 1;
  }
  else if (!favorite_uri.empty())
  {
    // If we have no applications opened, we must guess it position by favorites
    for (auto const& fav : FavoriteStore::Instance().GetFavorites())
    {
      if (fav == favorite_uri)
      {
        if (icon_prio == std::numeric_limits<int>::min())
          icon_prio = (*model_->begin())->SortPriority() - 1;

        break;
      }

      auto const& icon = GetIconByUri(fav);

      if (icon)
        icon_prio = icon->SortPriority();
    }
  }

  return icon_prio;
}

void Controller::Impl::OnApplicationStarted(ApplicationPtr const& app)
{
  if (app->sticky() || app->seen())
    return;

  AbstractLauncherIcon::Ptr icon(new ApplicationLauncherIcon(app));
  RegisterIcon(icon, GetLastIconPriority<ApplicationLauncherIcon>(local::RUNNING_APPS_URI));
}

void Controller::Impl::OnDeviceIconAdded(AbstractLauncherIcon::Ptr const& icon)
{
  RegisterIcon(icon, GetLastIconPriority<VolumeLauncherIcon>(local::DEVICES_URI));
}

AbstractLauncherIcon::Ptr Controller::Impl::CreateFavoriteIcon(std::string const& icon_uri, bool emit_signal)
{
  AbstractLauncherIcon::Ptr result;

  if (!FavoriteStore::IsValidFavoriteUri(icon_uri))
  {
    LOG_WARNING(logger) << "Ignoring favorite '" << icon_uri << "'.";
    return result;
  }

  std::string desktop_id;

  if (icon_uri.find(FavoriteStore::URI_PREFIX_APP) == 0)
  {
    desktop_id = icon_uri.substr(FavoriteStore::URI_PREFIX_APP.size());
  }
  else if (icon_uri.find(FavoriteStore::URI_PREFIX_FILE) == 0)
  {
    desktop_id = icon_uri.substr(FavoriteStore::URI_PREFIX_FILE.size());
  }

  if (!desktop_id.empty())
  {
    std::string const& desktop_path = DesktopUtilities::GetDesktopPathById(desktop_id);
    ApplicationPtr app = ApplicationManager::Default().GetApplicationForDesktopFile(desktop_path);
    if (!app || app->seen())
      return result;

    result = AbstractLauncherIcon::Ptr(new ApplicationLauncherIcon(app));
  }
  else if (icon_uri.find(FavoriteStore::URI_PREFIX_DEVICE) == 0)
  {
    auto const& devices = device_section_.GetIcons();
    auto const& icon = std::find_if(devices.begin(), devices.end(),
      [&icon_uri](AbstractLauncherIcon::Ptr const& i) { return (i->RemoteUri() == icon_uri); });

    if (icon == devices.end())
    {
      // Using an idle to remove the favorite, not to erase while iterating
      sources_.AddIdle([this, icon_uri] {
        FavoriteStore::Instance().RemoveFavorite(icon_uri);
        return false;
      });

      return result;
    }

    result = *icon;
  }
  else if (desktop_icon_->RemoteUri() == icon_uri)
  {
    result = desktop_icon_;
  }
  else if (expo_icon_->RemoteUri() == icon_uri)
  {
    result = expo_icon_;
  }

  if (result)
    result->Stick(emit_signal);

  return result;
}

AbstractLauncherIcon::Ptr Controller::Impl::GetIconByUri(std::string const& icon_uri)
{
  if (icon_uri.empty())
    return AbstractLauncherIcon::Ptr();

  auto const& icon = std::find_if(model_->begin(), model_->end(),
    [&icon_uri](AbstractLauncherIcon::Ptr const& i) { return (i->RemoteUri() == icon_uri); });

  if (icon != model_->end())
  {
    return *icon;
  }

  return AbstractLauncherIcon::Ptr();
}

SoftwareCenterLauncherIcon::Ptr Controller::Impl::CreateSCLauncherIcon(std::string const& file_path,
                                                                       std::string const& aptdaemon_trans_id,
                                                                       std::string const& icon_path)
{
  SoftwareCenterLauncherIcon::Ptr result;

  ApplicationPtr app = ApplicationManager::Default().GetApplicationForDesktopFile(file_path);
  if (!app)
    return result;

  if (app->seen)
    return result;

  result = new SoftwareCenterLauncherIcon(app, aptdaemon_trans_id, icon_path);

  return result;
}

void Controller::Impl::AddRunningApps()
{
  for (auto& app : ApplicationManager::Default().GetRunningApplications())
  {
    LOG_INFO(logger) << "Adding running app: " << app->title()
                     << ", seen already: "
                     << (app->seen() ? "yes" : "no");
    if (!app->seen())
    {
      AbstractLauncherIcon::Ptr icon(new ApplicationLauncherIcon(app));
      icon->SkipQuirkAnimation(AbstractLauncherIcon::Quirk::VISIBLE);
      RegisterIcon(icon, ++sort_priority_);
    }
  }
}

void Controller::Impl::AddDevices()
{
  auto& fav_store = FavoriteStore::Instance();
  for (auto const& icon : device_section_.GetIcons())
  {
    if (!icon->IsSticky() && !fav_store.IsFavorite(icon->RemoteUri()))
    {
      icon->SkipQuirkAnimation(AbstractLauncherIcon::Quirk::VISIBLE);
      RegisterIcon(icon, ++sort_priority_);
    }
  }
}

void Controller::Impl::MigrateFavorites()
{
  // This migrates favorites to new format, ensuring that upgrades won't lose anything
  auto& favorites = FavoriteStore::Instance();
  auto const& favs = favorites.GetFavorites();

  auto fav_it = std::find_if(begin(favs), end(favs), [](std::string const& fav) {
    return (fav.find(FavoriteStore::URI_PREFIX_UNITY) != std::string::npos);
  });

  if (fav_it == end(favs))
  {
    favorites.AddFavorite(local::RUNNING_APPS_URI, -1);
    favorites.AddFavorite(expo_icon_->RemoteUri(), -1);
    favorites.AddFavorite(local::DEVICES_URI, -1);
  }
}

void Controller::Impl::SetupIcons()
{
  MigrateFavorites();

  auto& favorite_store = FavoriteStore::Instance();
  FavoriteList const& favs = favorite_store.GetFavorites();
  bool running_apps_added = false;
  bool devices_added = false;

  for (auto const& fav_uri : favs)
  {
    if (fav_uri == local::RUNNING_APPS_URI)
    {
      LOG_INFO(logger) << "Adding running apps";
      AddRunningApps();
      running_apps_added = true;
      continue;
    }
    else if (fav_uri == local::DEVICES_URI)
    {
      LOG_INFO(logger) << "Adding devices";
      AddDevices();
      devices_added = true;
      continue;
    }

    LOG_INFO(logger) << "Adding favourite: " << fav_uri;

    auto const& icon = CreateFavoriteIcon(fav_uri);

    if (icon)
    {
      icon->SkipQuirkAnimation(AbstractLauncherIcon::Quirk::VISIBLE);
      RegisterIcon(icon, ++sort_priority_);
    }
  }

  if (!running_apps_added)
  {
    LOG_INFO(logger) << "Adding running apps";
    AddRunningApps();
  }

  if (!devices_added)
  {
    LOG_INFO(logger) << "Adding devices";
    AddDevices();
  }

  ApplicationManager::Default().application_started
    .connect(sigc::mem_fun(this, &Impl::OnApplicationStarted));

  device_section_.icon_added.connect(sigc::mem_fun(this, &Impl::OnDeviceIconAdded));
  favorite_store.favorite_added.connect(sigc::mem_fun(this, &Impl::OnFavoriteStoreFavoriteAdded));
  favorite_store.favorite_removed.connect(sigc::mem_fun(this, &Impl::OnFavoriteStoreFavoriteRemoved));
  favorite_store.reordered.connect(sigc::mem_fun(this, &Impl::ResetIconPriorities));

  model_->order_changed.connect(sigc::mem_fun(this, &Impl::SortAndUpdate));
  model_->icon_removed.connect(sigc::mem_fun(this, &Impl::OnIconRemoved));
  model_->saved.connect(sigc::mem_fun(this, &Impl::SaveIconsOrder));
}

void Controller::Impl::SendHomeActivationRequest()
{
  ubus.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                   g_variant_new("(sus)", "home.scope", dash::NOT_HANDLED, ""));
}

Controller::Controller(XdndManager::Ptr const& xdnd_manager, ui::EdgeBarrierController::Ptr const& edge_barriers)
 : options(std::make_shared<Options>())
 , multiple_launchers(true)
 , pimpl(new Impl(this, xdnd_manager, edge_barriers))
{
  multiple_launchers.changed.connect([this] (bool value) {
    UScreen* uscreen = UScreen::GetDefault();
    auto monitors = uscreen->GetMonitors();
    int primary = uscreen->GetPrimaryMonitor();
    pimpl->EnsureLaunchers(primary, monitors);
    options()->show_for_all = !value;
  });
}

Controller::~Controller()
{}

void Controller::UpdateNumWorkspaces(int workspaces)
{
  pimpl->UpdateNumWorkspaces(workspaces);
}

Launcher& Controller::launcher() const
{
  return *(pimpl->launcher_);
}

Controller::LauncherList& Controller::launchers() const
{
  return pimpl->launchers;
}

std::vector<char> Controller::GetAllShortcuts() const
{
  std::vector<char> shortcuts;
  for (auto icon : *(pimpl->model_))
  {
    // TODO: find out why the icons use guint64 for shortcuts.
    char shortcut = icon->GetShortcut();
    if (shortcut)
      shortcuts.push_back(shortcut);
  }
  return shortcuts;
}

std::vector<AbstractLauncherIcon::Ptr> Controller::GetAltTabIcons(bool current, bool show_desktop_disabled) const
{
  std::vector<AbstractLauncherIcon::Ptr> results;

  if (!show_desktop_disabled)
    results.push_back(pimpl->desktop_icon_);

  for (auto icon : *(pimpl->model_))
  {
    if (icon->ShowInSwitcher(current))
    {
      //otherwise we get two desktop icons in the switcher.
      if (icon->GetIconType() != AbstractLauncherIcon::IconType::DESKTOP)
      {
        results.push_back(icon);
      }
    }
  }
  return results;
}

Window Controller::LauncherWindowId(int launcher) const
{
  if (launcher >= (int)pimpl->launchers.size())
    return 0;
  return pimpl->launchers[launcher]->GetParent()->GetInputWindowId();
}

Window Controller::KeyNavLauncherInputWindowId() const
{
  if (KeyNavIsActive())
    return pimpl->keyboard_launcher_->GetParent()->GetInputWindowId();
  return 0;
}

void Controller::PushToFront()
{
  pimpl->launcher_->GetParent()->PushToFront();
}

int Controller::Impl::MonitorWithMouse()
{
  UScreen* uscreen = UScreen::GetDefault();
  return uscreen->GetMonitorWithMouse();
}

nux::ObjectPtr<Launcher> Controller::Impl::CurrentLauncher()
{
  nux::ObjectPtr<Launcher> result;
  int best = std::min<int> (launchers.size() - 1, MonitorWithMouse());
  if (best >= 0)
    result = launchers[best];
  return result;
}

void Controller::HandleLauncherKeyPress(int when)
{
  pimpl->launcher_key_press_time_ = when;

  auto show_launcher = [this]
  {
    if (pimpl->keyboard_launcher_.IsNull())
      pimpl->keyboard_launcher_ = pimpl->CurrentLauncher();

    pimpl->sources_.Remove(local::HIDE_TIMEOUT);
    pimpl->keyboard_launcher_->ForceReveal(true);
    pimpl->launcher_open = true;

    return false;
  };
  pimpl->sources_.AddTimeout(options()->super_tap_duration, show_launcher, local::KEYPRESS_TIMEOUT);

  auto show_shortcuts = [this]
  {
    if (!pimpl->launcher_keynav)
    {
      if (pimpl->keyboard_launcher_.IsNull())
        pimpl->keyboard_launcher_ = pimpl->CurrentLauncher();

      pimpl->keyboard_launcher_->ShowShortcuts(true);
      pimpl->launcher_open = true;
    }

    return false;
  };
  pimpl->sources_.AddTimeout(local::shortcuts_show_delay, show_shortcuts, local::LABELS_TIMEOUT);
}

bool Controller::AboutToShowDash(int was_tap, int when) const
{
  if ((when - pimpl->launcher_key_press_time_) < options()->super_tap_duration && was_tap)
    return true;
  return false;
}

void Controller::HandleLauncherKeyRelease(bool was_tap, int when)
{
  int tap_duration = when - pimpl->launcher_key_press_time_;
  WindowManager& wm = WindowManager::Default();

  if (tap_duration < options()->super_tap_duration && was_tap &&
      !wm.IsTopWindowFullscreenOnMonitorWithMouse())
  {
    LOG_DEBUG(logger) << "Quick tap, sending activation request.";
    pimpl->SendHomeActivationRequest();
  }
  else
  {
    LOG_DEBUG(logger) << "Tap too long: " << tap_duration;
  }

  pimpl->sources_.Remove(local::LABELS_TIMEOUT);
  pimpl->sources_.Remove(local::KEYPRESS_TIMEOUT);

  if (pimpl->keyboard_launcher_.IsValid())
  {
    pimpl->keyboard_launcher_->ShowShortcuts(false);

    int ms_since_show = tap_duration;
    if (ms_since_show > local::launcher_minimum_show_duration)
    {
      pimpl->keyboard_launcher_->ForceReveal(false);
      pimpl->launcher_open = false;

      if (!pimpl->launcher_keynav)
        pimpl->keyboard_launcher_.Release();
    }
    else
    {
      int time_left = local::launcher_minimum_show_duration - ms_since_show;

      auto hide_launcher = [this]
      {
        if (pimpl->keyboard_launcher_.IsValid())
        {
          pimpl->keyboard_launcher_->ForceReveal(false);
          pimpl->launcher_open = false;

          if (!pimpl->launcher_keynav)
            pimpl->keyboard_launcher_.Release();
        }

        return false;
      };

      pimpl->sources_.AddTimeout(time_left, hide_launcher, local::HIDE_TIMEOUT);
    }
  }
}

bool Controller::HandleLauncherKeyEvent(unsigned long key_state, unsigned int key_sym, Time timestamp)
{
  // Shortcut to start launcher icons. Only relies on Keycode, ignore modifier
  for (auto const& icon : *pimpl->model_)
  {
    if (icon->GetShortcut() == key_sym)
    {
      if ((key_state & nux::KEY_MODIFIER_SHIFT) &&
          icon->GetIconType() == AbstractLauncherIcon::IconType::APPLICATION)
      {
        icon->OpenInstance(ActionArg(ActionArg::Source::LAUNCHER_KEYBINDING, 0, timestamp));
      }
      else
      {
        icon->Activate(ActionArg(ActionArg::Source::LAUNCHER_KEYBINDING, 0, timestamp));
      }

      // disable the "tap on super" check
      pimpl->launcher_key_press_time_ = 0;
      return true;
    }
  }

  return false;
}

void Controller::Impl::ReceiveMouseDownOutsideArea(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (launcher_grabbed)
    parent_->KeyNavTerminate(false);
}

void Controller::KeyNavGrab()
{
  pimpl->launcher_grabbed = true;
  KeyNavActivate();
  pimpl->keyboard_launcher_->GrabKeyboard();

  pimpl->launcher_key_press_connection_ =
    pimpl->keyboard_launcher_->key_down.connect(sigc::mem_fun(pimpl.get(), &Controller::Impl::ReceiveLauncherKeyPress));
  pimpl->launcher_event_outside_connection_ =
    pimpl->keyboard_launcher_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(pimpl.get(), &Controller::Impl::ReceiveMouseDownOutsideArea));
}

void Controller::KeyNavActivate()
{
  if (pimpl->launcher_keynav)
    return;

  pimpl->reactivate_keynav = false;
  pimpl->launcher_keynav = true;
  pimpl->keynav_restore_window_ = true;
  pimpl->keyboard_launcher_ = pimpl->CurrentLauncher();

  pimpl->keyboard_launcher_->EnterKeyNavMode();
  pimpl->model_->SetSelection(0);

  if (pimpl->launcher_grabbed)
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_START_KEY_NAV,
                            glib::Variant(pimpl->keyboard_launcher_->monitor()));
  }
  else
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_START_KEY_SWITCHER,
                            glib::Variant(pimpl->keyboard_launcher_->monitor()));
  }

  AbstractLauncherIcon::Ptr const& selected = pimpl->model_->Selection();

  if (selected)
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                            glib::Variant(selected->tooltip_text()));
  }
}

void Controller::KeyNavNext()
{
  pimpl->model_->SelectNext();

  AbstractLauncherIcon::Ptr const& selected = pimpl->model_->Selection();

  if (selected)
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                            glib::Variant(selected->tooltip_text()));
  }
}

void Controller::KeyNavPrevious()
{
  pimpl->model_->SelectPrevious();

  AbstractLauncherIcon::Ptr const& selected = pimpl->model_->Selection();

  if (selected)
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                            glib::Variant(selected->tooltip_text()));
  }
}

void Controller::KeyNavTerminate(bool activate)
{
  if (!pimpl->launcher_keynav)
    return;

  if (activate && pimpl->keynav_restore_window_)
  {
    /* If the selected icon is running, we must not restore the input to the old */
    AbstractLauncherIcon::Ptr const& icon = pimpl->model_->Selection();
    pimpl->keynav_restore_window_ = !icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING);
  }

  pimpl->keyboard_launcher_->ExitKeyNavMode();

  if (pimpl->launcher_grabbed)
  {
    pimpl->keyboard_launcher_->UnGrabKeyboard();
    pimpl->launcher_key_press_connection_->disconnect();
    pimpl->launcher_event_outside_connection_->disconnect();
    pimpl->launcher_grabbed = false;
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_END_KEY_NAV,
                            glib::Variant(pimpl->keynav_restore_window_));
  }
  else
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_END_KEY_SWITCHER,
                            glib::Variant(pimpl->keynav_restore_window_));
  }

  if (activate)
  {
    auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;

    pimpl->sources_.AddIdle([this, timestamp] {
      pimpl->model_->Selection()->Activate(ActionArg(ActionArg::Source::LAUNCHER, 0, timestamp));
      return false;
    });
  }

  pimpl->launcher_keynav = false;
  if (!pimpl->launcher_open)
    pimpl->keyboard_launcher_.Release();
}

bool Controller::KeyNavIsActive() const
{
  return pimpl->launcher_keynav;
}

bool Controller::IsOverlayOpen() const
{
  for (auto launcher_ptr : pimpl->launchers)
  {
    if (launcher_ptr->IsOverlayOpen())
      return true;
  }
  return false;
}

std::string
Controller::GetName() const
{
  return "LauncherController";
}

void
Controller::AddProperties(debug::IntrospectionData& introspection)
{
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  introspection
  .add("key_nav_is_active", KeyNavIsActive())
  .add("key_nav_launcher_monitor", pimpl->keyboard_launcher_.IsValid() ?  pimpl->keyboard_launcher_->monitor : -1)
  .add("key_nav_selection", pimpl->model_->SelectionIndex())
  .add("key_nav_is_grabbed", pimpl->launcher_grabbed)
  .add("keyboard_launcher", pimpl->CurrentLauncher()->monitor);
}

void Controller::Impl::ReceiveLauncherKeyPress(unsigned long eventType,
                                               unsigned long keysym,
                                               unsigned long state,
                                               const char* character,
                                               unsigned short keyCount)
{
  /*
   * all key events below are related to keynavigation. Make an additional
   * check that we are in a keynav mode when we inadvertadly receive the focus
   */
  if (!launcher_grabbed)
    return;

  switch (keysym)
  {
      // up (move selection up or go to global-menu if at top-most icon)
    case NUX_VK_UP:
    case NUX_KP_UP:
      parent_->KeyNavPrevious();
      break;

      // down (move selection down and unfold launcher if needed)
    case NUX_VK_DOWN:
    case NUX_KP_DOWN:
      parent_->KeyNavNext();
      break;

      // super/control/esc/left (close quicklist or exit laucher key-focus)
    case NUX_VK_LWIN:
    case NUX_VK_RWIN:
    case NUX_VK_CONTROL:
    case NUX_VK_LEFT:
    case NUX_KP_LEFT:
    case NUX_VK_ESCAPE:
      // hide again
      parent_->KeyNavTerminate(false);
      break;

      // right/shift-f10 (open quicklist of currently selected icon)
    case XK_F10:
      if (!(state & nux::NUX_STATE_SHIFT))
        break;
    case NUX_VK_RIGHT:
    case NUX_KP_RIGHT:
    case XK_Menu:
      OpenQuicklist();
      break;

      // <SPACE> (open a new instance)
    case NUX_VK_SPACE:
    {
      auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
      model_->Selection()->OpenInstance(ActionArg(ActionArg::Source::LAUNCHER, 0, timestamp));
      parent_->KeyNavTerminate(false);
      break;
    }
      // <RETURN> (start/activate currently selected icon)
    case NUX_VK_ENTER:
    case NUX_KP_ENTER:
    parent_->KeyNavTerminate(true);
    break;

    default:
      // ALT + <Anykey>; treat it as a shortcut and exit keynav
      if (state & nux::NUX_STATE_ALT)
      {
        parent_->KeyNavTerminate(false);
      }
      break;
  }
}

void Controller::Impl::OpenQuicklist()
{
  if (model_->Selection()->OpenQuicklist(true, keyboard_launcher_->monitor()))
  {
    reactivate_keynav = true;
    reactivate_index = model_->SelectionIndex();
    parent_->KeyNavTerminate(false);
  }
}

GVariant* Controller::Impl::OnDBusMethodCall(std::string const& method, GVariant *parameters)
{
  if (method == "AddLauncherItemFromPosition")
  {
    glib::String icon, icon_title, desktop_file, aptdaemon_task;
    gint icon_x, icon_y, icon_size;

    g_variant_get(parameters, "(ssiiiss)", &icon_title, &icon, &icon_x, &icon_y,
                                           &icon_size, &desktop_file, &aptdaemon_task);

    OnLauncherAddRequestSpecial(desktop_file.Str(), aptdaemon_task.Str(),
                                icon.Str(), icon_x, icon_y, icon_size);
  }
  else if (method == "UpdateLauncherIconFavoriteState")
  {
    gboolean is_sticky;
    glib::String icon_uri;
    g_variant_get(parameters, "(sb)", &icon_uri, &is_sticky);

    OnLauncherUpdateIconStickyState(icon_uri.Str(), is_sticky);
  }

  return nullptr;
}

} // namespace launcher
} // namespace unity
