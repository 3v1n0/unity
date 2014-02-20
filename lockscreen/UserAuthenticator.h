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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITY_USER_AUTHENTICATOR_H
#define UNITY_USER_AUTHENTICATOR_H

#include <future>
#include <functional>
#include <memory>
#include <sigc++/signal.h>
#include <string>

namespace unity
{
namespace lockscreen
{

class UserAuthenticator
{
public:
  typedef std::function<void(bool)> AuthenticateEndCallback;

  virtual ~UserAuthenticator() {};

  // Authenticate the user in a background thread.
  virtual bool AuthenticateStart(std::string const& username,
                                 AuthenticateEndCallback authenticate_cb) = 0;

  // FIXME (andy) found a better name for the promise ptr.
  sigc::signal<void, std::string, std::shared_ptr<std::promise<std::string>> const&> echo_on_requested;
  sigc::signal<void, std::string, std::shared_ptr<std::promise<std::string>> const&> echo_off_requested;
  sigc::signal<void, std::string> message_requested;
  sigc::signal<void, std::string> error_requested;
  sigc::signal<void> clear_prompts;
};

} // lockscreen
} // unity

 #endif // UNITY_USER_AUTHENTICATOR_H
