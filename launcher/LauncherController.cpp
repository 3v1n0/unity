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

#include <glib/gi18n-lib.h>
#include <libbamf/libbamf.h>

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/BaseWindow.h>
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
#include "unity-shared/UScreen.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/TimeUtil.h"

namespace unity
{
namespace launcher
{
namespace
{
nux::logging::Logger logger("unity.launcher");

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
  "  </interface>"
  "</node>";
}

namespace local
{
namespace
{
  const int super_tap_duration = 250;
  const int launcher_minimum_show_duration = 1250;
  const int shortcuts_show_delay = 750;
  const int ignore_repeat_shortcut_duration = 250;

  const std::string KEYPRESS_TIMEOUT = "keypress-timeout";
  const std::string LABELS_TIMEOUT = "label-show-timeout";
  const std::string HIDE_TIMEOUT = "hide-timeout";

  const std::string SOFTWARE_CENTER_AGENT = "software-center-agent";

  const std::string RUNNING_APPS_URI = FavoriteStore::URI_PREFIX_UNITY + "running-apps";
  const std::string DEVICES_URI = FavoriteStore::URI_PREFIX_UNITY + "devices";
}
}

GDBusInterfaceVTable Controller::Impl::interface_vtable =
  { Controller::Impl::OnDBusMethodCall, NULL, NULL};


Controller::Impl::Impl(Controller* parent)
  : parent_(parent)
  , model_(std::make_shared<LauncherModel>())
  , matcher_(bamf_matcher_get_default())
  , device_section_(std::make_shared<VolumeMonitorWrapper>(), std::make_shared<DevicesSettingsImp>())
  , expo_icon_(new ExpoLauncherIcon())
  , desktop_icon_(new DesktopLauncherIcon())
  , sort_priority_(AbstractLauncherIcon::DefaultPriority(AbstractLauncherIcon::IconType::APPLICATION))
  , launcher_open(false)
  , launcher_keynav(false)
  , launcher_grabbed(false)
  , reactivate_keynav(false)
  , keynav_restore_window_(true)
  , launcher_key_press_time_(0)
  , dbus_owner_(g_bus_own_name(G_BUS_TYPE_SESSION, DBUS_NAME.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
                               OnBusAcquired, nullptr, nullptr, this, nullptr))
  , gdbus_connection_(nullptr)
  , reg_id_(0)
{
#ifdef UNITY_HAS_X_ORG_SUPPORT
  edge_barriers_.options = parent_->options();
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

  ubus.RegisterInterest(UBUS_QUICKLIST_END_KEY_NAV, [&](GVariant * args) {
    if (reactivate_keynav)
      parent_->KeyNavGrab();

    model_->SetSelection(reactivate_index);
    AbstractLauncherIcon::Ptr const& selected = model_->Selection();

    if (selected)
    {
      ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                       g_variant_new_string(selected->tooltip_text().c_str()));
    }
  });

  parent_->AddChild(model_.get());
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

  if (gdbus_connection_ && reg_id_)
    g_dbus_connection_unregister_object(gdbus_connection_, reg_id_);   

