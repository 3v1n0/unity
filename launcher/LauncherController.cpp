// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010, 2011 Canonical Ltd
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
 */

#include <glib/gi18n-lib.h>
#include <libbamf/libbamf.h>

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Logger.h>

#include "LauncherOptions.h"
#include "BamfLauncherIcon.h"
#include "DesktopLauncherIcon.h"
#include "DeviceLauncherIcon.h"
#include "DeviceLauncherSection.h"
#include "EdgeBarrierController.h"
#include "FavoriteStore.h"
#include "HudLauncherIcon.h"
#include "Launcher.h"
#include "LauncherController.h"
#include "LauncherEntryRemote.h"
#include "LauncherEntryRemoteModel.h"
#include "AbstractLauncherIcon.h"
#include "SoftwareCenterLauncherIcon.h"
#include "LauncherModel.h"
#include "VolumeMonitorWrapper.h"
#include "unity-shared/WindowManager.h"
#include "TrashLauncherIcon.h"
#include "BFBLauncherIcon.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/UBusWrapper.h"
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
}
}

class Controller::Impl
{
public:
  Impl(Display* display, Controller* parent);
  ~Impl();

  void UpdateNumWorkspaces(int workspaces);

  Launcher* CreateLauncher(int monitor);

  void Save();
  void SortAndUpdate();

  nux::ObjectPtr<Launcher> CurrentLauncher();

  void OnIconAdded(AbstractLauncherIcon::Ptr icon);
  void OnIconRemoved(AbstractLauncherIcon::Ptr icon);

  void OnLauncherAddRequest(char* path, AbstractLauncherIcon::Ptr before);
  void OnLauncherAddRequestSpecial(std::string const& path, std::string const& aptdaemon_trans_id,
                                   std::string const& icon_path, int icon_x, int icon_y, int icon_size);
  void OnLauncherRemoveRequest(AbstractLauncherIcon::Ptr icon);
  void OnSCIconAnimationComplete(AbstractLauncherIcon::Ptr icon);

  void OnLauncherEntryRemoteAdded(LauncherEntryRemote::Ptr const& entry);
  void OnLauncherEntryRemoteRemoved(LauncherEntryRemote::Ptr const& entry);

  void OnFavoriteStoreFavoriteAdded(std::string const& entry, std::string const& pos, bool before);
  void OnFavoriteStoreFavoriteRemoved(std::string const& entry);
  void OnFavoriteStoreReordered();


  void InsertExpoAction();
  void RemoveExpoAction();

  void InsertDesktopIcon();
  void RemoveDesktopIcon();

  void SendHomeActivationRequest();

  int MonitorWithMouse();

  void InsertTrash();

  void RegisterIcon(AbstractLauncherIcon::Ptr icon);

  AbstractLauncherIcon::Ptr CreateFavorite(const char* file_path);

  SoftwareCenterLauncherIcon::Ptr CreateSCLauncherIcon(std::string const& file_path, std::string const& aptdaemon_trans_id, std::string const& icon_path);

  void SetupBamf();

  void EnsureLaunchers(int primary, std::vector<nux::Geometry> const& monitors);

  void OnExpoActivated();

  void OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors);

  void OnWindowFocusChanged (guint32 xid);

  void OnViewOpened(BamfMatcher* matcher, BamfView* view);

  void ReceiveMouseDownOutsideArea(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void ReceiveLauncherKeyPress(unsigned long eventType,
                               unsigned long keysym,
                               unsigned long state,
                               const char* character,
                               unsigned short keyCount);

  static void OnBusAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
  static void OnDBusMethodCall(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                               const gchar* interface_name, const gchar* method_name,
                               GVariant* parameters, GDBusMethodInvocation* invocation,
                               gpointer user_data);

  static GDBusInterfaceVTable interface_vtable;

  Controller* parent_;
  LauncherModel::Ptr model_;
  nux::ObjectPtr<Launcher> launcher_;
  nux::ObjectPtr<Launcher> keyboard_launcher_;
  int                    sort_priority_;
  AbstractVolumeMonitorWrapper::Ptr volume_monitor_;
  DeviceLauncherSection  device_section_;
  LauncherEntryRemoteModel remote_model_;
  AbstractLauncherIcon::Ptr expo_icon_;
  AbstractLauncherIcon::Ptr desktop_icon_;
  int                    num_workspaces_;
  bool                   show_desktop_icon_;
  Display*               display_;

  bool                   launcher_open;
  bool                   launcher_keynav;
  bool                   launcher_grabbed;
  bool                   reactivate_keynav;
  int                    reactivate_index;
  bool                   keynav_restore_window_;
  int                    launcher_key_press_time_;
  unsigned int           dbus_owner_;

  ui::EdgeBarrierController edge_barriers_;

  LauncherList launchers;

  glib::Object<BamfMatcher> matcher_;
  glib::Signal<void, BamfMatcher*, BamfView*> view_opened_signal_;
  glib::SourceManager sources_;
  UBusManager ubus;

  sigc::connection on_expoicon_activate_connection_;
  sigc::connection launcher_key_press_connection_;
  sigc::connection launcher_event_outside_connection_;
};

