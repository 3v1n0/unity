// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 * Authored by: Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include "LauncherEntryRemote.h"
#include <UnityCore/Variant.h>
#include <NuxCore/Logger.h>

namespace unity
{
DECLARE_LOGGER(logger, "unity.launcher.entry.remote");

/**
 * Create a new LauncherEntryRemote parsed from the raw DBus wire format
 * of the com.canonical.Unity.LauncherEntry.Update signal '(sa{sv})'. The
 * dbus_name argument should be the unique bus name of the process owning
 * the launcher entry.
 *
 * You don't normally need to use this constructor yourself. The
 * LauncherEntryRemoteModel will do that for you when needed.
 */
LauncherEntryRemote::LauncherEntryRemote(std::string const& dbus_name, GVariant* val)
  : _dbus_name(dbus_name)
  , _count(0)
  , _progress(0.0f)
  , _emblem_visible(false)
  , _count_visible(false)
  , _progress_visible(false)
  , _urgent(false)
{
  glib::String app_uri;
  GVariantIter* prop_iter;

  if (!val || dbus_name.empty())
  {
    LOG_ERROR(logger) << "Invalid launcher entry remote construction";
    return;
  }

  /* This will make sure that the values are properly ref_sink'ed and unreff'ed */
  glib::Variant values(val);
  g_variant_get(values, "(sa{sv})", &app_uri, &prop_iter);

  _app_uri = app_uri.Str();

  Update(prop_iter);
  g_variant_iter_free(prop_iter);
}

/**
 * The appuri property is on the form application://$desktop_file_id and is
 * used within the LauncherEntryRemoteModel to uniquely identify the various
 * applications.
 *
 * Note that a desktop file id is defined to be the base name of the desktop
 * file including the extension. Eg. gedit.desktop. Thus a full appuri could be
 * application://gedit.desktop.
 */
std::string const& LauncherEntryRemote::AppUri() const
{
  return _app_uri;
}

/**
 * Return the unique DBus name for the remote party owning this launcher entry.
 */
std::string const& LauncherEntryRemote::DBusName() const
{
  return _dbus_name;
}

std::string const& LauncherEntryRemote::Emblem() const
{
  return _emblem;
}

int64_t LauncherEntryRemote::Count() const
{
  return _count;
}

double LauncherEntryRemote::Progress() const
{
  return _progress;
}

/**
 * Return a pointer to the current quicklist of the launcher entry; NULL if
 * it's unset.
 *
 * The returned object should not be freed.
 */
glib::Object<DbusmenuClient> const& LauncherEntryRemote::Quicklist() const
{
  return _quicklist;
}

bool LauncherEntryRemote::EmblemVisible() const
{
  return _emblem_visible;
}

bool LauncherEntryRemote::CountVisible() const
{
  return _count_visible;
}

bool LauncherEntryRemote::ProgressVisible() const
{
  return _progress_visible;
}

bool LauncherEntryRemote::Urgent() const
{
  return _urgent;
}

/**
 * Set the unique DBus name for the process owning the launcher entry.
 *
 * If the entry has any exported quicklist it will be removed.
 */
void LauncherEntryRemote::SetDBusName(std::string const& dbus_name)
{
  if (_dbus_name == dbus_name)
    return;

  std::string old_name(_dbus_name);
  _dbus_name = dbus_name;

  /* Remove the quicklist since we can't know if it it'll be valid on the
   * new name, and we certainly don't want the quicklist to operate on a
   * different name than the rest of the launcher API */
  SetQuicklist(nullptr);

  dbus_name_changed.emit(this, old_name);
}

void LauncherEntryRemote::SetEmblem(std::string const& emblem)
{
  if (_emblem == emblem)
    return;

  _emblem = emblem;
  emblem_changed.emit(this);
}

void LauncherEntryRemote::SetCount(int64_t count)
{
  if (_count == count)
    return;

  _count = count;
  count_changed.emit(this);
}

void LauncherEntryRemote::SetProgress(double progress)
{
  if (_progress == progress)
    return;

  _progress = progress;
  progress_changed.emit(this);
}

/**
 * Set the quicklist of this entry to be the Dbusmenu on the given path.
 * If entry already has a quicklist with the same path this call is a no-op.
 *
 * To unset the quicklist pass in a NULL path or empty string.
 */
void LauncherEntryRemote::SetQuicklistPath(std::string const& dbus_path)
{
  /* Check if existing quicklist have exact same path
   * and ignore the change in that case */
  if (_quicklist)
  {
    glib::String ql_path;
    g_object_get(_quicklist, DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &ql_path, NULL);

    if (ql_path.Str() == dbus_path)
    {
      return;
    }
  }
  else if (!_quicklist && dbus_path.empty())
    return;

  if (!dbus_path.empty())
    _quicklist = dbusmenu_client_new(_dbus_name.c_str(), dbus_path.c_str());
  else
    _quicklist = nullptr;

  quicklist_changed.emit(this);
}

/**
 * Set the quicklist of this entry to a given DbusmenuClient.
 * If entry already has a quicklist with the same path this call is a no-op.
 *
 * To unset the quicklist for this entry pass in NULL as the quicklist to set.
 */
void
LauncherEntryRemote::SetQuicklist(DbusmenuClient* quicklist)
{
  /* Check if existing quicklist have exact same path as the new one
   * and ignore the change in that case. We also assert that the quicklist
   * uses the same name as the connection owning this launcher entry */
  if (_quicklist)
  {
    glib::String ql_path, new_ql_path, new_ql_name;

    g_object_get(_quicklist, DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &ql_path, NULL);

    if (quicklist)
    {
      g_object_get(quicklist, DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &new_ql_path, NULL);
      g_object_get(quicklist, DBUSMENU_CLIENT_PROP_DBUS_NAME, &new_ql_name, NULL);
    }

    if (quicklist && new_ql_name.Str() != _dbus_name)
    {
      LOG_ERROR(logger) << "Mismatch between quicklist- and launcher entry owner:"
                        << new_ql_name << " and " << _dbus_name << " respectively";
      return;
    }

    if (new_ql_path.Str() == ql_path.Str())
    {
      return;
    }
  }
  else if (!_quicklist && !quicklist)
    return;

  if (!quicklist)
    _quicklist = nullptr;
  else
    _quicklist = glib::Object<DbusmenuClient>(quicklist, glib::AddRef());

  quicklist_changed.emit(this);
}

void LauncherEntryRemote::SetEmblemVisible(bool visible)
{
  if (_emblem_visible == visible)
    return;

  _emblem_visible = visible;
  emblem_visible_changed.emit(this);
}

void LauncherEntryRemote::SetCountVisible(bool visible)
{
  if (_count_visible == visible)
    return;

  _count_visible = visible;
  count_visible_changed.emit(this);
}

void LauncherEntryRemote::SetProgressVisible(bool visible)
{
  if (_progress_visible == visible)
    return;

  _progress_visible = visible;
  progress_visible_changed.emit(this);
}

void LauncherEntryRemote::SetUrgent(bool urgent)
{
  if (_urgent == urgent)
    return;

  _urgent = urgent;
  urgent_changed.emit(this);
}

/**
 * Set all properties from 'other' on 'this'.
 */
void LauncherEntryRemote::Update(LauncherEntryRemote::Ptr const& other)
{
  /* It's important that we update the DBus name first since it might
   * unset the quicklist if it changes */

  if (!other)
    return;

  SetDBusName(other->DBusName());

  SetEmblem(other->Emblem());
  SetCount(other->Count());
  SetProgress(other->Progress());
  SetQuicklist(other->Quicklist());
  SetUrgent(other->Urgent());

  SetEmblemVisible(other->EmblemVisible());
  SetCountVisible(other->CountVisible());
  SetProgressVisible(other->ProgressVisible());
}

/**
 * Iterate over a GVariantIter containing elements of type '{sv}' and apply
 * any properties to 'this'.
 */
void LauncherEntryRemote::Update(GVariantIter* prop_iter)
{
  gchar* prop_key;
  GVariant* prop_value;

  g_return_if_fail(prop_iter != NULL);

  while (g_variant_iter_loop(prop_iter, "{sv}", &prop_key, &prop_value))
  {
    if (g_str_equal("emblem", prop_key))
      SetEmblem(glib::String(g_variant_dup_string(prop_value, 0)).Str());
    else if (g_str_equal("count", prop_key))
      SetCount(g_variant_get_int64(prop_value));
    else if (g_str_equal("progress", prop_key))
      SetProgress(g_variant_get_double(prop_value));
    else if (g_str_equal("emblem-visible", prop_key))
      SetEmblemVisible(g_variant_get_boolean(prop_value));
    else if (g_str_equal("count-visible", prop_key))
      SetCountVisible(g_variant_get_boolean(prop_value));
    else if (g_str_equal("progress-visible", prop_key))
      SetProgressVisible(g_variant_get_boolean(prop_value));
    else if (g_str_equal("urgent", prop_key))
      SetUrgent(g_variant_get_boolean(prop_value));
    else if (g_str_equal("quicklist", prop_key))
    {
      /* The value is the object path of the dbusmenu */
      SetQuicklistPath(glib::String(g_variant_dup_string(prop_value, 0)).Str());
    }
  }
}

std::string LauncherEntryRemote::GetName() const
{
  // This seems a more appropriate name than LauncherEntryRemote
  return "LauncherEntry";
}

void LauncherEntryRemote::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add("count", Count())
    .add("progress", Progress())
    .add("emblem_visible", EmblemVisible())
    .add("count_visible", CountVisible())
    .add("progress_visible", ProgressVisible())
    .add("urgent", Urgent());
}

} // Namespace
