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

#include "PlaceEntryRemote.h"

#include <glib/gi18n-lib.h>

#define PLACE_ENTRY_IFACE "com.canonical.Unity.PlaceEntry"

#define DBUS_PATH "DBusObjectPath"

static void on_proxy_ready (GObject      *source,
                            GAsyncResult *result,
                            gpointer      user_data);

static void on_proxy_signal_received (GDBusProxy *proxy,
                                              gchar      *sender_name,
                                              gchar      *signal_name,
                                              GVariant   *parameters,
                                              gpointer    user_data);

PlaceEntryRemote::PlaceEntryRemote (const gchar *dbus_name)
: dirty (false),
  _dbus_path (NULL),
  _name (NULL),
  _icon (NULL),
  _description (NULL),
  _position (0),
  _mimetypes (NULL),
  _sensitive (true),
  _active (false),
  _valid (false),
  _show_in_launcher (true),
  _show_in_global (true),
  _proxy (NULL),
  _sections_model (NULL),
  _groups_model (NULL),
  _results_model (NULL)
{
  _dbus_name = g_strdup (dbus_name);
}

PlaceEntryRemote::~PlaceEntryRemote ()
{
  g_free (_name);
  g_free (_dbus_path);
  g_free (_icon);
  g_free (_description);
  g_strfreev (_mimetypes);
  
  g_object_unref (_proxy);

  g_object_unref (_sections_model);
  g_object_unref (_groups_model);
  g_object_unref (_results_model);
}

void
PlaceEntryRemote::InitFromKeyFile (GKeyFile    *key_file,
                                   const gchar *group)
{
  GError *error = NULL;
  gchar  *domain;
  gchar  *name;
  gchar  *description;

  _dbus_path = g_key_file_get_string (key_file, group, DBUS_PATH, &error);
  if (_dbus_path == NULL
      || g_strcmp0 (_dbus_path, "") == 0
      || _dbus_path[0] != '/'
      || error)
  {
    g_warning ("Unable to load PlaceEntry '%s': Does not contain valid '"DBUS_PATH"' (%s)",
               group,
               error ? error->message : "");
    if (error)
      g_error_free (error);
    return;
  }

  domain = g_key_file_get_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                  "X-Ubuntu-Gettext-Domain", &error);
  if (error)
  {
    // I'm messaging here because it probably should contain one
    g_message ("PlaceEntry %s does not contain a translation gettext name: %s",
               group,
               error->message);
    g_error_free (error);
    error = NULL;
  }

  name = g_key_file_get_string (key_file, group, G_KEY_FILE_DESKTOP_KEY_NAME, NULL);
  _icon = g_key_file_get_string (key_file, group, G_KEY_FILE_DESKTOP_KEY_ICON, NULL);
  description = g_key_file_get_string (key_file, group, "Description", NULL);

  if (domain)
  {
    _name = g_strdup (dgettext (domain, name));
    _description = g_strdup (dgettext (domain, description));
  }
  else
  {
    _name = g_strdup (name);
    _description = g_strdup (description);
  }

  /* Finally the two that should default to true */
  if (g_key_file_has_key (key_file, group, "ShowGlobal", NULL))
    _show_in_global = g_key_file_get_boolean (key_file, group, "ShowGlobal", NULL);

  if (g_key_file_has_key (key_file, group, "ShowEntry", NULL))
    _show_in_launcher = g_key_file_get_boolean (key_file, group, "ShowEntry", NULL);

  _valid = true;

  Connect ();

  g_debug ("PlaceEntry: %s", _name);

  g_free (name);
  g_free (description);
}

/* Overrides */
const gchar *
PlaceEntryRemote::GetId ()
{
  return _dbus_path;
}

const gchar *
PlaceEntryRemote::GetName ()
{
  return _name;
}

const gchar *
PlaceEntryRemote::GetIcon ()
{
  return _icon;
}

const gchar *
PlaceEntryRemote::GetDescription ()
{
  return _description;
}

guint32
PlaceEntryRemote::GetPosition  ()
{
  return _position;
}

const gchar **
PlaceEntryRemote::GetMimetypes ()
{
  return (const gchar **)_mimetypes;
}

