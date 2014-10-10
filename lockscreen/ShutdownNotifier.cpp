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

#include "ShutdownNotifier.h"

#include "UnityCore/GLibDBusProxy.h"

namespace unity
{
namespace lockscreen
{

//
// Private Implementation
//

class ShutdownNotifier::Impl
{
public:
  Impl();
  ~Impl();

  bool RegisterInterest(ShutdownCallback const& cb);
  void UnregisterInterest();

private:
  std::shared_ptr<glib::DBusProxy> logind_proxy_;
  ShutdownCallback cb_;
  gint delay_inhibit_fd_;
};

ShutdownNotifier::Impl::Impl()
  : logind_proxy_(std::make_shared<glib::DBusProxy>("org.freedesktop.login1",
                                                    "/org/freedesktop/login1",
                                                    "org.freedesktop.login1.Manager",
                                                    G_BUS_TYPE_SYSTEM))
  , delay_inhibit_fd_(-1)
{}

ShutdownNotifier::Impl::~Impl()
{
  UnregisterInterest();
}

bool ShutdownNotifier::Impl::RegisterInterest(ShutdownCallback const& cb)
{
  if (!cb or cb_)
    return false;

  cb_ = cb;

  std::vector<std::string> parameters;
  parameters.push_back("shutdown"); // what
  parameters.push_back("Unity Lockscreen"); // who
  parameters.push_back("Screen Locked"); // why
  parameters.push_back("delay"); // mode

  logind_proxy_->CallWithUnixFdList("Inhibit",
                                    glib::Variant::FromVector(parameters),
                                    [this](GVariant* variant, glib::Error const& e){
                                      // FIXME: we should handle the error.
                                      delay_inhibit_fd_ = glib::Variant(variant).GetUInt32();
                                    });

  logind_proxy_->Connect("PrepareForShutdown", [this](GVariant* variant) {
    bool active = glib::Variant(variant).GetBool();

    if (active)
      cb_();

    UnregisterInterest();
  });

  return true;
}

void ShutdownNotifier::Impl::UnregisterInterest()
{
  if (!cb_)
    return;

  logind_proxy_->DisconnectSignal("PrepareForShutdown");

  if (delay_inhibit_fd_ != -1)
    close(delay_inhibit_fd_);

  cb_ = 0;
  delay_inhibit_fd_ = -1;
}

//
// End Private Implementation
//

ShutdownNotifier::ShutdownNotifier()
  : pimpl_(new(Impl))
{}

ShutdownNotifier::~ShutdownNotifier()
{}

bool ShutdownNotifier::RegisterInterest(ShutdownCallback const& cb)
{
  return pimpl_->RegisterInterest(cb);
}

void ShutdownNotifier::UnregisterInterest()
{
  pimpl_->UnregisterInterest();
}

}
}
