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
/* Compiz */
#include <core/core.h>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Logger.h>

#include "BamfLauncherIcon.h"
#include "DeviceLauncherIcon.h"
#include "DeviceLauncherSection.h"
#include "FavoriteStore.h"
#include "Launcher.h"
#include "LauncherController.h"
#include "LauncherEntryRemote.h"
#include "LauncherEntryRemoteModel.h"
#include "LauncherIcon.h"
#include "LauncherModel.h"
#include "WindowManager.h"
#include "TrashLauncherIcon.h"
#include "BFBLauncherIcon.h"


namespace unity
{
namespace launcher
{
namespace
{
nux::logging::Logger logger("unity.launcher");
}

class Controller::Impl
{
public:
  Impl(Launcher* launcher);
  ~Impl();

  void UpdateNumWorkspaces(int workspaces);

private: //methods
  void Save();
  void SortAndUpdate();

  void OnIconAdded(LauncherIcon* icon);

  void OnLauncherAddRequest(char* path, LauncherIcon* before);
  void OnLauncherRemoveRequest(LauncherIcon* icon);

  void OnLauncherEntryRemoteAdded(LauncherEntryRemote* entry);
  void OnLauncherEntryRemoteRemoved(LauncherEntryRemote* entry);

  void InsertExpoAction();
  void RemoveExpoAction();

  void InsertTrash();

  void RegisterIcon(LauncherIcon* icon);

  LauncherIcon* CreateFavorite(const char* file_path);

  void SetupBamf();

  void OnExpoActivated();

  /* statics */

  static void OnViewOpened(BamfMatcher* matcher, BamfView* view, gpointer data);

private:
  BamfMatcher*           matcher_;
  Launcher*              launcher_;
  LauncherModel*         model_;
  int                    sort_priority_;
  DeviceLauncherSection* device_section_;
  LauncherEntryRemoteModel remote_model_;
  SimpleLauncherIcon*    expo_icon_;
  int                    num_workspaces_;

  guint            bamf_timer_handler_id_;

  guint32 on_view_opened_id_;

  sigc::connection on_expoicon_activate_connection_;
};



Controller::Impl::Impl(Launcher* launcher)
  : matcher_(nullptr)
  , sort_priority_(0)
{
  launcher_ = launcher;
  model_ = new LauncherModel();

  launcher_->SetModel(model_);
  launcher_->launcher_addrequest.connect(sigc::mem_fun(this, &Impl::OnLauncherAddRequest));
  launcher_->launcher_removerequest.connect(sigc::mem_fun(this, &Impl::OnLauncherRemoveRequest));

  device_section_ = new DeviceLauncherSection(launcher_);
  device_section_->IconAdded.connect(sigc::mem_fun(this, &Impl::OnIconAdded));

  num_workspaces_ = WindowManager::Default()->WorkspaceCount();
  if (num_workspaces_ > 1)
  {
    InsertExpoAction();
  }
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

  RegisterIcon (new BFBLauncherIcon (launcher));
}

Controller::Impl::~Impl()
{
  if (bamf_timer_handler_id_ != 0)
    g_source_remove(bamf_timer_handler_id_);

  if (matcher_ != NULL && on_view_opened_id_ != 0)
    g_signal_handler_disconnect((gpointer) matcher_, on_view_opened_id_);

  delete device_section_;
  delete model_;
}


void Controller::Impl::OnLauncherAddRequest(char* path, LauncherIcon* before)
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

  LauncherIcon* result = CreateFavorite(path);
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

void Controller::Impl::SortAndUpdate()
{
  gint   shortcut = 1;
  gchar* buff;

  std::list<BamfLauncherIcon*> launchers = model_->GetSublist<BamfLauncherIcon> ();
  for (auto it : launchers)
  {
    if (shortcut < 11 && it->GetQuirk(LauncherIcon::QUIRK_VISIBLE))
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

void Controller::Impl::OnIconAdded(LauncherIcon* icon)
{
  this->RegisterIcon(icon);
}

void Controller::Impl::OnLauncherRemoveRequest(LauncherIcon* icon)
{
  switch (icon->Type())
  {
    case LauncherIcon::TYPE_APPLICATION:
    {
      BamfLauncherIcon* bamf_icon = dynamic_cast<BamfLauncherIcon*>(icon);

      if (bamf_icon)
        bamf_icon->UnStick();

      break;
    }
    case LauncherIcon::TYPE_DEVICE:
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
  LauncherModel::iterator it;
  for (it = model_->begin(); it != model_->end(); it++)
  {
    LauncherIcon* icon = *it;

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
  LauncherModel::iterator it;
  for (it = model_->begin(); it != model_->end(); it++)
  {
    LauncherIcon* icon = *it;
    icon->RemoveEntryRemote(entry);
  }
}

void Controller::Impl::OnExpoActivated()
{
  WindowManager::Default()->InitiateExpo();
}

void Controller::Impl::InsertTrash()
{
  TrashLauncherIcon* icon;
  icon = new TrashLauncherIcon(launcher_);
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
  expo_icon_ = new SimpleLauncherIcon(launcher_);

  expo_icon_->tooltip_text = _("Workspace Switcher");
  expo_icon_->SetIconName("workspace-switcher");
  expo_icon_->SetQuirk(LauncherIcon::QUIRK_VISIBLE, true);
  expo_icon_->SetQuirk(LauncherIcon::QUIRK_RUNNING, false);
  expo_icon_->SetIconType(LauncherIcon::TYPE_EXPO);
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

void Controller::Impl::RegisterIcon(LauncherIcon* icon)
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

  if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
    return;

  BamfLauncherIcon* icon = new BamfLauncherIcon(self->launcher_, app);
  icon->SetIconType(LauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority(self->sort_priority_++);

  self->RegisterIcon(icon);
}

LauncherIcon* Controller::Impl::CreateFavorite(const char* file_path)
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
  icon = new BamfLauncherIcon(launcher_, app);
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
    LauncherIcon* fav = CreateFavorite(i->c_str());

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

    icon = new BamfLauncherIcon(launcher_, app);
    icon->SetSortPriority(sort_priority_++);
    RegisterIcon(icon);
  }
  SortAndUpdate();

  model_->order_changed.connect(sigc::mem_fun(this, &Impl::SortAndUpdate));
  model_->saved.connect(sigc::mem_fun(this, &Impl::Save));
  bamf_timer_handler_id_ = 0;
}


Controller::Controller(Launcher* launcher)
  : pimpl(new Impl(launcher))
{
}

Controller::~Controller()
{
  delete pimpl;
}

void Controller::UpdateNumWorkspaces(int workspaces)
{
  pimpl->UpdateNumWorkspaces(workspaces);
}


} // namespace launcher
} // namespace unity
