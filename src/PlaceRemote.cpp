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

#include "PlaceRemote.h"

#include <algorithm>
#include "PlaceEntryRemote.h"

#define PLACE_GROUP      "Place"
#define DBUS_NAME        "DBusName"
#define DBUS_PATH        "DBusObjectPath"

#define ENTRY_PREFIX     "Entry:"

#define ACTIVATION_GROUP "Activation"
#define URI_PATTERN      "URIPattern"
#define MIME_PATTERN     "MimetypePattern"

#define PLACE_IFACE "com.canonical.Unity.Place"
#define ACTIVE_IFACE "com.canonical.Unity.Activation"

static void on_service_proxy_ready (GObject      *source,
                                    GAsyncResult *result,
                                    gpointer      user_data);

static void on_service_proxy_signal_received (GDBusProxy *proxy,
                                              gchar      *sender_name,
                                              gchar      *signal_name,
                                              GVariant   *parameters,
                                              gpointer    user_data);

static void on_service_proxy_get_entries_received (GObject      *source,
                                                   GAsyncResult *result,
                                                   gpointer      user_data);

PlaceRemote::PlaceRemote (const char *path)
: _path (NULL),
  _dbus_name (NULL),
  _dbus_path (NULL),
  _uri_regex (NULL),
  _mime_regex (NULL),
  _valid (false),
  _service_proxy (NULL),
  _activation_proxy (NULL),
  _conn_attempt (false)
{
  GKeyFile *key_file;
  GError   *error = NULL;

  _path = g_strdup (path);

  // A .place file is a keyfile, so we create on representing the .place file to
  // read it's contents
  key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (key_file,
                                  path,
                                  G_KEY_FILE_NONE,
                                  &error))
  {
    g_warning ("Unable to load Place %s: %s",
               path,
               error ? error->message : "Unknown");
    if (error)
      g_error_free (error);

    return;
  }

  if (!g_key_file_has_group (key_file, PLACE_GROUP))
  {
    g_warning ("Unable to load Place %s: keyfile does not contain a 'Place' group",
               path);
    g_key_file_free (key_file);
    return;
  }

  // Seems valid, so let's try and load the main values
  _dbus_name = g_key_file_get_string (key_file, PLACE_GROUP, DBUS_NAME, &error);
  if (error)
  {
    g_warning ("Unable to load Place %s: keyfile does not contain a '" DBUS_NAME "' key, %s",
               path,
               error->message);
    g_error_free (error);
    g_key_file_free (key_file);
    return;
  }

  _dbus_path = g_key_file_get_string (key_file, PLACE_GROUP, DBUS_PATH, &error);
  if (error)
  {
    g_warning ("Unable to load Place %s: keyfile does not contain a '" DBUS_PATH "' key, %s",
               path,
               error->message);
    g_error_free (error);
    g_key_file_free (key_file);
    return;
  }

  // Now the optional bits
  if (g_key_file_has_group (key_file, ACTIVATION_GROUP))
  {
    gchar *uri_match;
    gchar *mime_match;

    uri_match = g_key_file_get_string (key_file, ACTIVATION_GROUP, URI_PATTERN, &error);
    if (error)
    {
      // Fail silently as it's not a requirement
      g_error_free (error);
      error = NULL;
    }
    else if (uri_match)
    {
      _uri_regex = g_regex_new (uri_match,
                                (GRegexCompileFlags)0,
                                (GRegexMatchFlags)0,
                                &error);
      if (error)
      {
        g_warning ("Unable to compile '"URI_PATTERN"' regex for %s: %s",
                   path,
                   error->message);
        g_error_free (error);
        error = NULL;
      }
    }

    mime_match = g_key_file_get_string (key_file, ACTIVATION_GROUP, MIME_PATTERN, &error);
    if (error)
    {
      // Fail silently as it's not a requirement
      g_error_free (error);
      error = NULL;
    }
    else if (mime_match)
    {
      _mime_regex = g_regex_new (mime_match,
                                (GRegexCompileFlags)0,
                                (GRegexMatchFlags)0,
                                &error);
      if (error)
      {
        g_warning ("Unable to compile '"MIME_PATTERN"' regex for %s: %s",
                   path,
                   error->message);
        g_error_free (error);
        error = NULL;
      }
    }

    g_free (uri_match);
    g_free (mime_match);
  }

  LoadKeyFileEntries (key_file);

  _valid = true;

  g_key_file_free (key_file);
}

