// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010, 2011 Canonical Ltd
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
*/

#include <gdk/gdk.h>
#include <gio/gio.h>

#include <NuxCore/Logger.h>

#include "UnitySettings.h"
#include "UScreen.h"

namespace unity
{
namespace
{
nux::logging::Logger logger("unity");

Settings* settings_instance = nullptr;

const std::string SETTINGS_NAME = "com.canonical.Unity";
const std::string FORM_FACTOR = "form-factor";
}

//
// Start private implementation
//
class Settings::Impl
{

public:
  Impl(Settings* owner)
    : parent_(owner)
    , gsettings_(g_settings_new(SETTINGS_NAME.c_str()))
    , cached_form_factor_(FormFactor::DESKTOP)
  {
    CacheFormFactor();

    form_factor_changed_.Connect(gsettings_, "changed::" + FORM_FACTOR, [this] (GSettings*, gchar*) {
      CacheFormFactor();
      parent_->form_factor.changed.emit(cached_form_factor_);
    });
  }

  void CacheFormFactor()
  {
    int raw_from_factor = g_settings_get_enum(gsettings_, FORM_FACTOR.c_str());

    if (raw_from_factor == 0) //Automatic
    {
      auto uscreen = UScreen::GetDefault();
      int primary_monitor = uscreen->GetMonitorWithMouse();
      auto const& geo = uscreen->GetMonitorGeometry(primary_monitor);

      cached_form_factor_ = geo.height > 799 ? FormFactor::DESKTOP : FormFactor::NETBOOK;
    }
    else
    {
      cached_form_factor_ = static_cast<FormFactor>(raw_from_factor);
    }
  }

  FormFactor GetFormFactor() const
  {
    return cached_form_factor_;
  }

  bool SetFormFactor(FormFactor factor)
  {
    g_settings_set_enum(gsettings_, FORM_FACTOR.c_str(), static_cast<int>(factor));
    return true;
  }

  Settings* parent_;
  glib::Object<GSettings> gsettings_;
  FormFactor cached_form_factor_;

  glib::Signal<void, GSettings*, gchar* > form_factor_changed_;
};

//
// End private implementation
//

Settings::Settings()
  : is_standalone(false)
  , pimpl(new Impl(this))
{
  if (settings_instance)
  {
    LOG_ERROR(logger) << "More than one unity::Settings created.";
  }
  else
  {
    form_factor.SetGetterFunction(sigc::mem_fun(*pimpl, &Impl::GetFormFactor));
    form_factor.SetSetterFunction(sigc::mem_fun(*pimpl, &Impl::SetFormFactor));

    settings_instance = this;
  }
}

Settings::~Settings()
{
  if (settings_instance == this)
    settings_instance = nullptr;
}

Settings& Settings::Instance()
{
  if (!settings_instance)
  {
    LOG_ERROR(logger) << "No unity::Settings created yet.";
  }

  return *settings_instance;
}


} // namespace unity
