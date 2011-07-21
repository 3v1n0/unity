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

#include "config.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlaceEntryRemote.h"
#include "NuxCore/Logger.h"

#include <glib/gi18n-lib.h>

#define PLACE_ENTRY_IFACE "com.canonical.Unity.PlaceEntry"

#define DBUS_PATH "DBusObjectPath"

namespace
{

nux::logging::Logger logger("unity.place");

}

static void on_proxy_ready(GObject*      source,
                           GAsyncResult* result,
                           gpointer      user_data);

static void on_proxy_signal_received(GDBusProxy* proxy,
                                     gchar*      sender_name,
                                     gchar*      signal_name,
                                     GVariant*   parameters,
                                     gpointer    user_data);

enum
{
  SECTION_NAME,
  SECTION_ICON
};

enum
{
  GROUP_RENDERER,
  GROUP_NAME,
  GROUP_ICON
};

enum
{
  RESULT_URI,
  RESULT_ICON,
  RESULT_GROUP_ID,
  RESULT_MIMETYPE,
  RESULT_NAME,
  RESULT_COMMENT
};

class PlaceEntrySectionRemote : public PlaceEntrySection
{
public:
  PlaceEntrySectionRemote(DeeModel* model, DeeModelIter* iter)
    : _model(model),
      _iter(iter)
  {
  }

  PlaceEntrySectionRemote(const PlaceEntrySectionRemote& b)
  {
    _model = b._model;
    _iter = b._iter;
  }

  const char* GetName() const
  {
    return dee_model_get_string(_model, _iter, SECTION_NAME);
  }

  const char* GetIcon() const
  {
    return dee_model_get_string(_model, _iter, SECTION_ICON);
  }

private:
  DeeModel*     _model;
  DeeModelIter* _iter;
};

class PlaceEntryGroupRemote : public PlaceEntryGroup
{
public:
  PlaceEntryGroupRemote(DeeModel* model, DeeModelIter* iter)
    : _model(model),
      _iter(iter)
  {
  }

  PlaceEntryGroupRemote(const PlaceEntryGroupRemote& b)
  {
    _model = b._model;
    _iter = b._iter;
  }

  const void* GetId() const
  {
    return _iter;
  }

  const char* GetRenderer() const
  {
    return dee_model_get_string(_model, _iter, GROUP_RENDERER);
  }

  const char* GetName() const
  {
    return dee_model_get_string(_model, _iter, GROUP_NAME);
  }

  const char* GetIcon() const
  {
    return dee_model_get_string(_model, _iter, GROUP_ICON);
  }

private:
  DeeModel*     _model;
  DeeModelIter* _iter;
};

class PlaceEntryResultRemote : public PlaceEntryResult
{
public:

  PlaceEntryResultRemote(DeeModel* model, DeeModelIter* iter)
    : _model(model),
      _iter(iter)
  {
  }

  PlaceEntryResultRemote(PlaceEntryResultRemote& b)
  {
    _model = b._model;
    _iter = b._iter;
  }

  const void* GetId() const
  {
    return _iter;
  };

  const char* GetName() const
  {
    return dee_model_get_string(_model, _iter, RESULT_NAME);
  }
  const char* GetIcon() const
  {
    return dee_model_get_string(_model, _iter, RESULT_ICON);
  }

  const char* GetMimeType() const
  {
    return dee_model_get_string(_model, _iter, RESULT_MIMETYPE);
  }

  const char* GetURI() const
  {
    return dee_model_get_string(_model, _iter, RESULT_URI);
  }

  const char* GetComment() const
  {
    return dee_model_get_string(_model, _iter, RESULT_COMMENT);
  }

private:
  DeeModel*     _model;
  DeeModelIter* _iter;
};


PlaceEntryRemote::PlaceEntryRemote(Place* parent, const gchar* dbus_name)
  : dirty(false),
    _parent(parent),
    _dbus_path(NULL),
    _name(NULL),
    _icon(NULL),
    _description(NULL),
    _searchhint(_("Search")),
    _shortcut(0),
    _position(0),
    _mimetypes(NULL),
    _sensitive(true),
    _active(false),
    _valid(false),
    _show_in_launcher(true),
    _show_in_global(true),
    _proxy(NULL),
    _sections_model(NULL),
    _groups_model(NULL),
    _results_model(NULL),
    _global_results_model(NULL),
    _global_groups_model(NULL),
    _previous_search(NULL),
    _previous_section(G_MAXUINT32),
    _conn_attempt(false)
{
  _dbus_name = g_strdup(dbus_name);
}