PlaceRemote::~PlaceRemote ()
{
  std::vector<PlaceEntry*>::iterator it;
  
  for (it = _entries.begin(); it != _entries.end(); it++)
  {
    PlaceEntry *entry = static_cast<PlaceEntry *> (*it);
    delete entry;
  }

  _entries.erase (_entries.begin (), _entries.end ());

  g_free (_path);
  g_free (_dbus_name);
  g_free (_dbus_path);
  
  if (_uri_regex)
    g_regex_unref (_uri_regex);
  if (_mime_regex)
    g_regex_unref (_mime_regex);

  if (_service_proxy)
    g_object_unref (_service_proxy);

  if (_activation_proxy)
    g_object_unref (_activation_proxy);
}

const gchar * 
PlaceRemote::GetDBusPath ()
{
  return _dbus_path;
}

void
PlaceRemote::Connect ()
{
  // We do not connect the entries, or ourselves, to the the daemon automatically at startup to
  // increase the total login time of the desktop as we then reduce the number of things 
  // trashing the disk (i.e. zeitgeist/gwibber/etc) and taking CPU time
  //
  // What this means in to Unity is that it needs to make sure to have called Connect before
  // trying to do use the Place or it's PlaceEntry's
    
  if (!_conn_attempt && !G_IS_DBUS_PROXY (_service_proxy))
  {
    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                              G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                              NULL,
                              _dbus_name,
                              _dbus_path,
                              PLACE_IFACE,
                              NULL,
                              on_service_proxy_ready,
                              this);

    if (_uri_regex || _mime_regex)
      g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                NULL,
                                _dbus_name,
                                _dbus_path,
                                ACTIVE_IFACE,
                                NULL,
                                (GAsyncReadyCallback)PlaceRemote::OnActivationProxyReady,
                                this);

    std::vector<PlaceEntry *>::iterator it, eit = _entries.end ();
    for (it = _entries.begin (); it != eit; ++it)
    {
      PlaceEntryRemote *entry = static_cast<PlaceEntryRemote *> (*it);
      entry->Connect ();
    }

    _conn_attempt = true;
  }
}

std::vector<PlaceEntry *>&
PlaceRemote::GetEntries ()
{
  return _entries;
}

guint32
PlaceRemote::GetNEntries ()
{
  return _entries.size ();
}

void
PlaceRemote::LoadKeyFileEntries (GKeyFile *key_file)
{
  gchar **groups;
  gint    i = 0;

  groups = g_key_file_get_groups (key_file, NULL);

  while (groups[i])
  {
    const gchar *group = groups[i];

    if (g_str_has_prefix (group, ENTRY_PREFIX))
    {
      PlaceEntryRemote *entry = new PlaceEntryRemote (this, _dbus_name);
      entry->InitFromKeyFile (key_file, group);
      
      if (entry->IsValid ())
      {
        _entries.push_back (entry);
        entry_added.emit (entry);
      }
      else
        delete entry;

    }

    i++;
  }

  g_strfreev (groups);
}

bool
PlaceRemote::IsValid ()
{
  return _valid;
}

void
PlaceRemote::OnServiceProxyReady (GObject *source, GAsyncResult *result)
{
  GError *error = NULL;
  gchar  *name_owner = NULL;

  _service_proxy = g_dbus_proxy_new_for_bus_finish (result, &error);
  name_owner = g_dbus_proxy_get_name_owner (_service_proxy);
  _conn_attempt = false;

  if (error || !name_owner)
  {
    g_warning ("Unable to connect to PlaceRemote %s: %s",
               _dbus_name,
               error ? error->message : "No name owner");
    if (error)
      g_error_free (error);

    g_free (name_owner);
    return;
  }

  g_signal_connect (_service_proxy, "g-signal",
                    G_CALLBACK (on_service_proxy_signal_received), this);
  g_signal_connect (_service_proxy, "notify::g-name-owner",
                    G_CALLBACK (PlaceRemote::OnProxyNameOwnerChanged), this);
  g_dbus_proxy_call (_service_proxy,
                     "GetEntries",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1, 
                     NULL,
                     on_service_proxy_get_entries_received,
                     this);
  
  g_free (name_owner);
}

void
PlaceRemote::OnProxyNameOwnerChanged (GDBusProxy  *proxy,
                                      GParamSpec  *pspec,
                                      PlaceRemote *self)
{
  gchar *name_owner  = g_dbus_proxy_get_name_owner (proxy);

  if (!name_owner)
  {
    // Remote proxy has died
    g_debug ("Remote PlaceRemote proxy %s no longer exists, reconnecting", self->_dbus_name);
    g_object_unref (self->_service_proxy);
    g_object_unref (self->_activation_proxy);
    self->_service_proxy = NULL;
    self->_activation_proxy = NULL;

    self->Connect ();
  }
  else
    g_free (name_owner);
}

