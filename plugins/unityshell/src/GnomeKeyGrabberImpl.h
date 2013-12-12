// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
*/

#ifndef __GNOME_KEY_GRABBER_IMPL_H__
#define __GNOME_KEY_GRABBER_IMPL_H__

#include "GnomeKeyGrabber.h"

#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/GLibDBusServer.h>

namespace unity
{
namespace grabber
{

struct GnomeKeyGrabber::Impl
{
  Impl(GnomeKeyGrabber* parent, CompScreen* screen, bool test_mode = false);
  ~Impl();

  unsigned int addAction(const CompAction& action);
  bool removeAction(const CompAction& action);
  bool removeAction(unsigned int action_id);

  GVariant* onShellMethodCall(std::string const& method, GVariant* parameters);
  unsigned int grabAccelerator(const char *accelerator, unsigned int flags);
  void activateAction(const CompAction* action, unsigned int device);

  bool actionInitiated(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool actionTerminated(CompAction* action, CompAction::State state, CompOption::Vector& options);

  GnomeKeyGrabber* grabber_;
  glib::DBusServer shell_server_;
  glib::DBusObject::Ptr shell_object_;

  CompScreen* screen_;
  CompAction::Vector actions_;
  unsigned int current_action_id_;
  std::map<unsigned int, CompAction*> action_for_id_;

  bool test_mode_;
};

} // namespace grabber
} // namespace unity

#endif // __GNOME_KEY_GRABBER_IMPL_H__
