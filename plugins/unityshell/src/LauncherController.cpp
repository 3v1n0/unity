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
#include "FavoriteStore.h"
#include "Launcher.h"
#include "LauncherController.h"
#include "LauncherEntryRemote.h"
#include "LauncherEntryRemoteModel.h"
#include "AbstractLauncherIcon.h"
#include "SoftwareCenterLauncherIcon.h"
#include "LauncherModel.h"
#include "WindowManager.h"
#include "TrashLauncherIcon.h"
#include "BFBLauncherIcon.h"
#include "UScreen.h"
#include "UBusWrapper.h"
#include "UBusMessages.h"
#include "TimeUtil.h"

namespace unity
{
namespace launcher
{
namespace
{
nux::logging::Logger logger("unity.launcher");
}

namespace local
{
namespace
{
  const int super_tap_duration = 250;
  const int launcher_minimum_show_duration = 1250;
  const int shortcuts_show_delay = 750;
  const int ignore_repeat_shortcut_duration = 250;
}
}

// FIXME: key-code defines for Up/Down/Left/Right of numeric keypad - needs to
// be moved to the correct place in NuxGraphics-headers
#define NUX_KP_DOWN  0xFF99
#define NUX_KP_UP    0xFF97
#define NUX_KP_LEFT  0xFF96
#define NUX_KP_RIGHT 0xFF98

class Controller::Impl
{
public:
  Impl(Display* display, Controller* parent);
  ~Impl();

  void UpdateNumWorkspaces(int workspaces);

  Launcher* CreateLauncher(int monitor);

  void Save();
  void SortAndUpdate();

  void OnIconAdded(AbstractLauncherIcon* icon);
  void OnIconRemoved(AbstractLauncherIcon* icon);

  void OnLauncherAddRequest(char* path, AbstractLauncherIcon* before);
  void OnLauncherAddRequestSpecial(char* path, AbstractLauncherIcon* before, char* aptdaemon_trans_id, char* icon_path);
  void OnLauncherRemoveRequest(AbstractLauncherIcon* icon);

  void OnLauncherEntryRemoteAdded(LauncherEntryRemote* entry);
  void OnLauncherEntryRemoteRemoved(LauncherEntryRemote* entry);
  
  void OnFavoriteStoreFavoriteAdded(std::string const& entry, std::string const& pos, bool before);
  void OnFavoriteStoreFavoriteRemoved(std::string const& entry);
  void OnFavoriteStoreReordered();


  void InsertExpoAction();
  void RemoveExpoAction();

  void InsertDesktopIcon();
  void RemoveDesktopIcon();

  bool TapTimeUnderLimit();

  void SendHomeActivationRequest();

  int TimeSinceLauncherKeyPress();

  int MonitorWithMouse();

  void InsertTrash();

  void RegisterIcon(AbstractLauncherIcon* icon);

  AbstractLauncherIcon* CreateFavorite(const char* file_path);

  SoftwareCenterLauncherIcon* CreateSCLauncherIcon(const char* file_path, const char* aptdaemon_trans_id, char* icon_path);

  void SetupBamf();

  void OnExpoActivated();

  void OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors);

