/*
 * Copyright (C) 2016 Canonical Ltd
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

#ifndef UNITY_LOCKSCREEN_LOGIN_MANAGER
#define UNITY_LOCKSCREEN_LOGIN_MANAGER

#include <memory>
#include <functional>
#include <sigc++/signal.h>

namespace unity
{
namespace lockscreen
{

typedef std::function<void(gint)> InhibitCallback;

class LoginManager
{
public:
  typedef std::shared_ptr<LoginManager> Ptr;

  LoginManager();
  ~LoginManager();

  void Inhibit(std::string const&, InhibitCallback const&);
  void Uninhibit(gint);

  nux::ROProperty<bool> session_active;

  sigc::signal<void> connected;
  sigc::signal<void, bool> prepare_for_sleep;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}
}

#endif