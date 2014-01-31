// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010, 2011, 2012 Canonical Ltd
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
*              Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <NuxCore/Logger.h>

#include "DecorationStyle.h"
#include "MultiMonitor.h"
#include "UnitySettings.h"
#include "UScreen.h"

namespace unity
{
DECLARE_LOGGER(logger, "unity.settings");
namespace
{
Settings* settings_instance = nullptr;

const std::string SETTINGS_NAME = "com.canonical.Unity";
const std::string FORM_FACTOR = "form-factor";
const std::string DOUBLE_CLICK_ACTIVATE = "double-click-activate";
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
    , cached_double_click_activate_(true)
    , lowGfx_(false)
    , em_converters_(monitors::MAX)
  {
    CacheFormFactor();
    CacheDoubleClickActivate();

    form_factor_changed_.Connect(gsettings_, "changed::" + FORM_FACTOR, [this] (GSettings*, gchar*) {
      CacheFormFactor();
      parent_->form_factor.changed.emit(cached_form_factor_);
    });
    double_click_activate_changed_.Connect(gsettings_, "changed::" + DOUBLE_CLICK_ACTIVATE, [this] (GSettings*, gchar*) {
      CacheDoubleClickActivate();
      parent_->double_click_activate.changed.emit(cached_double_click_activate_);
    });

    UpdateEMConverter();
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

  void CacheDoubleClickActivate()
  {
    cached_double_click_activate_ = g_settings_get_boolean(gsettings_, DOUBLE_CLICK_ACTIVATE.c_str());
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

  bool GetDoubleClickActivate() const
  {
    return cached_double_click_activate_;
  }

  int GetFontSize() const
  {
    gint font_size;
    PangoFontDescription* desc;

    desc = pango_font_description_from_string(decoration::Style::Get()->font().c_str());
    font_size = pango_font_description_get_size(desc);
    pango_font_description_free(desc);

    return font_size / 1024;
  }

  // FIXME Add in getting the specific dpi scale from each monitor
  int GetDPI(int monitor = 0) const
  {
    int dpi = 0;
    g_object_get(gtk_settings_get_default(), "gtk-xft-dpi", &dpi, nullptr);

    return dpi / 1024;
  }

  void UpdateFontSize()
  {
    int font_size = GetFontSize();

    for (auto& em : em_converters_)
      em.SetFontSize(font_size);
  }

  void UpdateDPI()
  {
    for (int i = 0; i < (int)em_converters_.size(); ++i)
    {
      int dpi = GetDPI(i);
      em_converters_[i].SetDPI(dpi);
    }
  }

  void UpdateEMConverter()
  {
    UpdateFontSize();
    UpdateDPI();
  }

  Settings* parent_;
  glib::Object<GSettings> gsettings_;
  FormFactor cached_form_factor_;
  bool cached_double_click_activate_;
  bool lowGfx_;

  glib::Signal<void, GSettings*, gchar* > form_factor_changed_;
  glib::Signal<void, GSettings*, gchar* > double_click_activate_changed_;

  std::vector<EMConverter> em_converters_;
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

    double_click_activate.SetGetterFunction(sigc::mem_fun(*pimpl, &Impl::GetDoubleClickActivate));

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

bool Settings::GetLowGfxMode() const
{
  return pimpl->lowGfx_;
}

void Settings::SetLowGfxMode(const bool low_gfx)
{
  pimpl->lowGfx_ = low_gfx;
}

EMConverter const& Settings::em(int monitor) const
{
  if (monitor < 0 || monitor >= (int)monitors::MAX)
  {
    LOG_ERROR(logger) << "Invalid monitor index: " << monitor << ". Returning index 0 monitor instead.";
    return pimpl->em_converters_[0];
  }

  return pimpl->em_converters_[monitor];
}

} // namespace unity