PlaceEntryRemote::~PlaceEntryRemote()
{
  g_free(_name);
  g_free(_dbus_path);
  g_free(_icon);
  g_free(_description);
  g_free(_previous_search);
  g_strfreev(_mimetypes);

  g_object_unref(_proxy);

  g_object_unref(_sections_model);
  g_object_unref(_groups_model);
  g_object_unref(_results_model);
  g_object_unref(_global_results_model);
  g_object_unref(_global_groups_model);
}

void
PlaceEntryRemote::InitFromKeyFile(GKeyFile*    key_file,
                                  const gchar* group)
{
  GError* error = NULL;
  gchar*  domain;
  gchar*  name;
  gchar*  description;
  gchar*  shortcut_entry;
  gchar*  searchhint;

  _dbus_path = g_key_file_get_string(key_file, group, DBUS_PATH, &error);
  if (_dbus_path == NULL
      || g_strcmp0(_dbus_path, "") == 0
      || _dbus_path[0] != '/'
      || error)
  {
    g_warning("Unable to load PlaceEntry '%s': Does not contain valid '"DBUS_PATH"' (%s)",
              group,
              error ? error->message : "");
    if (error)
      g_error_free(error);
    return;
  }

  domain = g_key_file_get_string(key_file, G_KEY_FILE_DESKTOP_GROUP,
                                 "X-Ubuntu-Gettext-Domain", &error);
  if (error)
  {
    // I'm messaging here because it probably should contain one
    g_message("PlaceEntry %s does not contain a translation gettext name: %s",
              group,
              error->message);
    g_error_free(error);
    error = NULL;
  }

  name = g_key_file_get_string(key_file, group, G_KEY_FILE_DESKTOP_KEY_NAME, NULL);
  _icon = g_key_file_get_string(key_file, group, G_KEY_FILE_DESKTOP_KEY_ICON, NULL);
  description = g_key_file_get_string(key_file, group, "Description", NULL);
  searchhint = g_key_file_get_string(key_file, group, "SearchHint", NULL);


  if (domain)
  {
    _name = g_strdup(dgettext(domain, name));
    _description = g_strdup(dgettext(domain, description));
    if (searchhint)
      _searchhint = g_strdup(dgettext(domain, searchhint));
  }
  else
  {
    _name = g_strdup(name);
    _description = g_strdup(description);
    if (searchhint)
      _searchhint = g_strdup(searchhint);
  }

  if (g_key_file_has_key(key_file, group, "Shortcut", NULL))
  {
    shortcut_entry = g_key_file_get_string(key_file, group, "Shortcut", NULL);
    if (strlen(shortcut_entry) == 1)
      _shortcut = shortcut_entry[0];
    else
      g_warning("Place %s has an uncompatible shortcut: %s", name, shortcut_entry);
    g_free(shortcut_entry);
  }

  /* Finally the two that should default to true */
  if (g_key_file_has_key(key_file, group, "ShowGlobal", NULL))
    _show_in_global = g_key_file_get_boolean(key_file, group, "ShowGlobal", NULL);

  if (g_key_file_has_key(key_file, group, "ShowEntry", NULL))
    _show_in_launcher = g_key_file_get_boolean(key_file, group, "ShowEntry", NULL);

  _valid = true;

  LOG_DEBUG(logger) << "PlaceEntry: " << _name;

  g_free(name);
  g_free(description);
}

/* Overrides */
Place*
PlaceEntryRemote::GetParent()
{
  return _parent;
}

const gchar*
PlaceEntryRemote::GetId()
{
  return _dbus_path;
}

const gchar*
PlaceEntryRemote::GetName()
{
  return _name;
}

const gchar*
PlaceEntryRemote::GetIcon()
{
  return _icon;
}

const gchar*
PlaceEntryRemote::GetDescription()
{
  return _description;
}

const gchar*
PlaceEntryRemote::GetSearchHint()
{
  return _searchhint;
}

guint64
PlaceEntryRemote::GetShortcut()
{
  return _shortcut;
}

guint32
PlaceEntryRemote::GetPosition()
{
  return _position;
}

const gchar**
PlaceEntryRemote::GetMimetypes()
{
  return (const gchar**)_mimetypes;
}

