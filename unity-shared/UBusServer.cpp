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

#include "UBusServer.h"

namespace unity
{
DECLARE_LOGGER(logger, "unity.ubus");

UBusServer::UBusServer()
  : last_id_(0)
{}

unsigned UBusServer::RegisterInterest(std::string const& interest_name,
                                      UBusCallback const& slot)
{
  if (!slot || interest_name.empty())
    return 0;

  unsigned connection_id = ++last_id_;
  auto connection = std::make_shared<UBusConnection>(slot, connection_id);
  interests_.insert(std::pair<std::string, UBusConnection::Ptr>(interest_name, connection));

  return connection_id;
}

void UBusServer::UnregisterInterest(unsigned connection_id)
{
  auto it = std::find_if(interests_.begin(), interests_.end(),
                         [&] (std::pair<std::string, UBusConnection::Ptr> const& p)
                         { return p.second->id == connection_id; });
  if (it != interests_.end()) interests_.erase(it);
}

void UBusServer::SendMessage(std::string const& message_name,
                              glib::Variant const& args)
{
  SendMessageFull(message_name, args, glib::Source::Priority::DEFAULT_IDLE);
}

void UBusServer::SendMessageFull(std::string const& message_name,
                                  glib::Variant const& args,
                                  glib::Source::Priority prio)
{
  // queue the message
  msg_queue_.insert(std::pair<int, std::pair<std::string, glib::Variant>>(prio, std::make_pair(message_name, args)));

  // start the source (if not already running)
  auto src_nick = std::to_string(static_cast<int>(prio));
  auto src_ptr = source_manager_.GetSource(src_nick);
  if (!src_ptr)
  {
    source_manager_.Add(new glib::Idle([this, prio] ()
    {
      return DispatchMessages(prio);
    }, prio));
  }
}

bool UBusServer::DispatchMessages(glib::Source::Priority prio)
{
  // copy messages we are about to dispatch to a separate container
  std::vector<std::pair<std::string, glib::Variant> > dispatched_msgs;

  auto iterators = msg_queue_.equal_range(prio);
  for (auto it = iterators.first; it != iterators.second; ++it)
  {
    dispatched_msgs.push_back((*it).second);
  }

  // remove the messages from the queue
  msg_queue_.erase(prio);

  for (unsigned i = 0; i < dispatched_msgs.size(); i++)
  {
    // invoke callbacks for this message_name
    std::string const& message_name = dispatched_msgs[i].first;
    glib::Variant const& msg_args = dispatched_msgs[i].second;
    auto interest_it = interests_.find(message_name);
    while (interest_it != interests_.end())
    {
      // add a reference to make sure we don't crash if the slot unregisters itself
      UBusConnection::Ptr connection((*interest_it).second);
      interest_it++;
      // invoke the slot
      // FIXME: what if this slot unregisters the next? We should mark the interests map dirty in UnregisterInterest
      connection->slot(msg_args);

      if (interest_it == interests_.end() || 
          (*interest_it).first != message_name)
        break;
    }
  }

  // return true if there are new queued messages with this prio
  return msg_queue_.find(prio) != msg_queue_.end();
}

}
