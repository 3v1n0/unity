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

#include "UBusWrapper.h"

namespace unity
{

UBusManager::UBusManager()
  : server_(ubus_server_get_default())
{}

UBusManager::~UBusManager()
{
  for (auto connection: connections_)
  {
    ubus_server_unregister_interest(server_, connection->id);
  }
}

void UBusManager::RegisterInterest(std::string const& interest_name,
                                   UBusManagerCallback slot)
{
  UBusConnection::Ptr connection (new UBusConnection());
  connection->manager = this;
  connection->name = interest_name;
  connection->slot = slot;
  connection->id = ubus_server_register_interest(server_,
                                                 interest_name.c_str(),
                                                 UBusManager::OnCallback,
                                                 connection.get());
  connections_.push_back(connection);
}

void UBusManager::UnregisterInterest(std::string const& interest_name)
{
  for (auto it = connections_.begin(); it != connections_.end(); ++it)
  {
    if ((*it)->name == interest_name)
    {
      ubus_server_unregister_interest(server_, (*it)->id);
      connections_.erase(it);
      break;
    }
  }
}

void UBusManager::OnCallback(GVariant* args, gpointer user_data)
{
  UBusConnection* connection = static_cast<UBusConnection*>(user_data);

  connection->slot(args);
}

void UBusManager::SendMessage(std::string const& message_name, GVariant* args)
{
  ubus_server_send_message(server_, message_name.c_str(), args);
}

}
