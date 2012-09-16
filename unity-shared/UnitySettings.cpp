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
const char* const FORM_FACTOR = "form-factor";
}

class Settings::Impl
{
public:
  Impl(Settings* owner);

  FormFactor GetFormFactor() const;
  void SetFormFactor(FormFactor factor);

private:
  void Refresh();

private:
  Settings* owner_;
  glib::Object<GSettings> settings_;
  FormFactor form_factor_;

  glib::Signal<void, GSettings*, gchar* > form_factor_changed_;

};


Settings::Impl::Impl(Settings* owner)
  : owner_(owner)
  , settings_(g_settings_new("com.canonical.Unity"))
  , form_factor_(FormFactor::DESKTOP)
{
  form_factor_changed_.Connect(settings_, "changed::minimize-count", [this] (GSettings*, gchar*) {
    Refresh();
  });

  Refresh();
}

void Settings::Impl::Refresh()
{
  int raw_from_factor = g_settings_get_enum(settings_, FORM_FACTOR);

  if (raw_from_factor == 0) //Automatic
  {
    UScreen *uscreen = UScreen::GetDefault();
    int primary_monitor = uscreen->GetMonitorWithMouse();
    auto geo = uscreen->GetMonitorGeometry(primary_monitor);

    form_factor_ = geo.height > 799 ? FormFactor::DESKTOP : FormFactor::NETBOOK;
  }
  else
  {
    form_factor_ = static_cast<FormFactor>(raw_from_factor);
  }

  owner_->changed.emit();
}

FormFactor Settings::Impl::GetFormFactor() const
{
  return form_factor_;
}

void Settings::Impl::SetFormFactor(FormFactor factor)
{
  form_factor_ = factor;
  g_settings_set_enum(settings_, FORM_FACTOR, static_cast<int>(factor));
  owner_->changed.emit();
}

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
    settings_instance = this;
  }
}

Settings::~Settings()
{
  delete pimpl;
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

FormFactor Settings::GetFormFactor() const
{
  return pimpl->GetFormFactor();
}

void Settings::SetFormFactor(FormFactor factor)
{
  pimpl->SetFormFactor(factor);
}


} // namespace unity