GDBusInterfaceVTable Controller::Impl::interface_vtable =
  { Controller::Impl::OnDBusMethodCall, NULL, NULL};

Controller::Impl::Impl(Display* display, Controller* parent)
  : parent_(parent)
  , model_(new LauncherModel())
  , sort_priority_(0)
  , volume_monitor_(new VolumeMonitorWrapper)
  , device_section_(volume_monitor_)
  , show_desktop_icon_(false)
  , display_(display)
  , matcher_(bamf_matcher_get_default())
{
  edge_barriers_.options = parent_->options();

  UScreen* uscreen = UScreen::GetDefault();
  auto monitors = uscreen->GetMonitors();
  int primary = uscreen->GetPrimaryMonitor();

  launcher_open = false;
  launcher_keynav = false;
  launcher_grabbed = false;
  reactivate_keynav = false;
  keynav_restore_window_ = true;

  EnsureLaunchers(primary, monitors);

  launcher_ = launchers[0];
  device_section_.IconAdded.connect(sigc::mem_fun(this, &Impl::OnIconAdded));

  num_workspaces_ = WindowManager::Default()->WorkspaceCount();
  if (num_workspaces_ > 1)
  {
    InsertExpoAction();
  }

  // Insert the "Show Desktop" launcher icon in the launcher...
  if (show_desktop_icon_)
    InsertDesktopIcon();

  InsertTrash();

  sources_.AddTimeout(500, [&] { SetupBamf(); return false; });

  remote_model_.entry_added.connect(sigc::mem_fun(this, &Impl::OnLauncherEntryRemoteAdded));
  remote_model_.entry_removed.connect(sigc::mem_fun(this, &Impl::OnLauncherEntryRemoteRemoved));

  FavoriteStore::Instance().favorite_added.connect(sigc::mem_fun(this, &Impl::OnFavoriteStoreFavoriteAdded));
  FavoriteStore::Instance().favorite_removed.connect(sigc::mem_fun(this, &Impl::OnFavoriteStoreFavoriteRemoved));
  FavoriteStore::Instance().reordered.connect(sigc::mem_fun(this, &Impl::OnFavoriteStoreReordered));

  LauncherHideMode hide_mode = parent_->options()->hide_mode;
  BFBLauncherIcon* bfb = new BFBLauncherIcon(hide_mode);
  RegisterIcon(AbstractLauncherIcon::Ptr(bfb));

  HudLauncherIcon* hud = new HudLauncherIcon(hide_mode);
  RegisterIcon(AbstractLauncherIcon::Ptr(hud));

  parent_->options()->hide_mode.changed.connect([bfb,hud](LauncherHideMode mode) {
    bfb->SetHideMode(mode);
    hud->SetHideMode(mode);
  });

  desktop_icon_ = AbstractLauncherIcon::Ptr(new DesktopLauncherIcon());

  uscreen->changed.connect(sigc::mem_fun(this, &Controller::Impl::OnScreenChanged));

  WindowManager& plugin_adapter = *(WindowManager::Default());
  plugin_adapter.window_focus_changed.connect (sigc::mem_fun (this, &Controller::Impl::OnWindowFocusChanged));

  launcher_key_press_time_ = 0;

  ubus.RegisterInterest(UBUS_QUICKLIST_END_KEY_NAV, [&](GVariant * args) {
    if (reactivate_keynav)
      parent_->KeyNavGrab();
      model_->SetSelection(reactivate_index);
  });

  parent_->AddChild(model_.get());

  uscreen->resuming.connect([&]() -> void {
    for (auto launcher : launchers)
      launcher->QueueDraw();
  });

  dbus_owner_ = g_bus_own_name(G_BUS_TYPE_SESSION, DBUS_NAME.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
                               OnBusAcquired, nullptr, nullptr, this, nullptr);
}