  void ReceiveMouseDownOutsideArea(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void ReceiveLauncherKeyPress(unsigned long eventType, 
                               unsigned long keysym, 
                               unsigned long state, 
                               const char* character, 
                               unsigned short keyCount);

  /* statics */

  static void OnViewOpened(BamfMatcher* matcher, BamfView* view, gpointer data);

  Controller* parent_;
  glib::Object<BamfMatcher> matcher_;
  LauncherModel::Ptr     model_;
  nux::ObjectPtr<Launcher> launcher_;
  nux::ObjectPtr<Launcher> keyboard_launcher_;
  int                    sort_priority_;
  DeviceLauncherSection* device_section_;
  LauncherEntryRemoteModel remote_model_;
  SimpleLauncherIcon*    expo_icon_;
  DesktopLauncherIcon*   desktop_launcher_icon_;
  nux::ObjectPtr<AbstractLauncherIcon> desktop_icon_;
  int                    num_workspaces_;
  bool                   show_desktop_icon_;
  Display*               display_;

  guint                  bamf_timer_handler_id_;
  guint                  on_view_opened_id_;
  guint                  launcher_key_press_handler_id_;
  guint                  launcher_label_show_handler_id_;
  guint                  launcher_hide_handler_id_;

  bool                   launcher_open;
  bool                   launcher_keynav;
  bool                   launcher_grabbed;
  bool                   reactivate_keynav;
  int                    reactivate_index;

  UBusManager            ubus;

  struct timespec        launcher_key_press_time_;

  LauncherList launchers;

  sigc::connection on_expoicon_activate_connection_;
  sigc::connection launcher_key_press_connection_;
  sigc::connection launcher_event_outside_connection_;
};


Controller::Impl::Impl(Display* display, Controller* parent)
  : parent_(parent)
  , matcher_(nullptr)
  , model_(new LauncherModel())
  , sort_priority_(0)
  , show_desktop_icon_(false)
  , display_(display)
{
  UScreen* uscreen = UScreen::GetDefault();
  auto monitors = uscreen->GetMonitors();

  launcher_open = false;
  launcher_keynav = false;
  launcher_grabbed = false;
  reactivate_keynav = false;

  int i = 0;
  for (auto monitor : monitors)
  {
    Launcher* launcher = CreateLauncher(i);
    launchers.push_back(nux::ObjectPtr<Launcher> (launcher));
    i++;
  }

  launcher_ = launchers[0];

  device_section_ = new DeviceLauncherSection();
  device_section_->IconAdded.connect(sigc::mem_fun(this, &Impl::OnIconAdded));

  num_workspaces_ = WindowManager::Default()->WorkspaceCount();
  if (num_workspaces_ > 1)
  {
    InsertExpoAction();
  }

  // Insert the "Show Desktop" launcher icon in the launcher...
  if (show_desktop_icon_)
    InsertDesktopIcon();

  InsertTrash();

  auto setup_bamf = [](gpointer user_data) -> gboolean
  {
    Impl* self = static_cast<Impl*>(user_data);
    self->SetupBamf();
    return FALSE;
  };
  bamf_timer_handler_id_ = g_timeout_add(500, setup_bamf, this);

  remote_model_.entry_added.connect(sigc::mem_fun(this, &Impl::OnLauncherEntryRemoteAdded));
  remote_model_.entry_removed.connect(sigc::mem_fun(this, &Impl::OnLauncherEntryRemoteRemoved));
  
  FavoriteStore::GetDefault().favorite_added.connect(sigc::mem_fun(this, &Impl::OnFavoriteStoreFavoriteAdded));
  FavoriteStore::GetDefault().favorite_removed.connect(sigc::mem_fun(this, &Impl::OnFavoriteStoreFavoriteRemoved));
  FavoriteStore::GetDefault().reordered.connect(sigc::mem_fun(this, &Impl::OnFavoriteStoreReordered));

  RegisterIcon(new BFBLauncherIcon());
  desktop_icon_ = new DesktopLauncherIcon();

  uscreen->changed.connect(sigc::mem_fun(this, &Controller::Impl::OnScreenChanged));

  launcher_key_press_time_ = { 0, 0 };

  ubus.RegisterInterest(UBUS_QUICKLIST_END_KEY_NAV, [&](GVariant * args) { 
    if (reactivate_keynav)
      parent_->KeyNavGrab();
      model_->SetSelection(reactivate_index);
  });
}

Controller::Impl::~Impl()
{
  if (bamf_timer_handler_id_ != 0)
    g_source_remove(bamf_timer_handler_id_);

  if (matcher_ != nullptr && on_view_opened_id_ != 0)
    g_signal_handler_disconnect((gpointer) matcher_, on_view_opened_id_);

  delete device_section_;
}

void Controller::Impl::OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors)
{
  unsigned int num_monitors = monitors.size();

  for (unsigned int i = 0; i < num_monitors; i++)
  {
    if (i >= launchers.size())
      launchers.push_back(nux::ObjectPtr<Launcher> (CreateLauncher(i)));
    
    launchers[i]->Resize();
  }

  launchers.resize(num_monitors);
}

Launcher* Controller::Impl::CreateLauncher(int monitor)
{
  nux::BaseWindow* launcher_window = new nux::BaseWindow(TEXT("LauncherWindow"));

  Launcher* launcher = new Launcher(launcher_window);
  launcher->display = display_;
  launcher->monitor = monitor;
  launcher->options = parent_->options();

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->AddView(launcher, 1);
  layout->SetContentDistribution(nux::eStackLeft);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  launcher_window->SetLayout(layout);
  launcher_window->SetBackgroundColor(nux::color::Transparent);
  launcher_window->ShowWindow(true);
  launcher_window->EnableInputWindow(true, "launcher", false, false);
  launcher_window->InputWindowEnableStruts(false);
  launcher_window->SetEnterFocusInputArea(launcher);

  launcher->SetModel(model_.get());
  launcher->launcher_addrequest.connect(sigc::mem_fun(this, &Impl::OnLauncherAddRequest));
  launcher->launcher_removerequest.connect(sigc::mem_fun(this, &Impl::OnLauncherRemoveRequest));

  parent_->AddChild(launcher);

  return launcher;
}

void Controller::Impl::OnLauncherAddRequest(char* path, AbstractLauncherIcon* before)
{
  for (auto it : model_->GetSublist<BamfLauncherIcon> ())
  {
    if (!g_strcmp0(path, it->DesktopFile()))
    {
      it->Stick();
      model_->ReorderBefore(it, before, false);
      Save();
      return;
    }
  }

  AbstractLauncherIcon* result = CreateFavorite(path);
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
  std::list<BamfLauncherIcon*> launchers = model_->GetSublist<BamfLauncherIcon> ();
  for (auto icon : launchers)
  {
    if (!icon->IsSticky())
      continue;

    const char* desktop_file = icon->DesktopFile();

    if (desktop_file && strlen(desktop_file) > 0)
      desktop_paths.push_back(desktop_file);
  }

  unity::FavoriteStore::GetDefault().SetFavorites(desktop_paths);
}

void
Controller::Impl::OnLauncherAddRequestSpecial(char* path, AbstractLauncherIcon* before, char* aptdaemon_trans_id, char* icon_path)
{
  std::list<BamfLauncherIcon*> launchers;
  std::list<BamfLauncherIcon*>::iterator it;

  launchers = model_->GetSublist<BamfLauncherIcon> ();
  for (it = launchers.begin(); it != launchers.end(); it++)
  {
    if (g_strcmp0(path, (*it)->DesktopFile()) == 0)
      return;
  }

  SoftwareCenterLauncherIcon* result = CreateSCLauncherIcon(path, aptdaemon_trans_id, icon_path);
  if (result)
  {
    RegisterIcon(result);

    if (before)
      model_->ReorderBefore(result, before, false);
  }
  Save();
}

void Controller::Impl::SortAndUpdate()
{
  gint   shortcut = 1;
  gchar* buff;

  std::list<BamfLauncherIcon*> launchers = model_->GetSublist<BamfLauncherIcon> ();
  for (auto it : launchers)
  {
    if (shortcut < 11 && it->GetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE))
    {
      buff = g_strdup_printf("%d", shortcut % 10);
      it->SetShortcut(buff[0]);
      g_free(buff);
      shortcut++;
    }
    // reset shortcut
    else
    {
      it->SetShortcut(0);
    }
  }
}

