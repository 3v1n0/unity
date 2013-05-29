/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef DBUSTESTRUNNER_H
#define DBUSTESTRUNNER_H

 #include <UnityCore/GLibDBusProxy.h>

#include <UnityCore/Preview.h>
#include <UnityCore/Results.h>
#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{
namespace previews
{
DECLARE_LOGGER(logger, "unity.dash.dbus.testrunner");

class DBusTestRunner
{
public:
  typedef std::map<std::string, unity::glib::Variant> Hints;

  DBusTestRunner(std::string const& dbus_name, std::string const& dbus_path, std::string const& interface_name)
  : proxy_(nullptr)
  , connected_(false)
  , dbus_name_(dbus_name)
  , dbus_path_(dbus_path)
  {
    proxy_ = new glib::DBusProxy(dbus_name, dbus_path, interface_name);
    proxy_->connected.connect(sigc::mem_fun(this, &DBusTestRunner::OnProxyConnectionChanged));
    proxy_->disconnected.connect(sigc::mem_fun(this, &DBusTestRunner::OnProxyDisconnected));
  }

  virtual void OnProxyConnectionChanged()
  {
    LOG_DEBUG(logger) << "Dbus connection changed. connected=" << (proxy_->IsConnected() ? "true" : "false");
  }

  virtual void OnProxyDisconnected()
  {
    LOG_DEBUG(logger) << "Dbus disconnected";
  }

  sigc::signal<void, bool> connected;

  glib::DBusProxy* proxy_;

  bool connected_;
  std::string dbus_name_;
  std::string dbus_path_;
};


} // namespace previews
} // namespace dash
} // namespace unity

#endif // DBUSTESTRUNNER_H