Controller::Impl::~Impl()
{
  // Since the launchers are in a window which adds a reference to the
  // launcher, we need to make sure the base windows are unreferenced
  // otherwise the launchers never die.
  for (auto launcher_ptr : launchers)
  {
    if (launcher_ptr.IsValid())
      launcher_ptr->GetParent()->UnReference();
  }

  g_bus_unown_name(dbus_owner_);
}

void Controller::Impl::EnsureLaunchers(int primary, std::vector<nux::Geometry> const& monitors)
{
  unsigned int num_monitors = monitors.size();
  unsigned int num_launchers = parent_->multiple_launchers ? num_monitors : 1;
  unsigned int launchers_size = launchers.size();
  unsigned int last_monitor = 0;

  if (num_launchers == 1)
  {
    if (launchers_size == 0)
    {
      launchers.push_back(nux::ObjectPtr<Launcher>(CreateLauncher(primary)));
    }
    else if (!launchers[0].IsValid())
    {
      launchers[0] = nux::ObjectPtr<Launcher>(CreateLauncher(primary));
    }

    launchers[0]->monitor(primary);
    launchers[0]->Resize();
    last_monitor = 1;
  }
  else
  {
    for (unsigned int i = 0; i < num_monitors; i++, last_monitor++)
    {
      if (i >= launchers_size)
      {
        launchers.push_back(nux::ObjectPtr<Launcher>(CreateLauncher(i)));
      }

      launchers[i]->monitor(i);
      launchers[i]->Resize();
    }
  }

  for (unsigned int i = last_monitor; i < launchers_size; ++i)
  {
    auto launcher = launchers[i];
    if (launcher.IsValid())
    {
      parent_->RemoveChild(launcher.GetPointer());
      launcher->GetParent()->UnReference();
      edge_barriers_.Unsubscribe(launcher.GetPointer(), launcher->monitor);
    }
  }

  launchers.resize(num_launchers);

  for (size_t i = 0; i < launchers.size(); ++i)
  {
    edge_barriers_.Subscribe(launchers[i].GetPointer(), launchers[i]->monitor);
  }
}

void Controller::Impl::OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors)
{
  EnsureLaunchers(primary_monitor, monitors);
}

void Controller::Impl::OnWindowFocusChanged (guint32 xid)
{
  static bool keynav_first_focus = false;

  if (parent_->IsOverlayOpen())
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
  launcher->display = display_;
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
  launcher_window->InputWindowEnableStruts(false);
  launcher_window->SetEnterFocusInputArea(launcher);

  launcher->launcher_addrequest.connect(sigc::mem_fun(this, &Impl::OnLauncherAddRequest));
  launcher->launcher_removerequest.connect(sigc::mem_fun(this, &Impl::OnLauncherRemoveRequest));

  launcher->icon_animation_complete.connect(sigc::mem_fun(this, &Impl::OnSCIconAnimationComplete));

  parent_->AddChild(launcher);

  return launcher;
}

void Controller::Impl::OnLauncherAddRequest(char* path, AbstractLauncherIcon::Ptr before)
{
  for (auto it : model_->GetSublist<BamfLauncherIcon> ())
  {
    if (path && path == it->DesktopFile())
    {
      it->Stick();
      model_->ReorderBefore(it, before, false);
      Save();
      return;
    }
  }

  AbstractLauncherIcon::Ptr result = CreateFavorite(path);
  if (result)
  {
    RegisterIcon(result);
    if (before)
      model_->ReorderBefore(result, before, false);
  }

  Save();
}

void Controller::Impl::Save()
{
  unity::FavoriteList desktop_paths;

  // Updates gsettings favorites.
  auto launchers = model_->GetSublist<BamfLauncherIcon> ();
  for (auto icon : launchers)
  {
    if (!icon->IsSticky())
      continue;

    std::string const& desktop_file = icon->DesktopFile();

    if (!desktop_file.empty())
      desktop_paths.push_back(desktop_file);
  }

  unity::FavoriteStore::Instance().SetFavorites(desktop_paths);
}

