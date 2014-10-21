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

#include "SuspendNotifier.h"

#include <NuxCore/Logger.h>
#include "UnityCore/GLibDBusProxy.h"

namespace unity
{
namespace lockscreen
{

DECLARE_LOGGER(logger, "unity.lockscreen.Suspendnotifier");

//
// Private Implementation
//

class SuspendNotifier::Impl
{
public:
  Impl();
  ~Impl();

  bool RegisterInterest(SuspendCallback const& cb);
  void UnregisterInterest();

private:
  std::shared_ptr<glib::DBusProxy> logind_proxy_;
  SuspendCallback cb_;
  gint delay_inhibit_fd_;
};

SuspendNotifier::Impl::Impl()
  : logind_proxy_(std::make_shared<glib::DBusProxy>("org.freedesktop.login1",
                                                    "/org/freedesktop/login1",
                                                    "org.freedesktop.login1.Manager",
                                                    G_BUS_TYPE_SYSTEM))
  , delay_inhibit_fd_(-1)
{}

SuspendNotifier::Impl::~Impl()
{
  UnregisterInterest();
  logind_proxy_->DisconnectSignal("PrepareForSleep");
}

bool SuspendNotifier::Impl::RegisterInterest(SuspendCallback const& cb)
{
  /*if (!cb or cb_)
    return false;*/

  cb_ = cb;

  logind_proxy_->CallWithUnixFdList("Inhibit",
                                    g_variant_new("(ssss)", "sleep", "Unity Lockscreen", "Screen is locked", "delay"),
                                    [this](GVariant* variant, glib::Error const& e){
                                      if (e)
                                      {
                                        LOG_ERROR(logger) << "Failed to inhbit suspend";
                                      }
                                      delay_inhibit_fd_ = glib::Variant(variant).GetInt32();
                                      std::cout << "#################" << delay_inhibit_fd_ << std::endl;
                                    });

  logind_proxy_->Connect("PrepareForSleep", [this](GVariant* variant) {
    bool active = glib::Variant(variant).GetBool();

    if (active)
    {
      //g_timeout_add(0, [] (gpointer user_data) {
        //Impl* self = static_cast<Impl*>(user_data);
        /*self->*/cb_();
        /*self->*/UnregisterInterest(); // Actually it's just unlock the inhbit
        //return FALSE;
     // }, this);
    }
    /*else
    {
      RegisterInterest(cb_);
    }*/
  });

  return true;
}

void SuspendNotifier::Impl::UnregisterInterest()
{
  /*if (!cb_)
    return;*/

  if (delay_inhibit_fd_ != -1)
    close(delay_inhibit_fd_);

  //cb_ = 0;
  delay_inhibit_fd_ = -1;
}

//
// End Private Implementation
//

SuspendNotifier::SuspendNotifier()
  : pimpl_(new(Impl))
{}

SuspendNotifier::~SuspendNotifier()
{}

bool SuspendNotifier::RegisterInterest(SuspendCallback const& cb)
{
  return pimpl_->RegisterInterest(cb);
}

void SuspendNotifier::UnregisterInterest()
{
  pimpl_->UnregisterInterest();
}

}
}
