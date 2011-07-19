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

#include "TrashLauncherIcon.h"
#include "Launcher.h"
#include "Nux/WindowCompositor.h"

#include "QuicklistManager.h"
#include "QuicklistMenuItemLabel.h"

#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <gconf/gconf-client.h>

#define ASK_CONFIRMATION_KEY "/apps/nautilus/preferences/confirm_trash"

TrashLauncherIcon::TrashLauncherIcon (Launcher* IconManager)
:   SimpleLauncherIcon(IconManager)
{
  tooltip_text = _("Trash");
  SetIconName ("user-trash");
  SetQuirk (QUIRK_VISIBLE, true);
  SetQuirk (QUIRK_RUNNING, false);
  SetIconType (TYPE_TRASH);
  SetShortcut ('t');
  _confirm_dialog = NULL;
  _on_confirm_dialog_close_id = 0;

  GFile *location = g_file_new_for_uri ("trash:///");
  
  m_TrashMonitor = g_file_monitor_directory (location,
                                             G_FILE_MONITOR_NONE,
                                             NULL,
                                             NULL);

  _on_trash_changed_handler_id = g_signal_connect (m_TrashMonitor,
                                                   "changed",
                                                   G_CALLBACK (&TrashLauncherIcon::OnTrashChanged),
                                                   this);

  UpdateTrashIcon ();
  
  g_object_unref (location);
}

TrashLauncherIcon::~TrashLauncherIcon()
{
  if (_on_trash_changed_handler_id != 0)
    g_signal_handler_disconnect ((gpointer) m_TrashMonitor,
                                  _on_trash_changed_handler_id);

  g_object_unref (m_TrashMonitor);
  
  if (_on_confirm_dialog_close_id)
    g_signal_handler_disconnect ((gpointer) _confirm_dialog, _on_confirm_dialog_close_id);

  if (_confirm_dialog)
    gtk_widget_destroy (_confirm_dialog);
}

nux::Color 
TrashLauncherIcon::BackgroundColor ()
{
  return nux::Color (0xFF333333);
}

nux::Color 
TrashLauncherIcon::GlowColor ()
{
  return nux::Color (0xFF333333);
}

std::list<DbusmenuMenuitem *>
TrashLauncherIcon::GetMenus ()
{
  std::list<DbusmenuMenuitem *> result;
  DbusmenuMenuitem *menu_item;
  
  /* Empty Trash */
  menu_item = dbusmenu_menuitem_new ();
  g_object_ref (menu_item);

  dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Empty Trash..."));
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, !_empty);
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  g_signal_connect (menu_item,
                    DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, 
                    (GCallback)&TrashLauncherIcon::OnEmptyTrash, this);
  result.push_back(menu_item);
  
  return result;
}

void
TrashLauncherIcon::ActivateLauncherIcon (ActionArg arg)
{
  GError *error = NULL;

  g_spawn_command_line_async ("xdg-open trash://", &error);

  if (error)
    g_error_free (error);
}

void
TrashLauncherIcon::OnConfirmDialogClose (GtkDialog *dialog,
                                         gint response,
                                         gpointer user_data)
{
  TrashLauncherIcon *self = (TrashLauncherIcon*)user_data;
  
  if (response == GTK_RESPONSE_OK)
    g_thread_create ((GThreadFunc)&TrashLauncherIcon::EmptyTrashAction, NULL, FALSE, NULL);

  if (self->_confirm_dialog)
    gtk_widget_destroy (GTK_WIDGET(self->_confirm_dialog));

  self->_confirm_dialog = NULL;
  self->_on_confirm_dialog_close_id = 0;
}

