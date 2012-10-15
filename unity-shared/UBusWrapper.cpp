/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Michal Hruby <michal.hruby@canonical.com>
 */

#include <NuxCore/Logger.h>

#include "UBusWrapper.h"

namespace unity
{

// initialize the global static ptr
std::unique_ptr<UBusServer> UBusManager::server(new UBusServer());

UBusManager::UBusManager()
{
}

UBusManager::~UBusManager()
{
  // we don't want to modify a container while iterating it
  std::set<unsigned> ids_copy(connection_ids_);
  for (auto it = ids_copy.begin(); it != ids_copy.end(); ++it)
  {
    UnregisterInterest(*it);
  }
}

unsigned UBusManager::RegisterInterest(std::string const& interest_name,
                                       UBusCallback const& slot)
{
  unsigned c_id = server->RegisterInterest(interest_name, slot);

  if (c_id != 0) connection_ids_.insert(c_id);

  return c_id;
}

void UBusManager::UnregisterInterest(unsigned connection_id)
{
  auto it = connection_ids_.find(connection_id);
  if (it != connection_ids_.end())
  {
    server->UnregisterInterest(connection_id);
    connection_ids_.erase(it);
  }
}

void UBusManager::SendMessage(std::string const& message_name,
                              glib::Variant const& args,
                              glib::Source::Priority prio)
{
  server->SendMessageFull(message_name, args, prio);
}

}