void Controller::Impl::OnIconAdded(AbstractLauncherIcon* icon)
{
  this->RegisterIcon(icon);
}

void Controller::Impl::OnIconRemoved(AbstractLauncherIcon* icon)
{
  SortAndUpdate();
}

void Controller::Impl::OnLauncherRemoveRequest(AbstractLauncherIcon* icon)
{
  switch (icon->Type())
  {
    case AbstractLauncherIcon::TYPE_APPLICATION:
    {
      BamfLauncherIcon* bamf_icon = dynamic_cast<BamfLauncherIcon*>(icon);

      if (bamf_icon)
      {
        bamf_icon->UnStick();
        bamf_icon->Quit();
      }

      break;
    }
    case AbstractLauncherIcon::TYPE_DEVICE:
    {
      DeviceLauncherIcon* device_icon = dynamic_cast<DeviceLauncherIcon*>(icon);

      if (device_icon && device_icon->CanEject())
        device_icon->Eject();

      break;
    }
    default:
      break;
  }
}

void Controller::Impl::OnLauncherEntryRemoteAdded(LauncherEntryRemote* entry)
{
  for (auto icon : *model_)
  {
    if (!icon || !icon->RemoteUri())
      continue;

    if (!g_strcmp0(entry->AppUri(), icon->RemoteUri()))
    {
      icon->InsertEntryRemote(entry);
    }
  }
}

