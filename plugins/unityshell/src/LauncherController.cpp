// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 */

#include "DeviceLauncherIcon.h"
#include "FavoriteStore.h"
#include "LauncherController.h"
#include "LauncherIcon.h"
#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"
#include "PluginAdapter.h"
#include "TrashLauncherIcon.h"
#include "BFBLauncherIcon.h"

#include <glib/gi18n-lib.h>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

using namespace unity;

LauncherController::LauncherController(Launcher* launcher, CompScreen* screen)
{
  _launcher = launcher;
  _screen = screen;
  _model = new LauncherModel();
  _sort_priority = 0;

  _launcher->SetModel(_model);
  _launcher->launcher_addrequest.connect(sigc::mem_fun(this, &LauncherController::OnLauncherAddRequest));
  _launcher->launcher_addrequest_special.connect(sigc::mem_fun(this, &LauncherController::OnLauncherAddRequestSpecial));
  _launcher->launcher_removerequest.connect(sigc::mem_fun(this, &LauncherController::OnLauncherRemoveRequest));

  _place_section = new PlaceLauncherSection(_launcher);
  _place_section->IconAdded.connect(sigc::mem_fun(this, &LauncherController::OnIconAdded));

  _device_section = new DeviceLauncherSection(_launcher);
  _device_section->IconAdded.connect(sigc::mem_fun(this, &LauncherController::OnIconAdded));

  _num_workspaces = _screen->vpSize().width() * _screen->vpSize().height();
  if (_num_workspaces > 1)
  {
    InsertExpoAction();
  }
  InsertTrash();

  _bamf_timer_handler_id = g_timeout_add(500, (GSourceFunc) &LauncherController::BamfTimerCallback, this);

  _remote_model = LauncherEntryRemoteModel::GetDefault();
  _remote_model->entry_added.connect(sigc::mem_fun(this, &LauncherController::OnLauncherEntryRemoteAdded));
  _remote_model->entry_removed.connect(sigc::mem_fun(this, &LauncherController::OnLauncherEntryRemoteRemoved));

  RegisterIcon (new BFBLauncherIcon (launcher));
}

LauncherController::~LauncherController()
{
  if (_bamf_timer_handler_id != 0)
    g_source_remove(_bamf_timer_handler_id);

  if (_matcher != NULL && _on_view_opened_id != 0)
    g_signal_handler_disconnect((gpointer) _matcher, _on_view_opened_id);

  delete _place_section;
  delete _device_section;
  delete _model;
}

void
LauncherController::OnLauncherAddRequest(char* path, LauncherIcon* before)
{
  std::list<BamfLauncherIcon*> launchers;
  std::list<BamfLauncherIcon*>::iterator it;

  launchers = _model->GetSublist<BamfLauncherIcon> ();
  for (it = launchers.begin(); it != launchers.end(); it++)
  {
    if (!g_strcmp0(path, (*it)->DesktopFile()))
      return;
  }

  LauncherIcon* result = CreateFavorite(path);
  if (result)
  {
    RegisterIcon(result);
    if (before)
      _model->ReorderBefore(result, before, false);
  }
}

void
LauncherController::OnLauncherAddRequestSpecial(char* path, LauncherIcon* before, char* aptdaemon_trans_id)
{
  std::list<BamfLauncherIcon*> launchers;
  std::list<BamfLauncherIcon*>::iterator it;

  launchers = _model->GetSublist<BamfLauncherIcon> ();
  for (it = launchers.begin(); it != launchers.end(); it++)
  {
    if (!g_strcmp0(path, (*it)->DesktopFile()))
      return;
  }

  SoftwareCenterLauncherIcon* result = CreateSCLauncherIcon(path, aptdaemon_trans_id);
  if (result)
  {
    RegisterIcon((LauncherIcon*)result);
    if (before)
      _model->ReorderBefore(result, before, false);
  }
}

void LauncherController::SortAndUpdate()
{
  std::list<BamfLauncherIcon*> launchers;
  std::list<BamfLauncherIcon*>::iterator it;
  FavoriteList desktop_paths;
  gint   shortcut = 1;
  gchar* buff;

  launchers = _model->GetSublist<BamfLauncherIcon> ();
  for (it = launchers.begin(); it != launchers.end(); it++)
  {
    if (shortcut < 11 && (*it)->GetQuirk(LauncherIcon::QUIRK_VISIBLE))
    {
      buff = g_strdup_printf("%d", shortcut % 10);
      (*it)->SetShortcut(buff[0]);
      g_free(buff);
      shortcut++;
    }
    // reset shortcut
    else
    {
      (*it)->SetShortcut(0);
    }
  }
}

void
LauncherController::OnIconAdded(LauncherIcon* icon)
{
  this->RegisterIcon(icon);
}

void
LauncherController::OnLauncherRemoveRequest(LauncherIcon* icon)
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