const std::map<gchar*, gchar*>&
PlaceEntryRemote::GetHints()
{
  return _hints;
}

bool
PlaceEntryRemote::IsSensitive()
{
  return _sensitive;
}

bool
PlaceEntryRemote::IsActive()
{
  return _active;
}

bool
PlaceEntryRemote::ShowInLauncher()
{
  return _show_in_launcher;
}

bool
PlaceEntryRemote::ShowInGlobal()
{
  return _show_in_global;
}

void
PlaceEntryRemote::SetActive(bool is_active)
{
  g_dbus_proxy_call(_proxy,
                    "SetActive",
                    g_variant_new("(b)", (gboolean)is_active),
                    G_DBUS_CALL_FLAGS_NONE,
                    -1,
                    NULL,
                    NULL,
                    NULL);
  _active = is_active;

  active_changed.emit(is_active);
}

void
PlaceEntryRemote::SetSearch(const gchar* search, std::map<gchar*, gchar*>& hints)
{
  GVariantBuilder* builder;

  if (g_strcmp0(_previous_search, search) == 0)
    return;

  g_free(_previous_search);
  _previous_search = g_strdup(search);

  // To make kamstrup's evil little mind happy
  SetActive(true);

  builder = g_variant_builder_new(G_VARIANT_TYPE("a{ss}"));

  /* FIXME: I'm ignoring hints because we don't use them currently */
  g_dbus_proxy_call(_proxy,
                    "SetSearch",
                    g_variant_new("(sa{ss})", search, builder),
                    G_DBUS_CALL_FLAGS_NONE,
                    -1,
                    NULL,
                    NULL,
                    NULL);
  g_variant_builder_unref(builder);
}

void
PlaceEntryRemote::SetActiveSection(guint32 section_id)
{
  if (_previous_section == section_id)
    return;

  _previous_section = section_id;

  g_dbus_proxy_call(_proxy,
                    "SetActiveSection",
                    g_variant_new("(u)", (guint32)section_id),
                    G_DBUS_CALL_FLAGS_NONE,
                    -1,
                    NULL,
                    NULL,
                    NULL);
}

void
PlaceEntryRemote::SetGlobalSearch(const gchar* search, std::map<gchar*, gchar*>& hints)
{
  GVariantBuilder* builder;

  // This is valid for a certain case with global search
  if (!G_IS_DBUS_PROXY(_proxy))
    return;

  builder = g_variant_builder_new(G_VARIANT_TYPE("a{ss}"));

  /* FIXME: I'm ignoring hints because we don't use them currently */
  g_dbus_proxy_call(_proxy,
                    "SetGlobalSearch",
                    g_variant_new("(sa{ss})", search, builder),
                    G_DBUS_CALL_FLAGS_NONE,
                    -1,
                    NULL,
                    NULL,
                    NULL);
  g_variant_builder_unref(builder);
}

void
PlaceEntryRemote::ForeachSection(SectionForeachCallback slot)
{
  DeeModelIter* iter, *eiter;

  iter = dee_model_get_first_iter(_sections_model);
  eiter = dee_model_get_last_iter(_sections_model);
  while (iter != eiter)
  {
    PlaceEntrySectionRemote section(_sections_model, iter);
    slot(this, section);

    iter = dee_model_next(_sections_model, iter);
  }
}

void
PlaceEntryRemote::ForeachGroup(GroupForeachCallback slot)
{
  DeeModelIter* iter, *eiter;

  iter = dee_model_get_first_iter(_groups_model);
  eiter = dee_model_get_last_iter(_groups_model);
  while (iter != eiter)
  {
    PlaceEntryGroupRemote group(_groups_model, iter);
    slot(this, group);

    iter = dee_model_next(_groups_model, iter);
  }
}

void
PlaceEntryRemote::ForeachResult(ResultForeachCallback slot)
{
  DeeModelIter* iter, *eiter;

  iter = dee_model_get_first_iter(_results_model);
  eiter = dee_model_get_last_iter(_results_model);

  while (iter != eiter)
  {
    guint         n_group;
    DeeModelIter* group_iter;

    n_group = dee_model_get_uint32(_results_model, iter, RESULT_GROUP_ID);
    group_iter = dee_model_get_iter_at_row(_groups_model, n_group);

    if (!group_iter)
    {
      g_warning("%s: Result %s does not have a valid group (%d). This is not a good thing.",
                G_STRFUNC,
                dee_model_get_string(_results_model, iter, RESULT_URI),
                n_group);

      iter = dee_model_next(_results_model, iter);
      continue;
    }

    PlaceEntryGroupRemote group(_groups_model, group_iter);
    PlaceEntryResultRemote result(_results_model, iter);

    slot(this, group, result);

    iter = dee_model_next(_results_model, iter);
  }
}