  g_bus_unown_name(dbus_owner_);
}

void Controller::Impl::EnsureLaunchers(int primary, std::vector<nux::Geometry> const& monitors)
{
  unsigned int num_monitors = monitors.size();
  unsigned int num_launchers = parent_->multiple_launchers ? num_monitors : 1;
  unsigned int launchers_size = launchers.size();
  unsigned int last_launcher = 0;

  for (unsigned int i = 0; i < num_launchers; ++i, ++last_launcher)
  {
    if (i >= launchers_size)
    {
      launchers.push_back(nux::ObjectPtr<Launcher>(CreateLauncher(i)));
    }
    else if (!launchers[i])
    {
      launchers[i] = nux::ObjectPtr<Launcher>(CreateLauncher(i));
    }

    int monitor = (num_launchers == 1) ? primary : i;

#ifdef UNITY_HAS_X_ORG_SUPPORT
    if (launchers[i]->monitor() != monitor)
    {
      edge_barriers_.Unsubscribe(launchers[i].GetPointer(), launchers[i]->monitor);
    }
#endif

    launchers[i]->monitor(monitor);
    launchers[i]->Resize();
#ifdef UNITY_HAS_X_ORG_SUPPORT
    edge_barriers_.Subscribe(launchers[i].GetPointer(), launchers[i]->monitor);
#endif
  }

  for (unsigned int i = last_launcher; i < launchers_size; ++i)
  {
    auto launcher = launchers[i];
    if (launcher)
    {
      parent_->RemoveChild(launcher.GetPointer());
      launcher->GetParent()->UnReference();
#ifdef UNITY_HAS_X_ORG_SUPPORT
      edge_barriers_.Unsubscribe(launcher.GetPointer(), launcher->monitor);
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

Launcher* Controller::Impl::CreateLauncher(int monitor)
{
  nux::BaseWindow* launcher_window = new nux::BaseWindow(TEXT("LauncherWindow"));

  Launcher* launcher = new Launcher(launcher_window, nux::ObjectPtr<DNDCollectionWindow>(new DNDCollectionWindow));
  launcher->monitor = monitor;
  launcher->options = parent_->options();
  launcher->SetModel(model_);

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->AddView(launcher, 1);
  layout->SetContentDistribution(nux::eStackLeft);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  launcher_window->SetLayout(layout);
  launcher_window->SetBackgroundColor(nux::color::Transparent);
  launcher_window->ShowWindow(true);
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
    app_uri = FavoriteStore::URI_PREFIX_APP + DesktopUtilities::GetDesktopID(desktop_path);
  }

  auto const& icon = GetIconByUri(app_uri.empty() ? icon_uri : app_uri);

  if (icon)
  {
    icon->Stick(false);
    model_->ReorderAfter(icon, icon_before);
  }
  else
  {
    if (icon_before)
      RegisterIcon(CreateFavoriteIcon(icon_uri), icon_before->SortPriority());
    else
      RegisterIcon(CreateFavoriteIcon(icon_uri));
  }

  SaveIconsOrder();
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
        result->Animate(CurrentLauncher(), icon_x, icon_y);
        return false;
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
  switch (icon->GetIconType())
  {
    case AbstractLauncherIcon::IconType::APPLICATION:
    {
      ApplicationLauncherIcon* bamf_icon = dynamic_cast<ApplicationLauncherIcon*>(icon.GetPointer());

      if (bamf_icon)
      {
        bamf_icon->UnStick();
        bamf_icon->Quit();
      }

      break;
    }
    case AbstractLauncherIcon::IconType::DEVICE:
    {
      auto device_icon = dynamic_cast<VolumeLauncherIcon*>(icon.GetPointer());

      if (device_icon)
      {
        if (device_icon->CanEject())
          device_icon->EjectAndShowNotification();
        else if (device_icon->CanStop())
          device_icon->StopDrive();
      }

      break;
    }
    default:
      break;
  }
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
  if (entry == local::RUNNING_APPS_URI || entry == local::DEVICES_URI || entry == expo_icon_->RemoteUri())
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
  if (entry == local::RUNNING_APPS_URI || entry == local::DEVICES_URI || entry == expo_icon_->RemoteUri())
  {
    // Since the running apps and the devices are always shown, when added to
    // the model, we only have to re-order them
    ResetIconPriorities();
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
  bool expo_icon_found = false;
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
    else if (fav == expo_icon_->RemoteUri())
    {
      expo_icon_found = true;
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

  if (!expo_icon_found)
    expo_icon_->SetSortPriority(++sort_priority_);

  if (!volumes_found)
  {
    for (auto const& ico : volumes_icons)
    {
      if (!ico->IsSticky())
        ico->SetSortPriority(++sort_priority_);
    }
  }

  model_->Sort();

  if (!expo_icon_found)
    SaveIconsOrder();
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

  icon->position_forgot.connect([this, icon_uri] {
    FavoriteStore::Instance().RemoveFavorite(icon_uri);
  });

  if (icon->GetIconType() == AbstractLauncherIcon::IconType::APPLICATION)
  {
    icon->visibility_changed.connect(sigc::mem_fun(this, &Impl::SortAndUpdate));
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

void Controller::Impl::OnViewOpened(BamfMatcher* matcher, BamfView* view)
{
  if (!BAMF_IS_APPLICATION(view))
    return;

  BamfApplication* app = BAMF_APPLICATION(view);

  if (bamf_view_is_sticky(view) ||
      g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
  {
    return;
  }

  AbstractLauncherIcon::Ptr icon(new ApplicationLauncherIcon(app));
  RegisterIcon(icon, GetLastIconPriority<ApplicationLauncherIcon>(local::RUNNING_APPS_URI));
}

void Controller::Impl::OnDeviceIconAdded(AbstractLauncherIcon::Ptr const& icon)
{
  RegisterIcon(icon, GetLastIconPriority<VolumeLauncherIcon>(local::DEVICES_URI));
}

AbstractLauncherIcon::Ptr Controller::Impl::CreateFavoriteIcon(std::string const& icon_uri)
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
    BamfApplication* app;
    std::string const& desktop_path = DesktopUtilities::GetDesktopPathById(desktop_id);

    app = bamf_matcher_get_application_for_desktop_file(matcher_, desktop_path.c_str(), true);

    if (!app)
      return result;

    if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
    {
      bamf_view_set_sticky(BAMF_VIEW(app), true);
      return result;
    }

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
    result->Stick(false);

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
  BamfApplication* app;
  SoftwareCenterLauncherIcon::Ptr result;

  app = bamf_matcher_get_application_for_desktop_file(matcher_, file_path.c_str(), true);
  if (!BAMF_IS_APPLICATION(app))
    return result;

  if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
  {
    bamf_view_set_sticky(BAMF_VIEW(app), true);
    return result;
  }

  bamf_view_set_sticky(BAMF_VIEW(app), true);
  result = new SoftwareCenterLauncherIcon(app, aptdaemon_trans_id, icon_path);

  return result;
}

void Controller::Impl::AddRunningApps()
{
  std::shared_ptr<GList> apps(bamf_matcher_get_applications(matcher_), g_list_free);

  for (GList *l = apps.get(); l; l = l->next)
  {
    if (!BAMF_IS_APPLICATION(l->data))
      continue;

    BamfApplication* app = BAMF_APPLICATION(l->data);

    if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
      continue;

    AbstractLauncherIcon::Ptr icon(new ApplicationLauncherIcon(app));
    RegisterIcon(icon, ++sort_priority_);
  }
}

void Controller::Impl::AddDevices()
{
  auto& fav_store = FavoriteStore::Instance();
  for (auto const& icon : device_section_.GetIcons())
  {
    if (!icon->IsSticky() && !fav_store.IsFavorite(icon->RemoteUri()))
      RegisterIcon(icon, ++sort_priority_);
  }
}

void Controller::Impl::SetupIcons()
{
  auto& favorite_store = FavoriteStore::Instance();
  FavoriteList const& favs = favorite_store.GetFavorites();
  bool running_apps_added = false;
  bool devices_added = false;

  for (auto const& fav_uri : favs)
  {
    if (fav_uri == local::RUNNING_APPS_URI)
    {
      AddRunningApps();
      running_apps_added = true;
      continue;
    }
    else if (fav_uri == local::DEVICES_URI)
    {
      AddDevices();
      devices_added = true;
      continue;
    }

    RegisterIcon(CreateFavoriteIcon(fav_uri), ++sort_priority_);
  }

  if (!running_apps_added)
    AddRunningApps();

  if (model_->IconIndex(expo_icon_) < 0)
    RegisterIcon(CreateFavoriteIcon(expo_icon_->RemoteUri()), ++sort_priority_);

  if (!devices_added)
    AddDevices();

  if (std::find(favs.begin(), favs.end(), expo_icon_->RemoteUri()) == favs.end())
    SaveIconsOrder();

  view_opened_signal_.Connect(matcher_, "view-opened", sigc::mem_fun(this, &Impl::OnViewOpened));
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
                   g_variant_new("(sus)", "home.lens", dash::NOT_HANDLED, ""));
}

Controller::Controller()
 : options(Options::Ptr(new Options()))
 , multiple_launchers(true)
 , pimpl(new Impl(this))
{
  multiple_launchers.changed.connect([&](bool value) -> void {
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

  auto show_launcher = [&]()
  {
    if (pimpl->keyboard_launcher_.IsNull())
      pimpl->keyboard_launcher_ = pimpl->CurrentLauncher();

    pimpl->sources_.Remove(local::HIDE_TIMEOUT);
    pimpl->keyboard_launcher_->ForceReveal(true);
    pimpl->launcher_open = true;

    return false;
  };
  pimpl->sources_.AddTimeout(local::super_tap_duration, show_launcher, local::KEYPRESS_TIMEOUT);

  auto show_shortcuts = [&]()
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
  if ((when - pimpl->launcher_key_press_time_) < local::super_tap_duration && was_tap)
    return true;
  return false;
}

void Controller::HandleLauncherKeyRelease(bool was_tap, int when)
{
  int tap_duration = when - pimpl->launcher_key_press_time_;
  if (tap_duration < local::super_tap_duration && was_tap)
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

      auto hide_launcher = [&]()
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

bool Controller::HandleLauncherKeyEvent(Display *display, unsigned int key_sym, unsigned long key_code, unsigned long key_state, char* key_string)
{
  LauncherModel::iterator it;

  // Shortcut to start launcher icons. Only relies on Keycode, ignore modifier
  for (it = pimpl->model_->begin(); it != pimpl->model_->end(); ++it)
  {
    if ((XKeysymToKeycode(display, (*it)->GetShortcut()) == key_code) ||
        ((gchar)((*it)->GetShortcut()) == key_string[0]))
    {
      struct timespec last_action_time = (*it)->GetQuirkTime(AbstractLauncherIcon::Quirk::LAST_ACTION);
      struct timespec current;
      TimeUtil::SetTimeStruct(&current);
      if (TimeUtil::TimeDelta(&current, &last_action_time) > local::ignore_repeat_shortcut_duration)
      {
        if (g_ascii_isdigit((gchar)(*it)->GetShortcut()) && (key_state & ShiftMask))
          (*it)->OpenInstance(ActionArg(ActionArg::LAUNCHER, 0));
        else
          (*it)->Activate(ActionArg(ActionArg::LAUNCHER, 0));
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
                            g_variant_new_int32(pimpl->keyboard_launcher_->monitor));
  }
  else
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_START_KEY_SWITCHER,
                            g_variant_new_int32(pimpl->keyboard_launcher_->monitor));
  }

  AbstractLauncherIcon::Ptr const& selected = pimpl->model_->Selection();

  if (selected)
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                            g_variant_new_string(selected->tooltip_text().c_str()));
  }
}

void Controller::KeyNavNext()
{
  pimpl->model_->SelectNext();

  AbstractLauncherIcon::Ptr const& selected = pimpl->model_->Selection();

  if (selected)
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                            g_variant_new_string(selected->tooltip_text().c_str()));
  }
}

void Controller::KeyNavPrevious()
{
  pimpl->model_->SelectPrevious();

  AbstractLauncherIcon::Ptr const& selected = pimpl->model_->Selection();

  if (selected)
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                            g_variant_new_string(selected->tooltip_text().c_str()));
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
    pimpl->launcher_key_press_connection_.disconnect();
    pimpl->launcher_event_outside_connection_.disconnect();
    pimpl->launcher_grabbed = false;
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_END_KEY_NAV,
                            g_variant_new_boolean(pimpl->keynav_restore_window_));
  }
  else
  {
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_END_KEY_SWITCHER,
                            g_variant_new_boolean(pimpl->keynav_restore_window_));
  }

