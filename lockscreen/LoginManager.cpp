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

  void Inhibit(std::string const&, InhibitCallback const&);
  void Uninhibit(gint);

private:
  void SetupProxies();

  LoginManager *parent_;
  std::shared_ptr<glib::DBusProxy> lm_proxy_;
  std::shared_ptr<glib::DBusProxy> ls_proxy_;
};

LoginManager::Impl::Impl(LoginManager *parent)
  : parent_(parent)
{
  SetupProxies();

  parent_->session_active.SetGetterFunction([this] {
    return ls_proxy_->GetProperty("Active").GetBool();
  });

  lm_proxy_->Connect("PrepareForSleep", [this](GVariant* variant) {
    bool about_to_suspend = glib::Variant(variant).GetBool();
    parent_->prepare_for_sleep.emit(about_to_suspend);
  });

  ls_proxy_->ConnectProperty("Active", [this] (GVariant *variant) {
    parent_->session_active.changed.emit(glib::Variant(variant).GetBool());
  });

  lm_proxy_->connected.connect(sigc::mem_fun(&parent->connected, &decltype(parent->connected)::emit));
  ls_proxy_->connected.connect(sigc::mem_fun(&parent->connected, &decltype(parent->connected)::emit));
}

LoginManager::Impl::~Impl()
{}

void LoginManager::Impl::SetupProxies()
{
  const char* session_id = g_getenv("XDG_SESSION_ID");

  lm_proxy_ = std::make_shared<glib::DBusProxy>("org.freedesktop.login1",
                                                "/org/freedesktop/login1",
                                                "org.freedesktop.login1.Manager",
                                                G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES);

  ls_proxy_ = std::make_shared<glib::DBusProxy>("org.freedesktop.login1",
                                                "/org/freedesktop/login1/session/" + glib::gchar_to_string(session_id),
                                                "org.freedesktop.login1.Session",
                                                G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES);
}

void LoginManager::Impl::Inhibit(std::string const& msg, InhibitCallback const& cb)
{
  GVariant* args = g_variant_new("(ssss)", "sleep", "Unity", msg.c_str(), "delay");

  lm_proxy_->CallWithUnixFdList("Inhibit", args, [this, cb] (GVariant* variant, glib::Error const& e) {
    if (e)
    {
      LOG_WARNING(logger) << "Failed to inhbit suspend";
      return;
    }

    cb(glib::Variant(variant).GetInt32());
  });
}

void LoginManager::Impl::Uninhibit(gint inhbit_handler)
{
  if (inhbit_handler >= 0)
    close(inhbit_handler);
}

//
// End Private Implementation
//

LoginManager::LoginManager()
  : pimpl_(new Impl(this))
{}

LoginManager::~LoginManager()
{}

void LoginManager::Inhibit(std::string const& msg, InhibitCallback const& cb)
{
  pimpl_->Inhibit(msg, cb);
}

void LoginManager::Uninhibit(gint inhbit_handler)
{
  pimpl_->Uninhibit(inhbit_handler);
}

}
}