void
PlaceEntryRemote::ForeachGlobalGroup(GroupForeachCallback slot)
{
  DeeModelIter* iter, *eiter;

  iter = dee_model_get_first_iter(_global_groups_model);
  eiter = dee_model_get_last_iter(_global_groups_model);
  while (iter != eiter)
  {
    PlaceEntryGroupRemote group(_global_groups_model, iter);
    slot(this, group);

    iter = dee_model_next(_global_groups_model, iter);
  }
}

void
PlaceEntryRemote::ForeachGlobalResult(ResultForeachCallback slot)
{
  DeeModelIter* iter, *eiter;

  iter = dee_model_get_first_iter(_global_results_model);
  eiter = dee_model_get_last_iter(_global_results_model);

  while (iter != eiter)
  {
    guint         n_group;
    DeeModelIter* group_iter;

    n_group = dee_model_get_uint32(_global_results_model, iter, RESULT_GROUP_ID);
    group_iter = dee_model_get_iter_at_row(_global_groups_model, n_group);

    if (!group_iter)
    {
      g_warning("%s: Result %s does not have a valid group (%d). This is not a good thing.",
                G_STRFUNC,
                dee_model_get_string(_global_results_model, iter, RESULT_URI),
                n_group);

      iter = dee_model_next(_global_results_model, iter);
      continue;
    }

    PlaceEntryGroupRemote group(_global_groups_model, group_iter);
    PlaceEntryResultRemote result(_global_results_model, iter);

    slot(this, group, result);

    iter = dee_model_next(_global_results_model, iter);
  }
}

void
PlaceEntryRemote::GetResult(const void* id, ResultForeachCallback slot)
{
  guint         n_group;
  DeeModelIter* iter = (DeeModelIter*)id;
  DeeModelIter* group_iter;

  if (iter == NULL || dee_model_is_last(_results_model, iter))
    return;

  n_group = dee_model_get_uint32(_results_model, iter, RESULT_GROUP_ID);
  group_iter = dee_model_get_iter_at_row(_groups_model, n_group);

  if (!group_iter)
  {
    g_warning("%s: Result %s does not have a valid group (%d). This is not a good thing.",
              G_STRFUNC,
              dee_model_get_string(_results_model, iter, RESULT_URI),
              n_group);
    return;
  }

  PlaceEntryGroupRemote group(_groups_model, group_iter);
  PlaceEntryResultRemote result(_results_model, iter);

  slot(this, group, result);
}

void
PlaceEntryRemote::GetGlobalResult(const void* id, ResultForeachCallback slot)
{
  guint         n_group;
  DeeModelIter* iter = (DeeModelIter*)id;
  DeeModelIter* group_iter;

  n_group = dee_model_get_uint32(_global_results_model, iter, RESULT_GROUP_ID);
  group_iter = dee_model_get_iter_at_row(_global_groups_model, n_group);

  if (!group_iter)
  {
    g_warning("%s: Result %s does not have a valid group (%d). This is not a good thing.",
              G_STRFUNC,
              dee_model_get_string(_global_results_model, iter, RESULT_URI),
              n_group);
    return;
  }

  PlaceEntryGroupRemote group(_global_groups_model, group_iter);
  PlaceEntryResultRemote result(_global_results_model, iter);

  slot(this, group, result);
}

void
PlaceEntryRemote::ActivateResult(const void* id)
{
  DeeModelIter* iter = (DeeModelIter*)id;

  _parent->ActivateResult(dee_model_get_string(_results_model, iter, RESULT_URI),
                          dee_model_get_string(_results_model, iter, RESULT_MIMETYPE));
}

void
PlaceEntryRemote::ActivateGlobalResult(const void* id)
{
  DeeModelIter* iter = (DeeModelIter*)id;

  _parent->ActivateResult(dee_model_get_string(_global_results_model, iter, RESULT_URI),
                          dee_model_get_string(_global_results_model, iter, RESULT_MIMETYPE));
}