void
Controller::Impl::OnLauncherAddRequestSpecial(std::string const& path,
                                              std::string const& aptdaemon_trans_id,
                                              std::string const& icon_path,
                                              int icon_x,
                                              int icon_y,
                                              int icon_size)
{
  auto bamf_icons = model_->GetSublist<BamfLauncherIcon>();
  for (auto icon : bamf_icons)
  {
    if (icon->DesktopFile() == path)
      return;
  }

  SoftwareCenterLauncherIcon::Ptr result = CreateSCLauncherIcon(path, aptdaemon_trans_id, icon_path);

  CurrentLauncher()->ForceReveal(true);

  if (result)
  {
    result->SetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE, false);
    result->Animate(CurrentLauncher(), icon_x, icon_y, icon_size);
    RegisterIcon(result);
    Save();
  }
}

void Controller::Impl::OnSCIconAnimationComplete(AbstractLauncherIcon::Ptr icon)
{
  icon->SetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE, true);
  launcher_->ForceReveal(false);
}

void Controller::Impl::SortAndUpdate()
{
  gint   shortcut = 1;

  auto launchers = model_->GetSublist<BamfLauncherIcon> ();
  for (auto icon : launchers)
  {
    if (shortcut <= 10 && icon->IsVisible())
    {
      std::stringstream shortcut_string;
      shortcut_string << (shortcut % 10);
      icon->SetShortcut(shortcut_string.str()[0]);
      ++shortcut;
    }
    // reset shortcut
    else
    {
      icon->SetShortcut(0);
    }
  }
}

void Controller::Impl::OnIconAdded(AbstractLauncherIcon::Ptr icon)
{
  this->RegisterIcon(icon);
}

void Controller::Impl::OnIconRemoved(AbstractLauncherIcon::Ptr icon)
{
  SortAndUpdate();
}

void Controller::Impl::OnLauncherRemoveRequest(AbstractLauncherIcon::Ptr icon)
{
  switch (icon->GetIconType())
  {
    case AbstractLauncherIcon::TYPE_APPLICATION:
    {
      BamfLauncherIcon* bamf_icon = dynamic_cast<BamfLauncherIcon*>(icon.GetPointer());

      if (bamf_icon)
      {
        bamf_icon->UnStick();
        bamf_icon->Quit();
      }

      break;
    }
    case AbstractLauncherIcon::TYPE_DEVICE:
    {
      DeviceLauncherIcon* device_icon = dynamic_cast<DeviceLauncherIcon*>(icon.GetPointer());

      if (device_icon && device_icon->CanEject())
        device_icon->Eject();

      break;
    }
    default:
      break;
  }
}

void Controller::Impl::OnLauncherEntryRemoteAdded(LauncherEntryRemote::Ptr const& entry)
{
  for (auto icon : *model_)
  {
    if (!icon || icon->RemoteUri().empty())
      continue;

    if (entry->AppUri() == icon->RemoteUri())
    {
      icon->InsertEntryRemote(entry);
    }
  }
}

void Controller::Impl::OnLauncherEntryRemoteRemoved(LauncherEntryRemote::Ptr const& entry)
{
  for (auto icon : *model_)
  {
    icon->RemoveEntryRemote(entry);
  }
}

void Controller::Impl::OnFavoriteStoreFavoriteAdded(std::string const& entry, std::string const& pos, bool before)
{
  auto bamf_list = model_->GetSublist<BamfLauncherIcon>();
  AbstractLauncherIcon::Ptr other;
  if (bamf_list.size() > 0)
    other = *(bamf_list.begin());

  if (!pos.empty())
  {
    for (auto it : bamf_list)
    {
      if (it->GetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE) && pos == it->DesktopFile())
        other = it;
    }
  }

  for (auto it : bamf_list)
  {
    if (entry == it->DesktopFile())
    {
      it->Stick(false);
      if (!before)
        model_->ReorderAfter(it, other);
      else
        model_->ReorderBefore(it, other, false);
      return;
    }
  }

  AbstractLauncherIcon::Ptr result = CreateFavorite(entry.c_str());
  if (result)
  {
    RegisterIcon(result);
    if (!before)
      model_->ReorderAfter(result, other);
    else
      model_->ReorderBefore(result, other, false);
  }

  SortAndUpdate();
}

