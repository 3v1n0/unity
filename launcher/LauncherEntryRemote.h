// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 */

#ifndef LAUNCHER_ENTRY_REMOTE_H
#define LAUNCHER_ENTRY_REMOTE_H

#include <glib.h>
#include <memory>
#include <sigc++/sigc++.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/menuitem.h>
#include <UnityCore/GLibWrapper.h>

 #include "unity-shared/Introspectable.h"

namespace unity
{

/**
 * Instances of this class mirrors the remote metadata for a laucnher entry
 * exposed by an application via the com.canonical.Unity.LauncherEntry DBus API.
 *
 * You do not create instances of LauncherEntryRemote yourself. Instead they
 * are created and managed dynamically by a LauncherEntryRemoteModel.
 */
class LauncherEntryRemote : public sigc::trackable, public unity::debug::Introspectable
{
public:
  typedef std::shared_ptr<LauncherEntryRemote> Ptr;

  LauncherEntryRemote(std::string const& dbus_name, GVariant* val);

  std::string const& AppUri() const;
  std::string const& DBusName() const;
  std::string const& Emblem() const;
  int32_t Count() const;
  double Progress() const;
  glib::Object<DbusmenuClient> const& Quicklist() const;

  bool EmblemVisible() const;
  bool CountVisible() const;
  bool ProgressVisible() const;
  bool Urgent() const;

  /// Update this instance using details from another:
  void Update(LauncherEntryRemote::Ptr const& other);
  /// Update this instance from a GVariant property iterator.
  void Update(GVariantIter* prop_iter);
  /// Set a new DBus name. This destroys the current quicklist.
  void SetDBusName(std::string const& dbus_name);

  // from Introspectable:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  sigc::signal<void, LauncherEntryRemote*, std::string> dbus_name_changed;   // gives the old name as arg
  sigc::signal<void, LauncherEntryRemote*> emblem_changed;
  sigc::signal<void, LauncherEntryRemote*> count_changed;
  sigc::signal<void, LauncherEntryRemote*> progress_changed;
  sigc::signal<void, LauncherEntryRemote*> quicklist_changed;

  sigc::signal<void, LauncherEntryRemote*> emblem_visible_changed;
  sigc::signal<void, LauncherEntryRemote*> count_visible_changed;
  sigc::signal<void, LauncherEntryRemote*> progress_visible_changed;
  sigc::signal<void, LauncherEntryRemote*> urgent_changed;

private:
  std::string _dbus_name;

  std::string _app_uri;
  std::string _emblem;
  int32_t _count;
  double _progress;

  std::string _quicklist_dbus_path;
  glib::Object<DbusmenuClient> _quicklist;

  bool _emblem_visible;
  bool _count_visible;
  bool _progress_visible;
  bool _urgent;

  void SetEmblem(std::string const& emblem);
  void SetCount(int32_t count);
  void SetProgress(double progress);
  void SetQuicklistPath(std::string const& dbus_path);
  void SetQuicklist(DbusmenuClient* quicklist);
  void SetEmblemVisible(bool visible);
  void SetCountVisible(bool visible);
  void SetProgressVisible(bool visible);
  void SetUrgent(bool urgent);
};

} // namespace
#endif // LAUNCHER_ENTRY_REMOTE_H
