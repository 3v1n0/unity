/*
 * Copyright (C) 2010-2012 Canonical Ltd
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

#ifndef UNITY_UBUS_WRAPPER_H
#define UNITY_UBUS_WRAPPER_H

#include <memory>
#include <string>
#include <set>

#include <UnityCore/Variant.h>
#include <UnityCore/GLibSource.h>

#include "UBusServer.h"

namespace unity
{

class UBusManager : public boost::noncopyable
{
public:
  UBusManager();
  ~UBusManager();

  unsigned RegisterInterest(std::string const& interest_name,
                            UBusCallback const& slot);
  void UnregisterInterest(unsigned connection_id);
  static void SendMessage(std::string const& message_name,
                          glib::Variant const& args = glib::Variant(),
                          glib::Source::Priority prio = glib::Source::Priority::DEFAULT);

private:
  static std::unique_ptr<UBusServer> server;
  std::set<unsigned> connection_ids_;
};

}

#endif