void Controller::Impl::OnFavoriteStoreFavoriteRemoved(std::string const& entry)
{
  for (auto it : model_->GetSublist<BamfLauncherIcon> ())
  {
    if (it->DesktopFile() == entry)
    {
      OnLauncherRemoveRequest(it);
      break;
     }
  }
}

void Controller::Impl::OnFavoriteStoreReordered()
{
  FavoriteList const& favs = FavoriteStore::Instance().GetFavorites();
  auto bamf_list = model_->GetSublist<BamfLauncherIcon>();

  int i = 0;
  for (auto it : favs)
  {
    auto icon = std::find_if(bamf_list.begin(), bamf_list.end(),
    [&it](AbstractLauncherIcon::Ptr x) { return (x->DesktopFile() == it); });

    if (icon != bamf_list.end())
    {
      (*icon)->SetSortPriority(i++);
    }
  }

  for (auto it : bamf_list)
  {
    if (!it->IsSticky())
      it->SetSortPriority(i++);
  }

  model_->Sort();
}

void Controller::Impl::OnExpoActivated()
{
  WindowManager::Default()->InitiateExpo();
}

void Controller::Impl::InsertTrash()
{
  AbstractLauncherIcon::Ptr icon(new TrashLauncherIcon());
  RegisterIcon(icon);
}

void Controller::Impl::UpdateNumWorkspaces(int workspaces)
{
  if ((num_workspaces_ == 0) && (workspaces > 0))
  {
    InsertExpoAction();
  }
  else if ((num_workspaces_ > 0) && (workspaces == 0))
  {
    RemoveExpoAction();
  }

  num_workspaces_ = workspaces;
}

void Controller::Impl::InsertExpoAction()
{
  expo_icon_ = AbstractLauncherIcon::Ptr(new SimpleLauncherIcon());

  SimpleLauncherIcon* icon = static_cast<SimpleLauncherIcon*>(expo_icon_.GetPointer());
  icon->tooltip_text = _("Workspace Switcher");
  icon->icon_name = "workspace-switcher";
  icon->SetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE, true);
  icon->SetQuirk(AbstractLauncherIcon::QUIRK_RUNNING, false);
  icon->SetIconType(AbstractLauncherIcon::TYPE_EXPO);
  icon->SetShortcut('s');

  on_expoicon_activate_connection_ = icon->activate.connect(sigc::mem_fun(this, &Impl::OnExpoActivated));


  RegisterIcon(expo_icon_);
}

void Controller::Impl::RemoveExpoAction()
{
  if (on_expoicon_activate_connection_)
    on_expoicon_activate_connection_.disconnect();
  model_->RemoveIcon(expo_icon_);
}

void Controller::Impl::InsertDesktopIcon()
{
  RegisterIcon(desktop_icon_);
}

void Controller::Impl::RemoveDesktopIcon()
{
  model_->RemoveIcon(desktop_icon_);
}

void Controller::Impl::RegisterIcon(AbstractLauncherIcon::Ptr icon)
{
  model_->AddIcon(icon);
  std::string const& path = icon->DesktopFile();

  if (!path.empty())
  {
    LauncherEntryRemote::Ptr const& entry = remote_model_.LookupByDesktopFile(path);

    if (entry)
      icon->InsertEntryRemote(entry);
  }
}

/* static private */
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

  AbstractLauncherIcon::Ptr icon(new BamfLauncherIcon(app));
  icon->visibility_changed.connect(sigc::mem_fun(this, &Impl::SortAndUpdate));
  icon->SetSortPriority(sort_priority_++);
  RegisterIcon(icon);
  SortAndUpdate();
}

AbstractLauncherIcon::Ptr Controller::Impl::CreateFavorite(const char* file_path)
{
  BamfApplication* app;
  AbstractLauncherIcon::Ptr result;

  app = bamf_matcher_get_application_for_desktop_file(matcher_, file_path, true);
  if (!app)
    return result;

  if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
  {
    bamf_view_set_sticky(BAMF_VIEW(app), true);
    return result;
  }

  bamf_view_set_sticky(BAMF_VIEW(app), true);
  AbstractLauncherIcon::Ptr icon (new BamfLauncherIcon(app));
  icon->SetSortPriority(sort_priority_++);
  result = icon;

  return result;
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
  result->SetSortPriority(sort_priority_++);

  return result;
}

