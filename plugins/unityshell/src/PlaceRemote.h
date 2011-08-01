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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACE_REMOTE_H
#define PLACE_REMOTE_H

#include <glib.h>
#include <gio/gio.h>

#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include "Place.h"

class PlaceRemote : public Place
{
public:
  PlaceRemote(const char* path);
  ~PlaceRemote();

  std::vector<PlaceEntry*>& GetEntries();
  guint32                    GetNEntries();

  bool IsValid();

  /* Callbacks */
  void OnServiceProxyReady(GObject* source, GAsyncResult* result);
  void OnEntriesReceived(GVariant* args);

  void OnEntryAdded(GVariant*    args);
  void OnEntryRemoved(const gchar* dbus_path);

  const gchar* GetDBusPath();

  void ActivateResult(const char* uri, const char* mimetype);
  void Connect();

private:
  void LoadKeyFileEntries(GKeyFile* key_file);

  static void OnActivationProxyReady(GObject*      source,
                                     GAsyncResult* result,
                                     PlaceRemote*  self);
  static void OnActivationResultReceived(GObject*      source,
                                         GAsyncResult* result,
                                         PlaceRemote*  self);
  static void OnProxyNameOwnerChanged(GDBusProxy*  proxy,
                                      GParamSpec*  pspec,
                                      PlaceRemote* self);
private:
  char*   _path;
  char*   _dbus_name;
  char*   _dbus_path;
  GRegex* _uri_regex;
  GRegex* _mime_regex;
  bool    _valid;

  GDBusProxy* _service_proxy;
  GDBusProxy* _activation_proxy;

  std::string _active_uri;

  bool    _conn_attempt;
};

#endif // PLACE_REMOTE_H