void
PlaceRemote::OnEntriesReceived (GVariant *args)
{
  GVariantIter *iter;
  gchar        *dbus_path;
  gchar        *name;
  gchar        *icon;
  guint32       position;
  GVariantIter *mimetypes;
  gboolean      sensitive;
  gchar        *sections_model;
  GVariantIter *hints;
  gchar        *entry_renderer;
  gchar        *entry_groups_model;
  gchar        *entry_results_model;
  GVariantIter *entry_hints;
  gchar        *global_renderer;
  gchar        *global_groups_model;
  gchar        *global_results_model;
  GVariantIter *global_hints;
  std::vector<PlaceEntry*>::iterator it;
  std::vector<PlaceEntry*> old;

   // Clear the main entries vector as we now need to figure out if there is a diff between
  // what was sent and what we loaded from the place file
  for (it = _entries.begin(); it != _entries.end(); it++)
  {
    PlaceEntryRemote *entry = static_cast<PlaceEntryRemote *> (*it);

    entry->dirty = true;
  }
  
  g_variant_get (args, "(a(sssuasbsa{ss}(sssa{ss})(sssa{ss})))", &iter);
  while (g_variant_iter_loop (iter, "(sssuasbsa{ss}(sssa{ss})(sssa{ss}))",
                              &dbus_path,
                              &name,
                              &icon,
                              &position,
                              &mimetypes,
                              &sensitive,
                              &sections_model,
                              &hints,
                              &entry_renderer,
                              &entry_groups_model,
                              &entry_results_model,
                              &entry_hints,
                              &global_renderer,
                              &global_groups_model,
                              &global_results_model,
                              &global_hints))
  {
    PlaceEntryRemote *existing = NULL;

    for (it = _entries.begin (); it != _entries.end (); it++)
    {
      PlaceEntryRemote *entry = static_cast<PlaceEntryRemote *> (*it);

      if (g_strcmp0 (entry->GetPath (), dbus_path) == 0)
      {
        existing = entry;
        break;
      }
    }

    if (!existing)
    {
      existing = new PlaceEntryRemote (this, _dbus_name);

      _entries.push_back (existing);
      entry_added.emit (existing);
    }

    existing->Update (dbus_path,
                      name,
                      icon,
                      position,
                      mimetypes,
                      sensitive,
                      sections_model,
                      hints,
                      entry_renderer,
                      entry_groups_model,
                      entry_results_model,
                      entry_hints,
                      global_renderer,
                      global_groups_model,
                      global_results_model,
                      global_hints);

    existing->dirty = false;
  }

  for (it = _entries.begin (); it != _entries.end (); it++)
  {
    PlaceEntryRemote *entry = static_cast<PlaceEntryRemote *> (*it);
    if (entry->dirty)
      old.push_back (entry);
  }
  for (it = old.begin (); it != old.end (); it++)
  {
    PlaceEntryRemote *entry = static_cast<PlaceEntryRemote *> (*it);
    std::vector<PlaceEntry *>::iterator i;

    if ((i = std::find (_entries.begin (), _entries.end (), entry)) != _entries.end ())
    {
       entry_removed.emit (entry);
      _entries.erase (i);
      delete entry;
    }
  }

  g_variant_iter_free (iter);
}

void
PlaceRemote::OnEntryAdded (GVariant *args)
{
  gchar        *dbus_path = NULL;
  gchar        *name = NULL;
  gchar        *icon = NULL;
  guint32       position = 0;
  GVariantIter *mimetypes = NULL;
  gboolean      sensitive = true;
  gchar        *sections_model = NULL;
  GVariantIter *hints = NULL;
  gchar        *entry_renderer = NULL;
  gchar        *entry_groups_model = NULL;
  gchar        *entry_results_model = NULL;
  GVariantIter *entry_hints = NULL;
  gchar        *global_renderer = NULL;
  gchar        *global_groups_model = NULL;
  gchar        *global_results_model = NULL;
  GVariantIter *global_hints = NULL;
  PlaceEntryRemote *entry;

  g_variant_get (args, "(sssuasbsa{ss}(sssa{ss})(sssa{ss}))",
                 &dbus_path,
                 &name,
                 &icon,
                 &position,
                 &mimetypes,
                 &sensitive,
                 &sections_model,
                 &hints,
                 &entry_renderer,
                 &entry_groups_model,
                 &entry_results_model,
                 &entry_hints,
                 &global_renderer,
                 &global_groups_model,
                 &global_results_model,
                 &global_hints);

  entry = new PlaceEntryRemote (this, _dbus_name);
  entry->Update (dbus_path,
                 name,
                 icon,
                 position,
                 mimetypes,
                 sensitive,
                 sections_model,
                 hints,
                 entry_renderer,
                 entry_groups_model,
                 entry_results_model,
                 entry_hints,
                 global_renderer,
                 global_groups_model,
                 global_results_model,
                 global_hints);
  
  _entries.push_back (entry);
  entry_added.emit (entry);
  
  g_free (dbus_path);
  g_free (name);
  g_free (icon);
  g_variant_iter_free (mimetypes);
  g_free (sections_model);
  g_variant_iter_free (hints);
  g_free (entry_renderer);
  g_free (entry_groups_model);
  g_free (entry_results_model);
  g_variant_iter_free (entry_hints);
  g_free (global_renderer);
  g_free (global_groups_model);
  g_free (global_results_model);
  g_variant_iter_free (global_hints);
}

