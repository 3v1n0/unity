// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013-2015 Canonical Ltd
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
 * Authored by: William Hua <william.hua@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef __GNOME_KEY_GRABBER_IMPL_H__
#define __GNOME_KEY_GRABBER_IMPL_H__

#include "GnomeKeyGrabber.h"

#include <unordered_map>
#include <unordered_set>
#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/GLibDBusNameWatcher.h>
#include <UnityCore/GLibSignal.h>

namespace unity
{
namespace key
{

struct GnomeGrabber::Impl
{
  Impl(bool test_mode = false);
  ~Impl();

  uint32_t NextActionID();

  bool AddAction(CompAction const& action, uint32_t& action_id);
  uint32_t AddAction(CompAction const& action);

  bool RemoveAction(CompAction const& action);
  bool RemoveActionByID(uint32_t action_id);
  bool RemoveActionByIndex(size_t index);

  GVariant* OnShellMethodCall(std::string const& method, GVariant* parameters, std::string const& sender, std::string const&);
  uint32_t GrabDBusAccelerator(std::string const& owner, std::string const& accelerator, uint32_t flags);
  bool UnGrabDBusAccelerator(std::string const& sender, uint32_t action_id);
  void ActivateDBusAction(CompAction const& action, uint32_t id, uint32_t device, uint32_t timestamp) const;

  bool IsActionPostponed(CompAction const& action) const;

  void UpdateWhitelist();

  CompScreen* screen_;

  glib::DBusServer shell_server_;
  glib::DBusObject::Ptr shell_object_;

  glib::Object<GSettings> settings_;
  glib::Signal<void, GSettings*, gchar*> whitelist_changed_signal_;
  std::list<std::string> whitelist_;

  uint32_t current_action_id_;
  std::vector<uint32_t> actions_ids_;
  std::vector<uint32_t> actions_customers_;
  CompAction::Vector actions_;

  struct OwnerActions { glib::DBusNameWatcher::Ptr watcher; std::unordered_set<uint32_t> actions; };
  std::unordered_map<std::string, OwnerActions> actions_by_owner_;
};

} // namespace key
} // namespace unity

#endif // __GNOME_KEY_GRABBER_IMPL_H__
