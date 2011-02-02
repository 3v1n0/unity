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
  _favorite_store = FavoriteStore::GetDefault ();

  _place_section = new PlaceLauncherSection (_launcher);
  _place_section->IconAdded.connect (sigc::mem_fun (this, &LauncherController::OnIconAdded));
 
  _device_section = new DeviceLauncherSection (_launcher);
  _device_section->IconAdded.connect (sigc::mem_fun (this, &LauncherController::OnIconAdded));

  InsertExpoAction ();
  InsertTrash ();

  g_timeout_add (500, (GSourceFunc) &LauncherController::BamfTimerCallback, this);

  _launcher->request_reorder_smart.connect (sigc::mem_fun (this, &LauncherController::OnLauncherRequestReorderSmart));
  _launcher->request_reorder_before.connect (sigc::mem_fun (this, &LauncherController::OnLauncherRequestReorderBefore));

  _remote_model = LauncherEntryRemoteModel::GetDefault();
  _remote_model->entry_added.connect (sigc::mem_fun (this, &LauncherController::OnLauncerEntryRemoteAdded));
}

LauncherController::~LauncherController()
{
  _favorite_store->UnReference ();
  delete _place_section;
  delete _device_section;
}

void
LauncherController::SortAndSave ()
{
  LauncherModel::iterator it;
  _model->Sort (&LauncherController::CompareIcons);
  
  std::list<const char*> desktop_paths;
  for (it = _model->begin (); it != _model->end (); it++)
  {
    BamfLauncherIcon *icon;
    icon = dynamic_cast<BamfLauncherIcon*> (*it);
    
    if (!icon)
      continue;
    
    if (!icon->IsSticky ())
      continue;
    
    const char* desktop_file = icon->DesktopFile ();
    
    if (desktop_file && strlen (desktop_file) > 0)
      desktop_paths.push_back (desktop_file);
  }
  
  _favorite_store->SetFavorites (desktop_paths);
}

void
LauncherController::OnLauncherRequestReorderBefore (LauncherIcon *icon, LauncherIcon *other, bool save)
{
  if (icon == other)
    return;

  LauncherModel::iterator it;
  
  int i = 0;
  int j = 0;
  for (it = _model->begin (); it != _model->end (); it++)
  {
    if ((*it) == icon)
    {
      j++;
      continue;
    }
    
    if ((*it) == other)
    {
      icon->SetSortPriority (i);
      if (i != j && save) (*it)->SaveCenter ();
      i++;
      
      (*it)->SetSortPriority (i);
      if (i != j && save) (*it)->SaveCenter ();
      i++;
    }
    else
    {
      (*it)->SetSortPriority (i);
      if (i != j && save) (*it)->SaveCenter ();
      i++;
    }
    j++;
  }
  
  SortAndSave ();
}

void
LauncherController::OnLauncherRequestReorderSmart (LauncherIcon *icon, LauncherIcon *other, bool save)
{
  if (icon == other)
    return;

  LauncherModel::iterator it;
  
  int i = 0;
  int j = 0;
  bool skipped = false;
  for (it = _model->begin (); it != _model->end (); it++)
  {
    if ((*it) == icon)
    {
      skipped = true;
      j++;
      continue;
    }
    
    if ((*it) == other)
    {
      if (!skipped)
      {
        icon->SetSortPriority (i);
        if (i != j && save) (*it)->SaveCenter ();
        i++;
      }
      
      (*it)->SetSortPriority (i);
      if (i != j && save) (*it)->SaveCenter ();
      i++;
      
      if (skipped)
      {
        icon->SetSortPriority (i);
        if (i != j && save) (*it)->SaveCenter ();
        i++;
      }
    }
    else
    {
      (*it)->SetSortPriority (i);
      if (i != j && save) (*it)->SaveCenter ();
      i++;
    }
    j++;
  }
  
  SortAndSave ();
}

void
LauncherController::OnIconAdded (LauncherIcon *icon)
{
  this->RegisterIcon (icon);
}

void
LauncherController::OnLauncerEntryRemoteAdded (LauncherEntryRemote *entry)
{
  g_debug ("LAUNCHER ENTRY ADDED: %s %s", entry->DBusName (), entry->AppUri ());
  // FIXME: Wire the signals on entry up to matching LauncherIcon from the LauncherModel
  
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
LauncherController::InsertExpoAction ()
{
  SimpleLauncherIcon *expoIcon;
  expoIcon = new SimpleLauncherIcon (_launcher);
  
  expoIcon->SetTooltipText (_("Workspace Switcher"));
  expoIcon->SetIconName ("workspace-switcher");
  expoIcon->SetQuirk (LauncherIcon::QUIRK_VISIBLE, true);
  expoIcon->SetQuirk (LauncherIcon::QUIRK_RUNNING, false);
  expoIcon->SetIconType (LauncherIcon::TYPE_EXPO);
  
  expoIcon->MouseClick.connect (sigc::mem_fun (this, &LauncherController::OnExpoClicked));
  
  RegisterIcon (expoIcon);
}

bool
LauncherController::CompareIcons (LauncherIcon *first, LauncherIcon *second)
{
  if (first->Type () < second->Type ())
    return true;
  else if (first->Type () > second->Type ())
    return false;
    
  return first->SortPriority () < second->SortPriority ();
}

void
LauncherController::RegisterIcon (LauncherIcon *icon)
{
  _model->AddIcon (icon);
  _model->Sort (&LauncherController::CompareIcons);
  
  LauncherModel::iterator it;
  
  int i = 0;
  for (it = _model->begin (); it != _model->end (); it++)
  {
    (*it)->SetSortPriority (i);
    i++;
  }
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
}