const std::map<gchar *, gchar *>&
PlaceEntryRemote::GetHints ()
{
  return _hints;
}

bool
PlaceEntryRemote::IsSensitive ()
{
  return _sensitive;
}

bool
PlaceEntryRemote::IsActive ()
{
  return _active;
}

bool
PlaceEntryRemote::ShowInLauncher ()
{
  return _show_in_launcher;
}

bool
PlaceEntryRemote::ShowInGlobal ()
{
  return _show_in_global;
}

void
PlaceEntryRemote::SetActive (bool is_active)
{
  g_dbus_proxy_call (_proxy,
                     "SetActive",
                     g_variant_new ("(b)", (gboolean)is_active),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1, 
                     NULL,
                     NULL,
                     NULL);
  _active = is_active;

  active_changed.emit (is_active);
}

void
PlaceEntryRemote::SetSearch (const gchar *search, std::map<gchar*, gchar*>& hints)
{
  GVariantBuilder *builder;
  
  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{ss}"));
 // g_variant_builder_add (builder, "{ss}", "foo", "bar");

  /* FIXME: I'm ignoring hints because we don't use them currently */
  g_dbus_proxy_call (_proxy,
                     "SetSearch",
                     g_variant_new ("(sa{ss})", search, builder),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     NULL);
  g_variant_builder_unref (builder);
}

void
PlaceEntryRemote::SetActiveSection (guint32 section_id)
{
  g_dbus_proxy_call (_proxy,
                     "SetActiveSection",
                     g_variant_new ("(u)", section_id),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     NULL);
}

void
PlaceEntryRemote::SetGlobalSearch (const gchar *search, std::map<gchar*, gchar*>& hints)
{
  /* FIXME: I'm ignoring hints because we don't use them currently */
  g_dbus_proxy_call (_proxy,
                     "SetGlobalSearch",
                     g_variant_new ("(sa{ss})", search, NULL),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     NULL);
}

DeeModel *
PlaceEntryRemote::GetSectionsModel ()
{
  return _sections_model;
}

DeeModel *
PlaceEntryRemote::GetGroupsModel ()
{
  return _groups_model;
}

DeeModel *
PlaceEntryRemote::GetResultsModel ()
{
  return _results_model;
}


/* Other methods */
bool
PlaceEntryRemote::IsValid ()
{
  return _valid;
}

const gchar *
PlaceEntryRemote::GetPath ()
{
  return _dbus_path;
}

