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

#include "FavoriteStore.h"
#include "LauncherController.h"
#include "LauncherIcon.h"
#include "Launcher.h"
#include "PluginAdapter.h"
#include "TrashLauncherIcon.h"

#include <glib/gi18n-lib.h>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

LauncherController::LauncherController(Launcher* launcher, CompScreen *screen, nux::BaseWindow* window)
{
  _launcher = launcher;
  _window = window;
  _screen = screen;
  _model = new LauncherModel ();
  _sort_priority = 0;
  
  _launcher->SetModel (_model);
  _launcher->launcher_dropped.connect (sigc::mem_fun (this, &LauncherController::OnLauncherDropped));
  _favorite_store = FavoriteStore::GetDefault ();

  _place_section = new PlaceLauncherSection (_launcher);
  _place_section->IconAdded.connect (sigc::mem_fun (this, &LauncherController::OnIconAdded));
 
  _device_section = new DeviceLauncherSection (_launcher);
  _device_section->IconAdded.connect (sigc::mem_fun (this, &LauncherController::OnIconAdded));

  _num_workspaces = _screen->vpSize ().width ();
  if(_num_workspaces > 1)
  {
    InsertExpoAction ();
  }
  InsertTrash ();

  g_timeout_add (500, (GSourceFunc) &LauncherController::BamfTimerCallback, this);

  _remote_model = LauncherEntryRemoteModel::GetDefault();
  _remote_model->entry_added.connect   (sigc::mem_fun (this, &LauncherController::OnLauncerEntryRemoteAdded));
  _remote_model->entry_removed.connect (sigc::mem_fun (this, &LauncherController::OnLauncerEntryRemoteRemoved));
}

LauncherController::~LauncherController()
{
  _favorite_store->UnReference ();
  delete _place_section;
  delete _device_section;
}

void
LauncherController::OnLauncherDropped (char *path, LauncherIcon *before)
{
  std::list<BamfLauncherIcon *> launchers;
  std::list<BamfLauncherIcon *>::iterator it;

  launchers = _model->GetSublist<BamfLauncherIcon> ();
  for (it = launchers.begin (); it != launchers.end (); it++)
  {
    if (g_str_equal (path, (*it)->DesktopFile ()))
      return;
  }

  LauncherIcon *result = CreateFavorite (path);
  if (result)
  {
    RegisterIcon (result);
    _model->ReorderBefore (result, before, false);
  }
}

void
LauncherController::SortAndSave ()
{
  std::list<BamfLauncherIcon *> launchers;
  std::list<BamfLauncherIcon *>::iterator it;
  std::list<const char*> desktop_paths;
  gint   shortcut = 1;
  gchar *buff;
    
  launchers = _model->GetSublist<BamfLauncherIcon> ();
  for (it = launchers.begin (); it != launchers.end (); it++)
  {
    BamfLauncherIcon *icon = *it;
    
    if (shortcut < 11 && (*it)->GetQuirk (LauncherIcon::QUIRK_VISIBLE))
    {
      buff = g_strdup_printf ("%d", shortcut % 10);  
      (*it)->SetShortcut (buff[0]);
      g_free (buff);
      shortcut++;
    }
    // reset shortcut
    else
      (*it)->SetShortcut (0);
    

    if (!icon->IsSticky ())
      continue;
    
    const char* desktop_file = icon->DesktopFile ();
    
    if (desktop_file && strlen (desktop_file) > 0)
      desktop_paths.push_back (desktop_file);
  }
  
  _favorite_store->SetFavorites (desktop_paths);
}

void
LauncherController::OnIconAdded (LauncherIcon *icon)
{
  this->RegisterIcon (icon);
}

void
LauncherController::OnLauncerEntryRemoteAdded (LauncherEntryRemote *entry)
{
  LauncherModel::iterator it;
  for (it = _model->begin (); it != _model->end (); it++)
  {
    LauncherIcon *icon = *it;
  
    if (!icon || !icon->RemoteUri ())
      continue;
    
    if (!g_strcmp0 (entry->AppUri (), icon->RemoteUri ()))
    {
      icon->InsertEntryRemote (entry);
    }
  }
}

void
LauncherController::OnLauncerEntryRemoteRemoved (LauncherEntryRemote *entry)
{
  LauncherModel::iterator it;
  for (it = _model->begin (); it != _model->end (); it++)
  {
    LauncherIcon *icon = *it;
    icon->RemoveEntryRemote (entry);
  }
}

