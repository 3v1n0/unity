// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
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

#include "UpstartWrapper.h"

#include <NuxCore/Logger.h>

#include <upstart.h>
#include <nih/alloc.h>
#include <nih/error.h>

namespace unity
{

DECLARE_LOGGER(logger, "unity.upstart");

//
// Start private implementation
//

class UpstartWrapper::Impl
{
public:
  Impl();
  ~Impl();

  void Emit(std::string const& name);

private:
  NihDBusProxy* upstart_;
};

UpstartWrapper::Impl::Impl()
  : upstart_(nullptr)
{
  const gchar* upstart_session = g_getenv("UPSTART_SESSION");

  if (upstart_session)
  {
    DBusConnection* conn = dbus_connection_open(upstart_session, nullptr);

    if (conn)
    {
      upstart_ = nih_dbus_proxy_new(nullptr, conn,
                                    nullptr,
                                    DBUS_PATH_UPSTART,
                                    nullptr, nullptr);
      if (!upstart_)
      {
        NihError* err = nih_error_get();
        LOG_WARN(logger) << "Unable to get Upstart proxy: " << err->message << std::endl;
        nih_free(err);
      }

      dbus_connection_unref (conn);
    }
  }
}

UpstartWrapper::Impl::~Impl()
{
  if (upstart_)
    nih_unref(upstart_, nullptr);
}

void UpstartWrapper::Impl::Emit(std::string const& name)
{
  if (upstart_)
  {
    int event_sent = upstart_emit_event_sync(nullptr, upstart_,
                                             name.c_str(), nullptr, 0);
    if (event_sent)
    {
      NihError * err = nih_error_get();
      LOG_WARN(logger) << "Unable to signal for indicator services to stop: " <<  err->message << std::endl;;
      nih_free(err);
    }
  }
}

//
// End private implementation
//

UpstartWrapper::UpstartWrapper()
  : pimpl_(new Impl)
{}

UpstartWrapper::~UpstartWrapper()
{}

void UpstartWrapper::Emit(std::string const& name)
{
  pimpl_->Emit(name);
}

}
