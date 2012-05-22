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
  ~Impl();

  FormFactor GetFormFactor() const;
  void SetFormFactor(FormFactor factor);

private:
  void Refresh();
  static void Changed(GSettings* settings, gchar* key, Impl* self);

private:
  Settings* owner_;
  GSettings* settings_;
  FormFactor form_factor_;
};


Settings::Impl::Impl(Settings* owner)
  : owner_(owner)
  , settings_(nullptr)
  , form_factor_(FormFactor::DESKTOP)
{
  settings_ = g_settings_new("com.canonical.Unity");
  g_signal_connect(settings_, "changed",
                   (GCallback)(Impl::Changed), this);
  Refresh();
}

Settings::Impl::~Impl()
{
  g_object_unref(settings_);
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

void Settings::Impl::Changed(GSettings* settings, char* key, Impl* self)
{
  self->Refresh();
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