void Controller::Impl::OnLauncherEntryRemoteRemoved(LauncherEntryRemote* entry)
{
  for (auto icon : *model_)
  {
    icon->RemoveEntryRemote(entry);
  }
}

void Controller::Impl::OnFavoriteStoreFavoriteAdded(std::string const& entry, std::string const& pos, bool before)
{  
  auto bamf_list = model_->GetSublist<BamfLauncherIcon>();  
  AbstractLauncherIcon* other = (bamf_list.size() > 0) ? *(bamf_list.begin()) : nullptr;
  
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

  AbstractLauncherIcon* result = CreateFavorite(entry.c_str());
  if (result)
  {
    RegisterIcon(result);
    if (!before)
      model_->ReorderAfter(result, other);
    else
      model_->ReorderBefore(result, other, false);
  }
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
  FavoriteList const& favs = FavoriteStore::GetDefault().GetFavorites();
  auto bamf_list = model_->GetSublist<BamfLauncherIcon>();
  
  int i = 0;
  for (auto it : favs)
  {    
    auto icon = std::find_if(bamf_list.begin(), bamf_list.end(),
    [&it](BamfLauncherIcon* x) { return (x->DesktopFile() == it); });
    
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
  TrashLauncherIcon* icon;
  icon = new TrashLauncherIcon();
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
  expo_icon_ = new SimpleLauncherIcon();

  expo_icon_->tooltip_text = _("Workspace Switcher");
  expo_icon_->icon_name = "workspace-switcher";
  expo_icon_->SetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE, true);
  expo_icon_->SetQuirk(AbstractLauncherIcon::QUIRK_RUNNING, false);
  expo_icon_->SetIconType(AbstractLauncherIcon::TYPE_EXPO);
  expo_icon_->SetShortcut('s');

  on_expoicon_activate_connection_ = expo_icon_->activate.connect(sigc::mem_fun(this, &Impl::OnExpoActivated));

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
  desktop_launcher_icon_ = new DesktopLauncherIcon();
  desktop_launcher_icon_->SetIconType(AbstractLauncherIcon::TYPE_DESKTOP);
  desktop_launcher_icon_->SetShowInSwitcher(false);

  RegisterIcon(desktop_launcher_icon_);
}

void Controller::Impl::RemoveDesktopIcon()
{
  model_->RemoveIcon(desktop_launcher_icon_);
}