void Controller::Impl::SetupBamf()
{
  GList* apps, *l;
  BamfApplication* app;

  // Sufficiently large number such that we ensure proper sorting
  // (avoids case where first item gets tacked onto end rather than start)
  int priority = 100;

  FavoriteList const& favs = FavoriteStore::Instance().GetFavorites();

  for (FavoriteList::const_iterator i = favs.begin(), end = favs.end();
       i != end; ++i)
  {
    AbstractLauncherIcon::Ptr fav = CreateFavorite(i->c_str());

    if (fav)
    {
      fav->SetSortPriority(priority);
      RegisterIcon(fav);
      priority++;
    }
  }

  apps = bamf_matcher_get_applications(matcher_);
  view_opened_signal_.Connect(matcher_, "view-opened", sigc::mem_fun(this, &Impl::OnViewOpened));

  for (l = apps; l; l = l->next)
  {
    app = BAMF_APPLICATION(l->data);

    if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
      continue;

    AbstractLauncherIcon::Ptr icon(new BamfLauncherIcon(app));
    icon->SetSortPriority(sort_priority_++);
    RegisterIcon(icon);
  }
  g_list_free(apps);
  SortAndUpdate();

  model_->order_changed.connect(sigc::mem_fun(this, &Impl::SortAndUpdate));
  model_->icon_removed.connect(sigc::mem_fun(this, &Impl::OnIconRemoved));
  model_->saved.connect(sigc::mem_fun(this, &Impl::Save));
}

void Controller::Impl::SendHomeActivationRequest()
{
  ubus.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST, g_variant_new("(sus)", "home.lens", dash::NOT_HANDLED, ""));
}

Controller::Controller(Display* display)
 : options(Options::Ptr(new Options()))
 , multiple_launchers(true)
 , pimpl(new Impl(display, this))
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

std::vector<AbstractLauncherIcon::Ptr> Controller::GetAltTabIcons(bool current) const
{
  std::vector<AbstractLauncherIcon::Ptr> results;

  results.push_back(pimpl->desktop_icon_);

  for (auto icon : *(pimpl->model_))
  {
    if (icon->ShowInSwitcher(current))
    {
      //otherwise we get two desktop icons in the switcher.
      if (icon->GetIconType() != AbstractLauncherIcon::IconType::TYPE_DESKTOP)
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

void Controller::SetShowDesktopIcon(bool show_desktop_icon)
{
  if (pimpl->show_desktop_icon_ == show_desktop_icon)
    return;

  pimpl->show_desktop_icon_ = show_desktop_icon;

  if (pimpl->show_desktop_icon_)
    pimpl->InsertDesktopIcon();
  else
    pimpl->RemoveDesktopIcon();
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
  for (it = pimpl->model_->begin(); it != pimpl->model_->end(); it++)
  {
    if ((XKeysymToKeycode(display, (*it)->GetShortcut()) == key_code) ||
        ((gchar)((*it)->GetShortcut()) == key_string[0]))
    {
      struct timespec last_action_time = (*it)->GetQuirkTime(AbstractLauncherIcon::QUIRK_LAST_ACTION);
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
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_START_KEY_SWTICHER,
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
    pimpl->keynav_restore_window_ = !icon->GetQuirk(AbstractLauncherIcon::QUIRK_RUNNING);
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
    pimpl->ubus.SendMessage(UBUS_LAUNCHER_END_KEY_SWTICHER,
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

  unity::variant::BuilderWrapper(builder)
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
      if (model_->Selection()->OpenQuicklist(true, keyboard_launcher_->monitor()))
      {
        reactivate_keynav = true;
        reactivate_index = model_->SelectionIndex();
        parent_->KeyNavTerminate(false);
      }
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

void Controller::Impl::OnBusAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
  GDBusNodeInfo* introspection_data = g_dbus_node_info_new_for_xml(DBUS_INTROSPECTION.c_str(), nullptr);
  unsigned int reg_id;

  if (!introspection_data)
  {
    LOG_WARNING(logger) << "No introspection data loaded. Won't get dynamic launcher addition.";
    return;
  }

  reg_id = g_dbus_connection_register_object(connection, DBUS_PATH.c_str(),
                                             introspection_data->interfaces[0],
                                             &interface_vtable, user_data,
                                             nullptr, nullptr);
  if (!reg_id)
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