  if (activate)
  {
    pimpl->sources_.AddIdle([this] {
      pimpl->model_->Selection()->Activate(ActionArg(ActionArg::LAUNCHER, 0));
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
Controller::AddProperties(GVariantBuilder* builder)
{
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  variant::BuilderWrapper(builder)
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
      model_->Selection()->OpenInstance(ActionArg(ActionArg::LAUNCHER, 0));
      parent_->KeyNavTerminate(false);
      break;

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

void Controller::Impl::OnBusAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
  GDBusNodeInfo* introspection_data = g_dbus_node_info_new_for_xml(DBUS_INTROSPECTION.c_str(), nullptr);

  if (!introspection_data)
  {
    LOG_WARNING(logger) << "No introspection data loaded. Won't get dynamic launcher addition.";
    return;
  }

  auto self = static_cast<Controller::Impl*>(user_data);

  self->gdbus_connection_ = connection;
  self->reg_id_ = g_dbus_connection_register_object(connection, DBUS_PATH.c_str(),
                                                    introspection_data->interfaces[0],
                                                    &interface_vtable, user_data,
                                                    nullptr, nullptr);
  if (!self->reg_id_)
  {
    LOG_WARNING(logger) << "Object registration failed. Won't get dynamic launcher addition.";
  }

  g_dbus_node_info_unref(introspection_data);
}

void Controller::Impl::OnDBusMethodCall(GDBusConnection* connection, const gchar* sender,
                                        const gchar* object_path, const gchar* interface_name,
                                        const gchar* method_name, GVariant* parameters,
                                        GDBusMethodInvocation* invocation, gpointer user_data)
{
  if (g_strcmp0(method_name, "AddLauncherItemFromPosition") == 0)
  {
    auto self = static_cast<Controller::Impl*>(user_data);
    glib::String icon, icon_title, desktop_file, aptdaemon_task;
    gint icon_x, icon_y, icon_size;

    g_variant_get(parameters, "(ssiiiss)", &icon_title, &icon, &icon_x, &icon_y,
                                           &icon_size, &desktop_file, &aptdaemon_task);

    self->OnLauncherAddRequestSpecial(desktop_file.Str(), aptdaemon_task.Str(),
                                      icon.Str(), icon_x, icon_y, icon_size);

    g_dbus_method_invocation_return_value(invocation, nullptr);
  }
}

} // namespace launcher
} // namespace unity