void Controller::Impl::RegisterIcon(AbstractLauncherIcon* icon)
{
  model_->AddIcon(icon);

  BamfLauncherIcon* bamf_icon = dynamic_cast<BamfLauncherIcon*>(icon);
  if (bamf_icon)
  {
    LauncherEntryRemote* entry = NULL;
    const char* path;
    path = bamf_icon->DesktopFile();
    if (path)
      entry = remote_model_.LookupByDesktopFile(path);
    if (entry)
      icon->InsertEntryRemote(entry);
  }
}

/* static private */
void Controller::Impl::OnViewOpened(BamfMatcher* matcher, BamfView* view, gpointer data)
{
  Impl* self = static_cast<Impl*>(data);
  BamfApplication* app;

  if (!BAMF_IS_APPLICATION(view))
    return;

  app = BAMF_APPLICATION(view);

  if (bamf_view_is_sticky(view) ||
      g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
  {
    return;
  }

  BamfLauncherIcon* icon = new BamfLauncherIcon(app);
  icon->SetIconType(AbstractLauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority(self->sort_priority_++);

  self->RegisterIcon(icon);
}

AbstractLauncherIcon* Controller::Impl::CreateFavorite(const char* file_path)
{
  BamfApplication* app;
  BamfLauncherIcon* icon;

  app = bamf_matcher_get_application_for_desktop_file(matcher_, file_path, true);
  if (!app)
    return NULL;

  if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
  {
    bamf_view_set_sticky(BAMF_VIEW(app), true);
    return 0;
  }

  g_object_set_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen"), GINT_TO_POINTER(1));

  bamf_view_set_sticky(BAMF_VIEW(app), true);
  icon = new BamfLauncherIcon(app);
  icon->SetIconType(AbstractLauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority(sort_priority_++);

  return icon;
}

SoftwareCenterLauncherIcon*
Controller::Impl::CreateSCLauncherIcon(const char* file_path, const char* aptdaemon_trans_id, char* icon_path)
{
  BamfApplication* app;
  SoftwareCenterLauncherIcon* icon;

  app = bamf_matcher_get_application_for_desktop_file(matcher_, file_path, true);
  if (!BAMF_IS_APPLICATION(app))
    return NULL;

  if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
  {
    bamf_view_set_sticky(BAMF_VIEW(app), true);
    return 0;
  }

  g_object_set_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen"), GINT_TO_POINTER(1));

  bamf_view_set_sticky(BAMF_VIEW(app), true);
  icon = new SoftwareCenterLauncherIcon(app, (char*)aptdaemon_trans_id, icon_path);
  icon->SetIconType(LauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority(sort_priority_++);

  return icon;
}

void Controller::Impl::SetupBamf()
{
  GList* apps, *l;
  BamfApplication* app;
  BamfLauncherIcon* icon;

  // Sufficiently large number such that we ensure proper sorting
  // (avoids case where first item gets tacked onto end rather than start)
  int priority = 100;

  matcher_ = bamf_matcher_get_default();

  FavoriteList const& favs = FavoriteStore::GetDefault().GetFavorites();

  for (FavoriteList::const_iterator i = favs.begin(), end = favs.end();
       i != end; ++i)
  {
    AbstractLauncherIcon* fav = CreateFavorite(i->c_str());

    if (fav)
    {
      fav->SetSortPriority(priority);
      RegisterIcon(fav);
      priority++;
    }
  }

  apps = bamf_matcher_get_applications(matcher_);
  on_view_opened_id_ = g_signal_connect(matcher_, "view-opened", (GCallback) &Impl::OnViewOpened, this);

  for (l = apps; l; l = l->next)
  {
    app = BAMF_APPLICATION(l->data);

    if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
      continue;
    g_object_set_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen"), GINT_TO_POINTER(1));

    icon = new BamfLauncherIcon(app);
    icon->SetSortPriority(sort_priority_++);
    RegisterIcon(icon);
  }
  SortAndUpdate();

  model_->order_changed.connect(sigc::mem_fun(this, &Impl::SortAndUpdate));
  model_->icon_removed.connect(sigc::mem_fun(this, &Impl::OnIconRemoved));
  model_->saved.connect(sigc::mem_fun(this, &Impl::Save));
  bamf_timer_handler_id_ = 0;
}

int Controller::Impl::TimeSinceLauncherKeyPress()
{
  struct timespec current;
  unity::TimeUtil::SetTimeStruct(&current);
  return unity::TimeUtil::TimeDelta(&current, &launcher_key_press_time_);
}

bool Controller::Impl::TapTimeUnderLimit()
{
  int time_difference = TimeSinceLauncherKeyPress();
  return time_difference < local::super_tap_duration;
}

void Controller::Impl::SendHomeActivationRequest()
{
  ubus.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST, g_variant_new("(sus)", "home.lens", 0, ""));
}

Controller::Controller(Display* display)
{
  options = Options::Ptr(new Options());

  // defaults must match XML file
  options()->tile_size = 54;
  options()->icon_size = 48;
  options()->backlight_mode = BACKLIGHT_ALWAYS_ON;
  options()->hide_mode = LAUNCHER_HIDE_DODGE_WINDOWS;
  options()->launch_animation = LAUNCH_ANIMATION_PULSE;
  options()->urgent_animation = URGENT_ANIMATION_WIGGLE;
  options()->edge_decay_rate = 7500;
  options()->edge_overcome_pressure = 7500;
  options()->edge_stop_velocity = 10000;
  options()->edge_reveal_pressure = 8000;
  
  // options must be set before creating pimpl which loads launchers
  pimpl = new Impl(display, this);
}

Controller::~Controller()
{
  delete pimpl;
}

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

std::vector<AbstractLauncherIcon*> Controller::GetAltTabIcons(bool current) const
{
  std::vector<AbstractLauncherIcon*> results;

  results.push_back(pimpl->desktop_icon_.GetPointer());

  for (auto icon : *(pimpl->model_))
    if (icon->ShowInSwitcher(current))
      results.push_back(icon);

  return results;
}

Window Controller::LauncherWindowId(int launcher) const
{
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

void Controller::HandleLauncherKeyPress()
{
  unity::TimeUtil::SetTimeStruct(&pimpl->launcher_key_press_time_);

  auto show_launcher = [](gpointer user_data) -> gboolean
  {
    Impl* self = static_cast<Impl*>(user_data);
    if (self->keyboard_launcher_.IsNull())
      self->keyboard_launcher_ = self->launchers[self->MonitorWithMouse()];
    
    if (self->launcher_hide_handler_id_ > 0)
    {
      g_source_remove(self->launcher_hide_handler_id_);
      self->launcher_hide_handler_id_ = 0;
    }

    self->keyboard_launcher_->ForceReveal(true);
    self->launcher_open = true;
    self->launcher_key_press_handler_id_ = 0;
    return FALSE;
  };
  pimpl->launcher_key_press_handler_id_ = g_timeout_add(local::super_tap_duration, show_launcher, pimpl);

  auto show_shortcuts = [](gpointer user_data) -> gboolean
  {
    Impl* self = static_cast<Impl*>(user_data);
    if (!self->launcher_keynav)
    {
      if (self->keyboard_launcher_.IsNull())
        self->keyboard_launcher_ = self->launchers[self->MonitorWithMouse()];

      self->keyboard_launcher_->ShowShortcuts(true);
      self->launcher_open = true;
      self->launcher_label_show_handler_id_ = 0;
    }
    return FALSE;
  };
  pimpl->launcher_label_show_handler_id_ = g_timeout_add(local::shortcuts_show_delay, show_shortcuts, pimpl);
}

void Controller::HandleLauncherKeyRelease()
{
  if (pimpl->TapTimeUnderLimit())
  {
    pimpl->SendHomeActivationRequest();
  }

  if (pimpl->launcher_label_show_handler_id_)
  {
    g_source_remove(pimpl->launcher_label_show_handler_id_);
    pimpl->launcher_label_show_handler_id_ = 0;
  }

  if (pimpl->launcher_key_press_handler_id_)
  {
    g_source_remove(pimpl->launcher_key_press_handler_id_);
    pimpl->launcher_key_press_handler_id_ = 0;
  }

  if (pimpl->keyboard_launcher_.IsValid())
  {
    pimpl->keyboard_launcher_->ShowShortcuts(false);

    int ms_since_show = pimpl->TimeSinceLauncherKeyPress();
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
      auto hide_launcher = [](gpointer user_data) -> gboolean
      {
        Impl *self = static_cast<Impl*>(user_data);
        if (self->keyboard_launcher_.IsValid())
        {
          self->keyboard_launcher_->ForceReveal(false);
          self->launcher_open = false;

          if (!self->launcher_keynav)
            self->keyboard_launcher_.Release();
        }

        self->launcher_hide_handler_id_ = 0;
        return FALSE;
      };

      pimpl->launcher_hide_handler_id_ = g_timeout_add(time_left, hide_launcher, pimpl);
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
      pimpl->launcher_key_press_time_ = { 0, 0 };
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
  KeyNavActivate();
  pimpl->keyboard_launcher_->GrabKeyboard();
  pimpl->launcher_grabbed = true;
  
  pimpl->launcher_key_press_connection_ = 
    pimpl->keyboard_launcher_->key_down.connect(sigc::mem_fun(pimpl, &Controller::Impl::ReceiveLauncherKeyPress));
  pimpl->launcher_event_outside_connection_ = 
    pimpl->keyboard_launcher_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(pimpl, &Controller::Impl::ReceiveMouseDownOutsideArea));
}

void Controller::KeyNavActivate()
{
  if (pimpl->launcher_keynav)
    return;
  
  pimpl->launcher_keynav = true;
  pimpl->keyboard_launcher_ = pimpl->launchers[pimpl->MonitorWithMouse()];

  pimpl->keyboard_launcher_->EnterKeyNavMode();
  pimpl->model_->SetSelection(0);

  pimpl->ubus.SendMessage(UBUS_LAUNCHER_START_KEY_SWTICHER, g_variant_new_boolean(true));
}

void Controller::KeyNavNext()
{
  pimpl->model_->SelectNext();
}

void Controller::KeyNavPrevious()
{
  pimpl->model_->SelectPrevious();
}

void Controller::KeyNavTerminate(bool activate)
{
  if (!pimpl->launcher_keynav)
    return;

  pimpl->keyboard_launcher_->ExitKeyNavMode();
  if (pimpl->launcher_grabbed)
  {
    pimpl->keyboard_launcher_->UnGrabKeyboard();
    pimpl->launcher_key_press_connection_.disconnect();
    pimpl->launcher_event_outside_connection_.disconnect();
    pimpl->launcher_grabbed = false;
  }

  if (activate)
    pimpl->model_->Selection()->Activate(ActionArg(ActionArg::LAUNCHER, 0));

  pimpl->launcher_keynav = false;
  if (!pimpl->launcher_open)
    pimpl->keyboard_launcher_.Release();
  
  pimpl->ubus.SendMessage(UBUS_LAUNCHER_END_KEY_SWTICHER, g_variant_new_boolean(true));
}

bool Controller::KeyNavIsActive() const
{
  return pimpl->launcher_keynav;
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
  .add("key_nav_is_grabbed", pimpl->launcher_grabbed);
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

      // esc/left (close quicklist or exit laucher key-focus)
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
    model_->Selection()->Activate(ActionArg(ActionArg::LAUNCHER, 0));
    parent_->KeyNavTerminate(false);
    break;

    default:
      break;
  }
}


} // namespace launcher
} // namespace unity
