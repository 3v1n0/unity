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

#ifndef UNITY_UBUS_WRAPPER_H
#define UNITY_UBUS_WRAPPER_H

#include <memory>
#include <string>
#include <vector>
#include <boost/utility.hpp>

#include "ubus-server.h"

namespace unity
{

class UBusManager : public boost::noncopyable
{
public:
  typedef std::function<void(GVariant*)> UBusManagerCallback;

  UBusManager();
  ~UBusManager();

  void RegisterInterest(std::string const& interest_name, UBusManagerCallback slot);
  void UnregisterInterest(std::string const& interest_name);
  void SendMessage(std::string const& message_name, GVariant* args = NULL);

private:
  struct UBusConnection
  {
    typedef std::shared_ptr<UBusConnection> Ptr;

    std::string name;
    UBusManagerCallback slot;
    guint id;
  };

  static void OnCallback(GVariant* args, gpointer user_data);

  UBusServer* server_;
  std::vector<UBusConnection::Ptr> connections_;
};

}

#endif