/* Other methods */
bool
PlaceEntryRemote::IsValid()
{
  return _valid;
}

const gchar*
PlaceEntryRemote::GetPath()
{
  return _dbus_path;
}

void
PlaceEntryRemote::Update(const gchar*  dbus_path,
                         const gchar*  name,
                         const gchar*  icon,
                         guint32       position,
                         GVariantIter* mimetypes,
                         gboolean      sensitive,
                         const gchar*  sections_model_name,
                         GVariantIter* hints,
                         const gchar*  entry_renderer,
                         const gchar*  entry_groups_model,
                         const gchar*  entry_results_model,
                         GVariantIter* entry_hints,
                         const gchar*  global_renderer,
                         const gchar*  global_groups_model,
                         const gchar*  global_results_model,
                         GVariantIter* global_hints)
{
  bool _state_changed           = false;
  bool _vis_changed             = false;
  bool _entry_renderer_changed  = false;
  bool _global_renderer_changed = false;

  if (_dbus_path)
    g_assert_cmpstr(_dbus_path, == , dbus_path);

  // Hold on tight

  if (g_strcmp0("", name) != 0 && g_strcmp0(_name, name) != 0)
  {
    g_free(_name);
    _name = g_strdup(name);
    _state_changed = true;
  }

  if (g_strcmp0("", icon) != 0 && g_strcmp0(_icon, icon) != 0)
  {
    g_free(_icon);
    _icon = g_strdup(icon);
    _state_changed = true;
  }

  if (_state_changed)
    state_changed.emit();

  if (_position != position)
  {
    _position = position;
    position_changed.emit(_position);
  }

  // FIXME: Need to handle mimetypes

  if (_sensitive != sensitive)
  {
    _sensitive = sensitive;
    sensitive_changed.emit(_sensitive);
  }

  if (!DEE_IS_SHARED_MODEL(_sections_model) ||
      g_strcmp0(dee_shared_model_get_swarm_name(DEE_SHARED_MODEL(_sections_model)), sections_model_name) != 0)
  {
    if (DEE_IS_SHARED_MODEL(_sections_model))
      g_object_unref(_sections_model);

    _sections_model = dee_shared_model_new(sections_model_name);
    dee_model_set_schema(_sections_model, "s", "s", NULL);

    sections_model_changed.emit();
  }

  // FIXME: Handle place entry hints

  // FIXME: Spec says if entry_renderer == "", then ShowInLauncher () == false, but currently
  //        both places return ""

  if (!DEE_IS_SHARED_MODEL(_groups_model) ||
      g_strcmp0(dee_shared_model_get_swarm_name(DEE_SHARED_MODEL(_groups_model)), entry_groups_model) != 0)
  {
    if (DEE_IS_SHARED_MODEL(_groups_model))
      g_object_unref(_groups_model);

    _groups_model = dee_shared_model_new(entry_groups_model);
    dee_model_set_schema(_groups_model, "s", "s", "s", NULL);

    g_signal_connect(_groups_model, "row-added",
                     (GCallback)PlaceEntryRemote::OnGroupAdded, this);

    _entry_renderer_changed = true;
  }

  if (!DEE_IS_SHARED_MODEL(_results_model) ||
      g_strcmp0(dee_shared_model_get_swarm_name(DEE_SHARED_MODEL(_results_model)), entry_results_model) != 0)
  {
    if (DEE_IS_SHARED_MODEL(_results_model))
      g_object_unref(_results_model);

    _results_model = dee_shared_model_new(entry_results_model);
    dee_model_set_schema(_results_model, "s", "s", "u", "s", "s", "s", NULL);

    g_signal_connect(_results_model, "row-added",
                     (GCallback)PlaceEntryRemote::OnResultAdded, this);
    g_signal_connect(_results_model, "row-removed",
                     (GCallback)PlaceEntryRemote::OnResultRemoved, this);

    _entry_renderer_changed = true;
  }

  // FIXME: Handle entry renderer hints

  // FIXME: Spec says if global_renderer == "", then ShowInGlobal () == false, but currently
  //        both places return ""

  if (!DEE_IS_SHARED_MODEL(_global_groups_model) ||
      g_strcmp0(dee_shared_model_get_swarm_name(DEE_SHARED_MODEL(_global_groups_model)), global_groups_model) != 0)
  {
    if (DEE_IS_SHARED_MODEL(_global_groups_model))
      g_object_unref(_global_groups_model);

    _global_groups_model = dee_shared_model_new(global_groups_model);
    dee_model_set_schema(_global_groups_model, "s", "s", "s", NULL);

    g_signal_connect(_global_groups_model, "row-added",
                     (GCallback)PlaceEntryRemote::OnGlobalGroupAdded, this);

    _global_renderer_changed = true;
  }

  if (!DEE_IS_SHARED_MODEL(_global_results_model) ||
      g_strcmp0(dee_shared_model_get_swarm_name(DEE_SHARED_MODEL(_global_results_model)), global_results_model) != 0)
  {
    if (DEE_IS_SHARED_MODEL(_global_results_model))
      g_object_unref(_global_results_model);

    _global_results_model = dee_shared_model_new(global_results_model);
    dee_model_set_schema(_global_results_model, "s", "s", "u", "s", "s", "s", NULL);

    g_signal_connect(_global_results_model, "row-added",
                     (GCallback)PlaceEntryRemote::OnGlobalResultAdded, this);
    g_signal_connect(_global_results_model, "row-removed",
                     (GCallback)PlaceEntryRemote::OnGlobalResultRemoved, this);

    _global_renderer_changed = true;
  }
  // FIXME: Handle global renderer hints

  if (_vis_changed)
    visibility_changed.emit();

  if (_entry_renderer_changed)
    entry_renderer_changed.emit();

  if (_global_renderer_changed)
    global_renderer_changed.emit(this);

  // If this was the first time we know the path, let's do the Connect dance
  if (_dbus_path == NULL)
  {
    _dbus_path = g_strdup(dbus_path);
    Connect();
  }

  // place might be already active until is successfully connected and updated
  // therefore activation procedure needs to be redone
  if (_active)
  {
    ubus_server_send_message(ubus_server_get_default(),
                             UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                             g_variant_new("(sus)",
                                           GetId(),
                                           _previous_section,
                                           _previous_search));
  }

  _valid = true;
}

