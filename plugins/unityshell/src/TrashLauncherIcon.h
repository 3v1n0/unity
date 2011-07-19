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

#ifndef TRASHLAUNCHERICON_H
#define TRASHLAUNCHERICON_H

#include "SimpleLauncherIcon.h"

class TrashLauncherIcon : public SimpleLauncherIcon
{

public:
  TrashLauncherIcon  (Launcher *launcher);
  ~TrashLauncherIcon ();
  
  virtual nux::Color BackgroundColor ();
  virtual nux::Color GlowColor ();

protected:
  void UpdateTrashIcon ();
  
  nux::DndAction OnQueryAcceptDrop (std::list<char *> uris);
  void OnAcceptDrop (std::list<char *> uris);

private:
  gulong _on_trash_changed_handler_id;
  gulong _on_confirm_dialog_close_id;
  GFileMonitor *m_TrashMonitor;
  gboolean _empty;
  GtkWidget   *_confirm_dialog;

  void ActivateLauncherIcon (ActionArg arg);
  std::list<DbusmenuMenuitem *> GetMenus ();

  static void UpdateTrashIconCb (GObject *source, GAsyncResult *res, gpointer data);
  static void OnTrashChanged (GFileMonitor *monitor, GFile *file, GFile *other_file,
                              GFileMonitorEvent event_type, gpointer data);
  static void OnEmptyTrash (DbusmenuMenuitem *item, int time, TrashLauncherIcon *self);
  static void OnConfirmDialogClose (GtkDialog *dialog, gint response, gpointer user_data);
  static void EmptyTrashAction ();
  static void RecursiveDelete (GFile *location);

};

#endif // TRASHLAUNCHERICON_H