void
PlaceRemote::OnEntryRemoved (const gchar *dbus_path)
{
  std::vector<PlaceEntry *>::iterator it;

  for (it = _entries.begin (); it != _entries.end (); it++)
  {
    PlaceEntryRemote *entry = static_cast<PlaceEntryRemote *> (*it);

    if (g_strcmp0 (entry->GetPath (), dbus_path) == 0)
    {
      entry_removed.emit (entry);
      _entries.erase (it);
      delete entry;
      break;
    }
  }
}

void
PlaceRemote::OnActivationResultReceived (GObject      *source,
                                         GAsyncResult *result,
                                         PlaceRemote  *self)
{
  GVariant    *args;
  GError      *error = NULL;
  guint32      ret;

  args = g_dbus_proxy_call_finish ((GDBusProxy *)source, result, &error);
  if (error)
  {
    g_warning ("Unable to call Activate() on: %s",
               error->message);
    g_error_free (error);
    return;
  }
  
  g_variant_get (args, "(u)", &ret);
  self->result_activated.emit (self->_active_uri.c_str (), (ActivationResult)ret);

  g_variant_unref (args);
}

void
PlaceRemote::ActivateResult (const char *uri, const char *mimetype)
{
  if (G_IS_DBUS_PROXY (_activation_proxy)
      && ((_uri_regex && g_regex_match (_uri_regex, uri, (GRegexMatchFlags)0, NULL))
          || (_mime_regex && g_regex_match (_mime_regex, mimetype, (GRegexMatchFlags)0, NULL))))
  {
    _active_uri = uri;
    g_dbus_proxy_call (_activation_proxy,
                       "Activate",
                       g_variant_new ("(s)", uri),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, 
                       NULL,
                       (GAsyncReadyCallback)OnActivationResultReceived,
                       this);
  }
  else
  {
    result_activated.emit (uri, FALLBACK);
  }
}

void
PlaceRemote::OnActivationProxyReady (GObject      *source,
                                     GAsyncResult *result,
                                     PlaceRemote  *self)
{
  GError *error = NULL;
  gchar  *name_owner = NULL;

  self->_activation_proxy = g_dbus_proxy_new_for_bus_finish (result, &error);
  name_owner = g_dbus_proxy_get_name_owner (self->_activation_proxy);

  if (error || !name_owner)
  {
    g_warning ("Unable to connect to PlaceRemote Activation %s: %s",
               self->_dbus_name,
               error ? error->message : "No name owner");
    if (error)
      g_error_free (error);
  }

  g_free (name_owner);
}


/*
 * C callbacks
 */
static void
on_service_proxy_ready (GObject      *source,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  PlaceRemote *self = static_cast<PlaceRemote *> (user_data);

  self->OnServiceProxyReady (source, result);
}

static void
on_service_proxy_signal_received (GDBusProxy *proxy,
                                  gchar      *sender_name,
                                  gchar      *signal_name,
                                  GVariant   *parameters,
                                  gpointer    user_data)
{ 
  PlaceRemote *self = static_cast<PlaceRemote *> (user_data);
  
  if (g_strcmp0 (signal_name, "EntryAdded") == 0)
  {
    self->OnEntryAdded (parameters);
  }
  else if (g_strcmp0 (signal_name, "EntryRemoved") == 0)
  {
    self->OnEntryRemoved (g_variant_get_string (g_variant_get_child_value (parameters, 0), NULL));
  }
}

static void
on_service_proxy_get_entries_received (GObject      *source,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  PlaceRemote *self = static_cast<PlaceRemote *> (user_data);
  GVariant    *args;
  GError      *error = NULL;

  args = g_dbus_proxy_call_finish ((GDBusProxy *)source, result, &error);
  if (error)
  {
    g_warning ("Unable to call GetEntries() on: %s",
               error->message);
    g_error_free (error);
    return;
  }

  self->OnEntriesReceived (args);

  g_variant_unref (args);
}