void
PlaceEntryRemote::Connect()
{
  if (!_conn_attempt && !G_IS_DBUS_PROXY(_proxy))
  {
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
                             G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                             NULL,
                             _dbus_name,
                             _dbus_path,
                             PLACE_ENTRY_IFACE,
                             NULL,
                             on_proxy_ready,
                             this);
    _conn_attempt = true;
  }
}

void
PlaceEntryRemote::OnServiceProxyReady(GObject* source, GAsyncResult* result)
{
  GError* error = NULL;
  gchar*  name_owner = NULL;

  _proxy = g_dbus_proxy_new_for_bus_finish(result, &error);
  name_owner = g_dbus_proxy_get_name_owner(_proxy);
  _conn_attempt = false;

  if (error || !name_owner)
  {
    g_warning("Unable to connect to PlaceEntryRemote %s: %s",
              _dbus_name,
              error ? error->message : "No name owner");
    if (error)
      g_error_free(error);

    g_free(name_owner);
    return;
  }

  LOG_DEBUG(logger) << "Connected to proxy";

  g_signal_connect(_proxy, "g-signal",
                   G_CALLBACK(on_proxy_signal_received), this);
  g_signal_connect(_proxy, "notify::g-name-owner",
                   G_CALLBACK(PlaceEntryRemote::OnProxyNameOwnerChanged), this);

  g_free(name_owner);
}

void
PlaceEntryRemote::OnProxyNameOwnerChanged(GDBusProxy*       proxy,
                                          GParamSpec*       pspec,
                                          PlaceEntryRemote* self)
{
  gchar* name_owner  = g_dbus_proxy_get_name_owner(proxy);

  if (!name_owner)
  {
    // Remote proxy has died
    LOG_DEBUG(logger) << "Remote PlaceEntryRemote proxy "
                      << self->_dbus_path
                      << " no longer exists, reconnecting";
    g_object_unref(self->_proxy);
    self->_proxy = NULL;

    self->Connect();
  }
  else
    g_free(name_owner);
}

void
PlaceEntryRemote::OnGroupAdded(DeeModel* model, DeeModelIter* iter, PlaceEntryRemote* self)
{
  PlaceEntryGroupRemote group(model, iter);

  self->group_added.emit(self, group);
}

