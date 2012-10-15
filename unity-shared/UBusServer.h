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
 *              Michal Hruby <michal.hruby@canonical.com>
 */

#ifndef UNITY_UBUS_SERVER_H
#define UNITY_UBUS_SERVER_H

#include <memory>
#include <string>
#include <map>

#include <UnityCore/Variant.h>
#include <UnityCore/GLibSource.h>

namespace unity
{

typedef std::function<void(glib::Variant const&)> UBusCallback;

class UBusServer : public boost::noncopyable
{
public:
  UBusServer();

  unsigned RegisterInterest(std::string const& interest_name,
                            UBusCallback const& slot);
  void UnregisterInterest(unsigned connection_id);
  void SendMessage(std::string const& message_name,
                   glib::Variant const& args = glib::Variant());
  void SendMessageFull(std::string const& message_name,
                       glib::Variant const& args,
                       glib::Source::Priority prio);

private:
  struct UBusConnection
  {
    typedef std::shared_ptr<UBusConnection> Ptr;

    UBusCallback slot;
    unsigned id;

    UBusConnection(UBusCallback const& cb, unsigned connection_id)
      : slot(cb), id(connection_id) {}
  };

  bool DispatchMessages(glib::Source::Priority);

  unsigned last_id_;
  std::multimap<std::string, UBusConnection::Ptr> interests_;
  std::multimap<int, std::pair<std::string, glib::Variant> > msg_queue_;
  glib::SourceManager source_manager_;
};

}

#endif
