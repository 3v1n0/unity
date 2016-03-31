/*
 * Copyright (C) 2016 Canonical Ltd
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

#include "LoginManager.h"

#include <NuxCore/Logger.h>
#include "UnityCore/GLibDBusProxy.h"

namespace unity
{
namespace lockscreen
{

DECLARE_LOGGER(logger, "unity.lockscreen.loginmanager");

//
// Private Implementation
//

class LoginManager::Impl
{
public:
  Impl(LoginManager *parent);
  ~Impl();

  void Inhibit(std::string const&);
  void Uninhibit();
  bool IsInhibited();

private:
  LoginManager *parent_;
  std::shared_ptr<glib::DBusProxy> lm_proxy_;
  gint inhibitor_handler_;
};

LoginManager::Impl::Impl(LoginManager *parent)
  : parent_(parent)
  , inhibitor_handler_(-1)
{
  lm_proxy_ = std::make_shared<glib::DBusProxy>("org.freedesktop.login1",
                                                "/org/freedesktop/login1",
                                                "org.freedesktop.login1.Manager",
                                                G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES);

  lm_proxy_->Connect("PrepareForSleep", [this] (GVariant* variant) {
    if (glib::Variant(variant).GetBool())
      parent_->about_to_suspend.emit();
  });

  lm_proxy_->connected.connect(sigc::mem_fun(&parent->connected, &decltype(parent->connected)::emit));
}

LoginManager::Impl::~Impl()
{}

void LoginManager::Impl::Inhibit(std::string const& msg)
{
  if (IsInhibited())
    return;

  GVariant* args = g_variant_new("(ssss)", "sleep", "Unity", msg.c_str(), "delay");

  lm_proxy_->CallWithUnixFdList("Inhibit", args, [this] (GVariant* variant, glib::Error const& e) {
    if (e)
    {
      LOG_WARNING(logger) << "Failed to inhbit suspend";
      return;
    }

    inhibitor_handler_ = glib::Variant(variant).GetInt32();
  });
}

void LoginManager::Impl::Uninhibit()
{
  if (IsInhibited())
  {
    close(inhibitor_handler_);
    inhibitor_handler_ = -1;
  }
}

bool LoginManager::Impl::IsInhibited()
{
  return inhibitor_handler_ >= 0;
}

//
// End Private Implementation
//

LoginManager::LoginManager()
  : pimpl_(new Impl(this))
{}

LoginManager::~LoginManager()
{}

void LoginManager::Inhibit(std::string const& msg)
{
  pimpl_->Inhibit(msg);
}

void LoginManager::Uninhibit()
{
  pimpl_->Uninhibit();
}

bool LoginManager::IsInhibited()
{
  return pimpl_->IsInhibited();
}


}
}
