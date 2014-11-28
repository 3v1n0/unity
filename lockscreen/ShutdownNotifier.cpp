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

#include <NuxCore/Logger.h>
#include "UnityCore/GLibDBusProxy.h"

namespace unity
{
namespace lockscreen
{

DECLARE_LOGGER(logger, "unity.lockscreen.shutdownnotifier");

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

  void Inhibit();
  void Uninhibit();
  bool IsInhibited() const;

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

  Inhibit();

  logind_proxy_->Connect("PrepareForShutdown", [this](GVariant* variant) {
    bool active = glib::Variant(variant).GetBool();

    if (active)
    {
      cb_();
      UnregisterInterest();
    }
  });

  return true;
}

void ShutdownNotifier::Impl::UnregisterInterest()
{
  if (!cb_)
    return;

  Uninhibit();

  logind_proxy_->DisconnectSignal("PrepareForShutdown");
  cb_ = nullptr;
}

void ShutdownNotifier::Impl::Inhibit()
{
  if (IsInhibited())
    return;

  GVariant* args = g_variant_new("(ssss)", "shutdown", "Unity Lockscreen", "Screen is locked", "delay");

  logind_proxy_->CallWithUnixFdList("Inhibit", args, [this] (GVariant* variant, glib::Error const& e) {
    if (e)
    {
      LOG_ERROR(logger) << "Failed to inhbit suspend";
    }
    delay_inhibit_fd_ = glib::Variant(variant).GetInt32();
  });
}

void ShutdownNotifier::Impl::Uninhibit()
{
  if (!IsInhibited())
    return;

  close(delay_inhibit_fd_);
  delay_inhibit_fd_ = -1;
}

bool ShutdownNotifier::Impl::IsInhibited() const
{
  return delay_inhibit_fd_ != -1;
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