void
PlaceEntryRemote::OnResultAdded(DeeModel* model, DeeModelIter* iter, PlaceEntryRemote* self)
{
  guint         n_group;
  DeeModelIter* group_iter;

  n_group = dee_model_get_uint32(model, iter, RESULT_GROUP_ID);
  group_iter = dee_model_get_iter_at_row(self->_groups_model, n_group);

  if (!group_iter)
  {
    g_warning("%s: Result %s does not have a valid group (%d). This is not a good thing.",
              G_STRFUNC,
              dee_model_get_string(model, iter, RESULT_URI),
              n_group);
    return;
  }

  PlaceEntryGroupRemote group(self->_groups_model, group_iter);
  PlaceEntryResultRemote result(model, iter);

  self->result_added.emit(self, group, result);
}

void
PlaceEntryRemote::OnResultRemoved(DeeModel* model, DeeModelIter* iter, PlaceEntryRemote* self)
{
  guint         n_group;
  DeeModelIter* group_iter;

  n_group = dee_model_get_uint32(model, iter, RESULT_GROUP_ID);
  group_iter = dee_model_get_iter_at_row(self->_groups_model, n_group);

  if (!group_iter)
  {
    g_warning("%s: Result %s does not have a valid group (%d). This is not a good thing.",
              G_STRFUNC,
              dee_model_get_string(model, iter, RESULT_URI),
              n_group);
    return;
  }

  PlaceEntryGroupRemote group(self->_groups_model, group_iter);
  PlaceEntryResultRemote result(model, iter);

  self->result_removed.emit(self, group, result);
}


void
PlaceEntryRemote::OnGlobalGroupAdded(DeeModel* model, DeeModelIter* iter, PlaceEntryRemote* self)
{
  PlaceEntryGroupRemote group(model, iter);

  self->global_group_added.emit(self, group);
}

void
PlaceEntryRemote::OnGlobalResultAdded(DeeModel* model, DeeModelIter* iter, PlaceEntryRemote* self)
{
  guint         n_group;
  DeeModelIter* group_iter;

  n_group = dee_model_get_uint32(model, iter, RESULT_GROUP_ID);
  group_iter = dee_model_get_iter_at_row(self->_global_groups_model, n_group);

  if (!group_iter)
  {
    g_warning("%s: Result %s does not have a valid group (%d). This is not a good thing.",
              G_STRFUNC,
              dee_model_get_string(model, iter, RESULT_URI),
              n_group);
    return;
  }

  PlaceEntryGroupRemote group(self->_global_groups_model, group_iter);
  PlaceEntryResultRemote result(model, iter);

  self->global_result_added.emit(self, group, result);
}

void
PlaceEntryRemote::OnGlobalResultRemoved(DeeModel* model, DeeModelIter* iter, PlaceEntryRemote* self)
{
  guint         n_group;
  DeeModelIter* group_iter;

  n_group = dee_model_get_uint32(model, iter, RESULT_GROUP_ID);
  group_iter = dee_model_get_iter_at_row(self->_global_groups_model, n_group);

  if (!group_iter)
  {
    g_warning("%s: Result %s does not have a valid group (%d). This is not a good thing.",
              G_STRFUNC,
              dee_model_get_string(model, iter, RESULT_URI),
              n_group);
    return;
  }

  PlaceEntryGroupRemote group(self->_global_groups_model, group_iter);
  PlaceEntryResultRemote result(model, iter);

  self->global_result_removed.emit(self, group, result);
}

/*
 * C -> C++ glue
 */
static void
on_proxy_ready(GObject*      source,
               GAsyncResult* result,
               gpointer      user_data)
{
  PlaceEntryRemote* self = static_cast<PlaceEntryRemote*>(user_data);

  self->OnServiceProxyReady(source, result);
}

static void
on_proxy_signal_received(GDBusProxy* proxy,
                         gchar*      sender_name,
                         gchar*      signal_name,
                         GVariant*   parameters,
                         gpointer    user_data)
{
  PlaceEntryRemote* self = static_cast<PlaceEntryRemote*>(user_data);

  if (g_strcmp0(signal_name, "SearchFinished") == 0)
  {
    guint32       section = 0;
    gchar*        search_string = NULL;
    GVariantIter* iter;
    std::map<const char*, const char*> hints;

    g_variant_get(parameters, "(usa{ss})", &section, &search_string, &iter);
    self->search_finished.emit(search_string, section, hints);

    g_free(search_string);
    g_variant_iter_free(iter);
  }
}