void
PlaceEntryRemote::Update (const gchar  *dbus_path,
                          const gchar  *name,
                          const gchar  *icon,
                          guint32       position,
                          GVariantIter *mimetypes,
                          gboolean      sensitive,
                          const gchar  *sections_model_name,
                          GVariantIter *hints,
                          const gchar  *entry_renderer,
                          const gchar  *entry_groups_model,
                          const gchar  *entry_results_model,
                          GVariantIter *entry_hints,
                          const gchar  *global_renderer,
                          const gchar  *global_groups_model,
                          const gchar  *global_results_model,
                          GVariantIter *global_hints)
{
  bool _state_changed           = false;
  bool _vis_changed             = false;
  bool _entry_renderer_changed  = false;
  bool _global_renderer_changed = false;

  if (_dbus_path)
    g_assert_cmpstr (_dbus_path, ==, dbus_path);
 
  // Hold on tight

  if (g_strcmp0 ("", name) != 0 && g_strcmp0 (_name, name) != 0)
  {
    g_free (_name);
    _name = g_strdup (name);
    _state_changed = true;
  }
  
  if (g_strcmp0 (_icon, icon) != 0)
  {
    g_free (_icon);
    _icon = g_strdup (icon);
    _state_changed = true;
  }

  if (_state_changed)
    state_changed.emit ();

  if (_position != position)
  {
    _position = position;
    position_changed.emit (_position);
  }

  // FIXME: Need to handle mimetypes

  if (_sensitive != sensitive)
  {
    _sensitive = sensitive;
    sensitive_changed.emit (_sensitive);
  }

  if (!DEE_IS_SHARED_MODEL (_sections_model) ||
      g_strcmp0 (dee_shared_model_get_swarm_name (DEE_SHARED_MODEL (_sections_model)), sections_model_name) != 0)
  {
    if (DEE_IS_SHARED_MODEL (_sections_model))
      g_object_unref (_sections_model);

    _sections_model = dee_shared_model_new (sections_model_name);
    dee_model_set_schema (_sections_model, "s", "s", NULL);

    sections_model_changed.emit ();
  }


  // FIXME: Handle place entry hints

  // FIXME: Spec says if entry_renderer == "", then ShowInLauncher () == false, but currently
  //        both places return ""
  
  if (!DEE_IS_SHARED_MODEL (_groups_model) ||
      g_strcmp0 (dee_shared_model_get_swarm_name (DEE_SHARED_MODEL (_groups_model)), entry_groups_model) != 0)
  {
    if (DEE_IS_SHARED_MODEL (_groups_model))
      g_object_unref (_groups_model);

    _groups_model = dee_shared_model_new (entry_groups_model);
    dee_model_set_schema (_groups_model, "s", "s", "s", NULL);

    _entry_renderer_changed = true;
  }

  if (!DEE_IS_SHARED_MODEL (_results_model) ||
      g_strcmp0 (dee_shared_model_get_swarm_name (DEE_SHARED_MODEL (_results_model)), entry_results_model) != 0)
  {
    if (DEE_IS_SHARED_MODEL (_results_model))
      g_object_unref (_results_model);

    _results_model = dee_shared_model_new (entry_results_model);
    dee_model_set_schema (_results_model, "s", "s", "u", "s", "s", "s", NULL);

    _entry_renderer_changed = true;
  }
  
  

  // FIXME: Handle entry renderer hints

  // FIXME: Spec says if global_renderer == "", then ShowInGlobal () == false, but currently
  //        both places return ""

  // FIXME: Handle global groups model name
  // FIXME: Handle global results model name
  // FIXME: Handle global renderer hints

  if (_vis_changed)
    visibility_changed.emit ();

  if (_entry_renderer_changed)
    entry_renderer_changed.emit ();

  if (_global_renderer_changed)
    global_renderer_changed.emit ();

  // If this was the first time we know the path, let's do the Connect dance
  if (_dbus_path == NULL)
  {
    _dbus_path = g_strdup (dbus_path);
    Connect ();
  }

  _valid = true;
}

void
PlaceEntryRemote::Connect ()
{
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                            NULL,
                            _dbus_name,
                            _dbus_path,
                            PLACE_ENTRY_IFACE,
                            NULL,
                            on_proxy_ready,
                            this);
}

void
PlaceEntryRemote::OnServiceProxyReady (GObject *source, GAsyncResult *result)
{
  GError *error = NULL;
  gchar  *name_owner = NULL;

  _proxy = g_dbus_proxy_new_for_bus_finish (result, &error);
  name_owner = g_dbus_proxy_get_name_owner (_proxy);

  if (error || !name_owner)
  {
    g_warning ("Unable to connect to PlaceEntryRemote %s: %s",
               _dbus_name,
               error ? error->message : "No name owner");
    if (error)
      g_error_free (error);

    g_free (name_owner);
    return;
  }

  g_debug ("Connected to proxy");

  g_signal_connect (_proxy, "g-signal",
                    G_CALLBACK (on_proxy_signal_received), this);

  g_free (name_owner);
}


/*
 * C -> C++ glue
 */
static void
on_proxy_ready (GObject      *source,
                GAsyncResult *result,
                gpointer      user_data)
{
  PlaceEntryRemote *self = static_cast<PlaceEntryRemote *> (user_data);

  self->OnServiceProxyReady (source, result);
}

static void
on_proxy_signal_received (GDBusProxy *proxy,
                                  gchar      *sender_name,
                                  gchar      *signal_name,
                                  GVariant   *parameters,
                                  gpointer    user_data)
{ 
  PlaceEntryRemote *self = static_cast<PlaceEntryRemote *> (user_data);  

  g_debug ("%p: %s", self, sender_name);
}