void
LauncherController::OnLauncherEntryRemoteAdded(LauncherEntryRemote* entry)
{
  LauncherModel::iterator it;
  for (it = _model->begin(); it != _model->end(); it++)
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

void
LauncherController::OnLauncherEntryRemoteRemoved(LauncherEntryRemote* entry)
{
  LauncherModel::iterator it;
  for (it = _model->begin(); it != _model->end(); it++)
  {
    LauncherIcon* icon = *it;
    icon->RemoveEntryRemote(entry);
  }
}

void
LauncherController::OnExpoActivated()
{
  PluginAdapter::Default()->InitiateExpo();
}

void
LauncherController::InsertTrash()
{
  TrashLauncherIcon* icon;
  icon = new TrashLauncherIcon(_launcher);
  RegisterIcon(icon);
}

void
LauncherController::UpdateNumWorkspaces(int workspaces)
{
  if ((_num_workspaces == 0) && (workspaces > 0))
  {
    InsertExpoAction();
  }
  else if ((_num_workspaces > 0) && (workspaces == 0))
  {
    RemoveExpoAction();
  }

  _num_workspaces = workspaces;
}

void
LauncherController::InsertExpoAction()
{
  _expoIcon = new SimpleLauncherIcon(_launcher);

  _expoIcon->tooltip_text = _("Workspace Switcher");
  _expoIcon->SetIconName("workspace-switcher");
  _expoIcon->SetQuirk(LauncherIcon::QUIRK_VISIBLE, true);
  _expoIcon->SetQuirk(LauncherIcon::QUIRK_RUNNING, false);
  _expoIcon->SetIconType(LauncherIcon::TYPE_EXPO);
  _expoIcon->SetShortcut('s');

  _on_expoicon_activate_connection = _expoIcon->activate.connect(sigc::mem_fun(this, &LauncherController::OnExpoActivated));

  RegisterIcon(_expoIcon);
}

void
LauncherController::RemoveExpoAction()
{
  if (_on_expoicon_activate_connection)
    _on_expoicon_activate_connection.disconnect();
  _model->RemoveIcon(_expoIcon);
}

void
LauncherController::RegisterIcon(LauncherIcon* icon)
{
  _model->AddIcon(icon);

  BamfLauncherIcon* bamf_icon = dynamic_cast<BamfLauncherIcon*>(icon);
  if (bamf_icon)
  {
    LauncherEntryRemote* entry = NULL;
    const char* path;
    path = bamf_icon->DesktopFile();
    if (path)
      entry = _remote_model->LookupByDesktopFile(path);
    if (entry)
      icon->InsertEntryRemote(entry);
  }
}

/* static private */
bool
LauncherController::BamfTimerCallback(void* data)
{
  LauncherController* self = (LauncherController*) data;

  self->SetupBamf();

  self->_bamf_timer_handler_id = 0;
  return false;
}

/* static private */
void
LauncherController::OnViewOpened(BamfMatcher* matcher, BamfView* view, gpointer data)
{
  LauncherController* self = (LauncherController*) data;
  BamfApplication* app;

  if (!BAMF_IS_APPLICATION(view))
    return;

  app = BAMF_APPLICATION(view);

  if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
    return;

  BamfLauncherIcon* icon = new BamfLauncherIcon(self->_launcher, app, self->_screen);
  icon->SetIconType(LauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority(self->_sort_priority++);

  self->RegisterIcon(icon);
}

LauncherIcon*
LauncherController::CreateFavorite(const char* file_path)
{
  BamfApplication* app;
  BamfLauncherIcon* icon;

  app = bamf_matcher_get_application_for_desktop_file(_matcher, file_path, true);
  if (!app)
    return NULL;

  if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
  {
    bamf_view_set_sticky(BAMF_VIEW(app), true);
    return 0;
  }

  g_object_set_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen"), GINT_TO_POINTER(1));

  bamf_view_set_sticky(BAMF_VIEW(app), true);
  icon = new BamfLauncherIcon(_launcher, app, _screen);
  icon->SetIconType(LauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority(_sort_priority++);

  return icon;
}

SoftwareCenterLauncherIcon*
LauncherController::CreateSCLauncherIcon(const char* file_path, const char* aptdaemon_trans_id)
{
  BamfApplication* app;
  SoftwareCenterLauncherIcon* icon;

  app = bamf_matcher_get_application_for_desktop_file(_matcher, file_path, true);
  if (!app)
    return NULL;

  if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
  {
    bamf_view_set_sticky(BAMF_VIEW(app), true);
    return 0;
  }

  g_object_set_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen"), GINT_TO_POINTER(1));

  bamf_view_set_sticky(BAMF_VIEW(app), true);
  icon = new SoftwareCenterLauncherIcon(_launcher, app, _screen, (char*)aptdaemon_trans_id);
  icon->SetIconType(LauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority(_sort_priority++);

  return icon;
}

/* private */
void LauncherController::SetupBamf()
{
  GList* apps, *l;
  BamfApplication* app;
  BamfLauncherIcon* icon;

  // Sufficiently large number such that we ensure proper sorting 
  // (avoids case where first item gets tacked onto end rather than start)
  int priority = 100;

  _matcher = bamf_matcher_get_default();

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

  apps = bamf_matcher_get_applications(_matcher);
  _on_view_opened_id = g_signal_connect(_matcher, "view-opened", (GCallback) &LauncherController::OnViewOpened, this);

  for (l = apps; l; l = l->next)
  {
    app = BAMF_APPLICATION(l->data);

    if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
      continue;
    g_object_set_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen"), GINT_TO_POINTER(1));

    icon = new BamfLauncherIcon(_launcher, app, _screen);
    icon->SetSortPriority(_sort_priority++);
    RegisterIcon(icon);
  }

  _model->order_changed.connect(sigc::mem_fun(this, &LauncherController::SortAndUpdate));
}