void
LauncherController::OnExpoClicked (int button)
{
  if (button == 1)
    PluginAdapter::Default ()->InitiateExpo ();
}

void
LauncherController::InsertTrash ()
{
  TrashLauncherIcon *icon;
  icon = new TrashLauncherIcon (_launcher);
  RegisterIcon (icon);
}

void
LauncherController::UpdateNumWorkspaces (int workspaces)
{
  if ((_num_workspaces == 0) && (workspaces > 0))
  {
    InsertExpoAction ();
  }
  else if((_num_workspaces > 0) && (workspaces == 0))
  {
    RemoveExpoAction ();
  }
  
  _num_workspaces = workspaces;
}

void
LauncherController::InsertExpoAction ()
{
  _expoIcon = new SimpleLauncherIcon (_launcher);
  
  _expoIcon->SetTooltipText (_("Workspace Switcher"));
  _expoIcon->SetIconName ("workspace-switcher");
  _expoIcon->SetQuirk (LauncherIcon::QUIRK_VISIBLE, true);
  _expoIcon->SetQuirk (LauncherIcon::QUIRK_RUNNING, false);
  _expoIcon->SetIconType (LauncherIcon::TYPE_EXPO);
  _expoIcon->SetShortcut('w');
  
  _expoIcon->MouseClick.connect (sigc::mem_fun (this, &LauncherController::OnExpoClicked));
  
  RegisterIcon (_expoIcon);
}

void
LauncherController::RemoveExpoAction ()
{
  _model->RemoveIcon (_expoIcon);
}

void
LauncherController::RegisterIcon (LauncherIcon *icon)
{
  _model->AddIcon (icon);
}

/* static private */
bool 
LauncherController::BamfTimerCallback (void *data)
{
  LauncherController *self = (LauncherController*) data;

  self->SetupBamf ();
  
  return false;
}

/* static private */
void
LauncherController::OnViewOpened (BamfMatcher *matcher, BamfView *view, gpointer data)
{
  LauncherController *self = (LauncherController *) data;
  BamfApplication *app;
  
  if (!BAMF_IS_APPLICATION (view))
    return;
  
  app = BAMF_APPLICATION (view);
  
  if (g_object_get_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen")))
    return;
  
  BamfLauncherIcon *icon = new BamfLauncherIcon (self->_launcher, app, self->_screen);
  icon->SetIconType (LauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority (self->_sort_priority++);

  self->RegisterIcon (icon);
}

LauncherIcon *
LauncherController::CreateFavorite (const char *file_path)
{
  BamfApplication *app;
  BamfLauncherIcon *icon;

  app = bamf_matcher_get_application_for_desktop_file (_matcher, file_path, true);
  if (!app)
    return NULL;
  
  if (g_object_get_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen")))
  {
    bamf_view_set_sticky (BAMF_VIEW (app), true);
    return 0;
  }
  
  g_object_set_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen"), GINT_TO_POINTER (1));
  
  bamf_view_set_sticky (BAMF_VIEW (app), true);
  icon = new BamfLauncherIcon (_launcher, app, _screen);
  icon->SetIconType (LauncherIcon::TYPE_APPLICATION);
  icon->SetSortPriority (_sort_priority++);
  
  return icon;
}

/* private */
void
LauncherController::SetupBamf ()
{
  GList *apps, *l;
  GSList *favs, *f;
  BamfApplication *app;
  BamfLauncherIcon *icon;
  int priority = 0;
  
  _matcher = bamf_matcher_get_default ();
  
  favs = FavoriteStore::GetDefault ()->GetFavorites ();
  
  for (f = favs; f; f = f->next)
  {
    LauncherIcon *fav = CreateFavorite ((const char *) f->data);
    
    if (fav)
    {
      fav->SetSortPriority (priority);
      RegisterIcon (fav);
      priority++;
    }
  }
  
  apps = bamf_matcher_get_applications (_matcher);
  g_signal_connect (_matcher, "view-opened", (GCallback) &LauncherController::OnViewOpened, this);
  
  for (l = apps; l; l = l->next)
  {
    app = BAMF_APPLICATION (l->data);
    
    if (g_object_get_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen")))
      continue;
    g_object_set_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen"), GINT_TO_POINTER (1));
    
    icon = new BamfLauncherIcon (_launcher, app, _screen);
    icon->SetSortPriority (_sort_priority++);
    RegisterIcon (icon);
  }
  
  _model->order_changed.connect (sigc::mem_fun (this, &LauncherController::SortAndSave));
}