void 
TrashLauncherIcon::OnEmptyTrash(DbusmenuMenuitem *item, int time, TrashLauncherIcon *self)
{
  GConfClient *client;
  bool        ask_confirmation;
  
  if (self->_confirm_dialog != NULL) {
    gtk_window_present_with_time (GTK_WINDOW (self->_confirm_dialog), time);
    return;
  }

  client = gconf_client_get_default ();
  ask_confirmation = gconf_client_get_bool (client, ASK_CONFIRMATION_KEY, NULL);
  g_object_unref (client);

  if (ask_confirmation)
  {
    self->_confirm_dialog = gtk_message_dialog_new (NULL, GtkDialogFlags(0),
                                                    GTK_MESSAGE_WARNING,
                                                    GTK_BUTTONS_CANCEL,
                                                    NULL);

    g_object_set (GTK_DIALOG (self->_confirm_dialog),
		  "text", _("Empty all items from Trash?"),
		  "secondary-text", _("All items in the Trash will be permanently deleted."),
		  NULL);
    gtk_dialog_add_button (GTK_DIALOG (self->_confirm_dialog), _("Empty Trash"), GTK_RESPONSE_OK);
    self->_on_confirm_dialog_close_id = g_signal_connect (self->_confirm_dialog, "response", (GCallback)&TrashLauncherIcon::OnConfirmDialogClose, self);
    gtk_widget_show_all (self->_confirm_dialog);
  }

  QuicklistManager::Default ()->HideQuicklist (self->_quicklist);

  if (!ask_confirmation) 
    g_thread_create ((GThreadFunc)&TrashLauncherIcon::EmptyTrashAction, NULL, FALSE, NULL);

}

void
TrashLauncherIcon::EmptyTrashAction()
{
  // This function runs in a different thread
  // created in TrashLauncherIcon::OnEmptyTrash
  GFile *location;
  location = g_file_new_for_uri ("trash:///");
  
  RecursiveDelete (location);

  g_object_unref (location);

}
void
TrashLauncherIcon::RecursiveDelete(GFile *location)
{

  GFileInfo *info;
  GFile *child;
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (location,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME "," 
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
					                                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
					                                NULL,
					                                NULL);
  if (enumerator)
  {
    while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL)
    {
      child = g_file_get_child (location, g_file_info_get_name (info));
      if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) RecursiveDelete (child);

      g_file_delete (child, NULL, NULL);
      g_object_unref (child);
      g_object_unref (info);
       
    }

    g_file_enumerator_close (enumerator, NULL, NULL);
    g_object_unref (enumerator);
  }

}

void
TrashLauncherIcon::UpdateTrashIcon ()
{
  GFile *location;
  location = g_file_new_for_uri ("trash:///");

  g_file_query_info_async (location,
                           G_FILE_ATTRIBUTE_STANDARD_ICON, 
                           G_FILE_QUERY_INFO_NONE, 
                           0, 
                           NULL, 
                           &TrashLauncherIcon::UpdateTrashIconCb, 
                           this);

  g_object_unref (location);
}

void
TrashLauncherIcon::UpdateTrashIconCb (GObject      *source,
                                      GAsyncResult *res,
                                      gpointer      data)
{
  TrashLauncherIcon *self = (TrashLauncherIcon*) data;
  GFileInfo *info;
  GIcon *icon;
  gchar *icon_name;

  // FIXME: should use the generic LoadIcon function (not taking from the unity theme)
  info = g_file_query_info_finish (G_FILE (source), res, NULL);

  if (info != NULL) {
    icon = g_file_info_get_icon (info);
    icon_name = g_icon_to_string (icon);
    self->SetIconName (icon_name);

    if (g_strcmp0 (icon_name, "user-trash") == 0) self->_empty = TRUE;
    else self->_empty = FALSE;

    g_object_unref(info);
    g_free(icon_name);
  }
}

void
TrashLauncherIcon::OnTrashChanged (GFileMonitor        *monitor,
                                   GFile               *file,
                                   GFile               *other_file,
                                   GFileMonitorEvent    event_type,
                                   gpointer             data)
{
    TrashLauncherIcon *self = (TrashLauncherIcon*) data;
    self->UpdateTrashIcon ();
}

nux::DndAction 
TrashLauncherIcon::OnQueryAcceptDrop (std::list<char *> uris)
{
  return nux::DNDACTION_MOVE;
}
  
void 
TrashLauncherIcon::OnAcceptDrop (std::list<char *> uris)
{
  std::list<char *>::iterator it;
  
  for (it = uris.begin (); it != uris.end (); it++)
  {
    GFile *file = g_file_new_for_uri (*it);
    g_file_trash (file, NULL, NULL);
    g_object_unref (file);
  }
}
