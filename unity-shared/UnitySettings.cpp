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

#include <glib.h>
#include <NuxCore/Logger.h>
#include <UnityCore/GLibSource.h>
#include <UnityCore/Variant.h>

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
const std::string DESKTOP_TYPE = "desktop-type";
const std::string PAM_CHECK_ACCOUNT_TYPE = "pam-check-account-type";

const std::string LAUNCHER_SETTINGS = "com.canonical.Unity.Launcher";
const std::string LAUNCHER_POSITION = "launcher-position";

const std::string LIM_SETTINGS = "com.canonical.Unity.IntegratedMenus";
const std::string CLICK_MOVEMENT_THRESHOLD = "click-movement-threshold";
const std::string DOUBLE_CLICK_WAIT = "double-click-wait";
const std::string UNFOCUSED_MENU_POPUP = "unfocused-windows-popup";

const std::string UI_SETTINGS = "com.canonical.Unity.Interface";
const std::string TEXT_SCALE_FACTOR = "text-scale-factor";
const std::string CURSOR_SCALE_FACTOR = "cursor-scale-factor";
const std::string APP_SCALE_MONITOR = "app-scale-factor-monitor";
const std::string APP_USE_MAX_SCALE = "app-fallback-to-maximum-scale-factor";

const std::string UBUNTU_UI_SETTINGS = "com.ubuntu.user-interface";
const std::string SCALE_FACTOR = "scale-factor";

const std::string GNOME_UI_SETTINGS = "org.gnome.desktop.interface";
const std::string GNOME_FONT_NAME = "font-name";
const std::string GNOME_CURSOR_SIZE = "cursor-size";
const std::string GNOME_SCALE_FACTOR = "scaling-factor";
const std::string GNOME_TEXT_SCALE_FACTOR = "text-scaling-factor";

const std::string REMOTE_CONTENT_SETTINGS = "com.canonical.Unity.Lenses";
const std::string REMOTE_CONTENT_KEY = "remote-content-search";

const std::string GESTURES_SETTINGS = "com.canonical.Unity.Gestures";
const std::string LAUNCHER_DRAG = "launcher-drag";
const std::string DASH_TAP = "dash-tap";
const std::string WINDOWS_DRAG_PINCH = "windows-drag-pinch";

const int DEFAULT_LAUNCHER_SIZE = 64;
const int MINIMUM_DESKTOP_HEIGHT = 800;
const int GNOME_SETTINGS_CHANGED_WAIT_SECONDS = 1;
const double DEFAULT_DPI = 96.0f;
}

//
// Start private implementation
//
class Settings::Impl : public sigc::trackable
{
public:
  Impl(Settings* owner)
    : parent_(owner)
    , usettings_(g_settings_new(SETTINGS_NAME.c_str()))
    , launcher_settings_(g_settings_new(LAUNCHER_SETTINGS.c_str()))
    , lim_settings_(g_settings_new(LIM_SETTINGS.c_str()))
    , gestures_settings_(g_settings_new(GESTURES_SETTINGS.c_str()))
    , ui_settings_(g_settings_new(UI_SETTINGS.c_str()))
    , ubuntu_ui_settings_(g_settings_new(UBUNTU_UI_SETTINGS.c_str()))
    , gnome_ui_settings_(g_settings_new(GNOME_UI_SETTINGS.c_str()))
    , remote_content_settings_(g_settings_new(REMOTE_CONTENT_SETTINGS.c_str()))
    , launcher_sizes_(monitors::MAX, DEFAULT_LAUNCHER_SIZE)
    , cached_launcher_position_(LauncherPosition::LEFT)
    , cached_form_factor_(FormFactor::DESKTOP)
    , cursor_scale_(1.0)
    , cached_double_click_activate_(true)
    , changing_gnome_settings_(false)
    , lowGfx_(false)
    , remote_content_enabled_(true)
  {
    parent_->form_factor.SetGetterFunction(sigc::mem_fun(this, &Impl::GetFormFactor));
    parent_->form_factor.SetSetterFunction(sigc::mem_fun(this, &Impl::SetFormFactor));
    parent_->double_click_activate.SetGetterFunction(sigc::mem_fun(this, &Impl::GetDoubleClickActivate));
    parent_->remote_content.SetGetterFunction(sigc::mem_fun(this, &Impl::GetRemoteContentEnabled));
    parent_->launcher_position.SetGetterFunction(sigc::mem_fun(this, &Impl::GetLauncherPosition));
    parent_->launcher_position.SetSetterFunction(sigc::mem_fun(this, &Impl::SetLauncherPosition));
    parent_->desktop_type.SetGetterFunction(sigc::mem_fun(this, &Impl::GetDesktopType));
    parent_->pam_check_account_type.SetGetterFunction(sigc::mem_fun(this, &Impl::GetPamCheckAccountType));

    for (unsigned i = 0; i < monitors::MAX; ++i)
      em_converters_.emplace_back(std::make_shared<EMConverter>());

    signals_.Add<void, GSettings*, const gchar*>(usettings_, "changed::" + FORM_FACTOR, [this] (GSettings*, const gchar*) {
      CacheFormFactor();
    });

    signals_.Add<void, GSettings*, const gchar*>(usettings_, "changed::" + DOUBLE_CLICK_ACTIVATE, [this] (GSettings*, const gchar*) {
      CacheDoubleClickActivate();
      parent_->double_click_activate.changed.emit(cached_double_click_activate_);
    });

    signals_.Add<void, GSettings*, const gchar*>(launcher_settings_, "changed::" + LAUNCHER_POSITION, [this] (GSettings*, const gchar*) {
      CacheLauncherPosition();
      parent_->launcher_position.changed.emit(cached_launcher_position_);
    });

    signals_.Add<void, GSettings*, const gchar*>(ubuntu_ui_settings_, "changed::" + SCALE_FACTOR, [this] (GSettings*, const gchar* t) {
      UpdateDPI();
    });

    signals_.Add<void, GSettings*, const gchar*>(ui_settings_, "changed::" + TEXT_SCALE_FACTOR, [this] (GSettings*, const gchar* t) {
      UpdateTextScaleFactor();
      UpdateDPI();
    });

    signals_.Add<void, GSettings*, const gchar*>(ui_settings_, "changed::" + CURSOR_SCALE_FACTOR, [this] (GSettings*, const gchar* t) {
      UpdateCursorScaleFactor();
      UpdateDPI();
    });

    signals_.Add<void, GSettings*, const gchar*>(ui_settings_, "changed::" + APP_SCALE_MONITOR, [this] (GSettings*, const gchar* t) {
      UpdateDPI();
    });

    signals_.Add<void, GSettings*, const gchar*>(ui_settings_, "changed::" + APP_USE_MAX_SCALE, [this] (GSettings*, const gchar* t) {
      UpdateDPI();
    });

    signals_.Add<void, GSettings*, const gchar*>(gnome_ui_settings_, "changed::" + GNOME_FONT_NAME, [this] (GSettings*, const gchar* t) {
      UpdateFontSize();
      UpdateDPI();
    });

    signals_.Add<void, GSettings*, const gchar*>(gnome_ui_settings_, "changed::" + GNOME_TEXT_SCALE_FACTOR, [this] (GSettings*, const gchar* t) {
      if (!changing_gnome_settings_)
      {
        double new_scale_factor = g_settings_get_double(gnome_ui_settings_, GNOME_TEXT_SCALE_FACTOR.c_str());
        g_settings_set_double(ui_settings_, TEXT_SCALE_FACTOR.c_str(), new_scale_factor);
      }
    });

    signals_.Add<void, GSettings*, const gchar*>(lim_settings_, "changed", [this] (GSettings*, const gchar*) {
      UpdateLimSetting();
    });

    signals_.Add<void, GSettings*, const gchar*>(gestures_settings_, "changed", [this] (GSettings*, const gchar*) {
      UpdateGesturesSetting();
    });

    signals_.Add<void, GSettings*, const gchar*>(remote_content_settings_, "changed::" + REMOTE_CONTENT_KEY, [this] (GSettings*, const gchar* t) {
      UpdateRemoteContentSearch();
    });

    UScreen::GetDefault()->changed.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &Impl::UpdateDPI))));

    // The order is important here, DPI is the last thing to be updated
    UpdateLimSetting();
    UpdateGesturesSetting();
    UpdateTextScaleFactor();
    UpdateCursorScaleFactor();
    UpdateFontSize();
    UpdateDPI();

    CacheFormFactor();
    CacheDoubleClickActivate();
    UpdateRemoteContentSearch();
    CacheLauncherPosition();
  }

  void CacheFormFactor()
  {
    int raw_from_factor = g_settings_get_enum(usettings_, FORM_FACTOR.c_str());
    FormFactor new_form_factor;

    if (raw_from_factor == 0) //Automatic
    {
      auto uscreen = UScreen::GetDefault();
      int primary_monitor = uscreen->GetPrimaryMonitor();
      auto const& geo = uscreen->GetMonitorGeometry(primary_monitor);
      double monitor_scaling = em(primary_monitor)->DPIScale();

      new_form_factor = (geo.height * monitor_scaling) >= MINIMUM_DESKTOP_HEIGHT ? FormFactor::DESKTOP : FormFactor::NETBOOK;
    }
    else
    {
      new_form_factor = static_cast<FormFactor>(raw_from_factor);
    }

    if (new_form_factor != cached_form_factor_)
    {
      cached_form_factor_ = new_form_factor;
      parent_->form_factor.changed.emit(cached_form_factor_);
    }
  }

  void CacheDoubleClickActivate()
  {
    cached_double_click_activate_ = g_settings_get_boolean(usettings_, DOUBLE_CLICK_ACTIVATE.c_str());
  }

  void CacheLauncherPosition()
  {
    cached_launcher_position_ = static_cast<LauncherPosition>(g_settings_get_enum(launcher_settings_, LAUNCHER_POSITION.c_str()));
  }

  void UpdateLimSetting()
  {
    parent_->lim_movement_thresold = g_settings_get_uint(lim_settings_, CLICK_MOVEMENT_THRESHOLD.c_str());
    parent_->lim_double_click_wait = g_settings_get_uint(lim_settings_, DOUBLE_CLICK_WAIT.c_str());
    parent_->lim_unfocused_popup = g_settings_get_boolean(lim_settings_, UNFOCUSED_MENU_POPUP.c_str());
  }

  void UpdateGesturesSetting()
  {
    parent_->gestures_launcher_drag = g_settings_get_boolean(gestures_settings_, LAUNCHER_DRAG.c_str());
    parent_->gestures_dash_tap = g_settings_get_boolean(gestures_settings_, DASH_TAP.c_str());
    parent_->gestures_windows_drag_pinch = g_settings_get_boolean(gestures_settings_, WINDOWS_DRAG_PINCH.c_str());
    parent_->gestures_changed.emit();
  }

  FormFactor GetFormFactor() const
  {
    return cached_form_factor_;
  }

  bool SetFormFactor(FormFactor factor)
  {
    g_settings_set_enum(usettings_, FORM_FACTOR.c_str(), static_cast<int>(factor));
    return false;
  }

  bool GetDoubleClickActivate() const
  {
    return cached_double_click_activate_;
  }

  LauncherPosition GetLauncherPosition() const
  {
    return cached_launcher_position_;
  }

  bool SetLauncherPosition(LauncherPosition launcherPosition)
  {
    g_settings_set_enum(launcher_settings_, LAUNCHER_POSITION.c_str(), static_cast<int>(launcherPosition));
    return false;
  }

  DesktopType GetDesktopType() const
  {
    return static_cast<DesktopType>(g_settings_get_enum(usettings_, DESKTOP_TYPE.c_str()));
  }

  bool GetPamCheckAccountType() const
  {
    return g_settings_get_boolean(usettings_, PAM_CHECK_ACCOUNT_TYPE.c_str());
  }

  int GetFontSize() const
  {
    gint font_size;
    PangoFontDescription* desc;

    glib::String font_name(g_settings_get_string(gnome_ui_settings_, GNOME_FONT_NAME.c_str()));
    desc = pango_font_description_from_string(font_name);
    font_size = pango_font_description_get_size(desc);
    pango_font_description_free(desc);

    return font_size / 1024;
  }

  void UpdateFontSize()
  {
    int font_size = GetFontSize();

    for (auto const& em : em_converters_)
      em->SetFontSize(font_size);
  }

  void UpdateTextScaleFactor()
  {
    parent_->font_scaling = g_settings_get_double(ui_settings_, TEXT_SCALE_FACTOR.c_str());
    decoration::Style::Get()->font_scale = parent_->font_scaling();
  }

  void UpdateCursorScaleFactor()
  {
    cursor_scale_ = g_settings_get_double(ui_settings_, CURSOR_SCALE_FACTOR.c_str());
  }

  EMConverter::Ptr const& em(int monitor) const
  {
    if (monitor < 0 || monitor >= (int)monitors::MAX)
    {
      LOG_ERROR(logger) << "Invalid monitor index: " << monitor << ". Returning index 0 monitor instead.";
      return em_converters_[0];
    }

    return em_converters_[monitor];
  }

  void UpdateDPI()
  {
    auto* uscreen = UScreen::GetDefault();
    double min_scale = 4.0;
    double max_scale = 0.0;
    bool any_changed = false;

    glib::Variant dict;
    g_settings_get(ubuntu_ui_settings_, SCALE_FACTOR.c_str(), "@a{si}", &dict);

    glib::String app_target_monitor(g_settings_get_string(ui_settings_, APP_SCALE_MONITOR.c_str()));
    double app_target_scale = 0;

    for (unsigned monitor = 0; monitor < em_converters_.size(); ++monitor)
    {
      int dpi = DEFAULT_DPI;

      if (monitor < uscreen->GetMonitors().size())
      {
        auto const& monitor_name = uscreen->GetMonitorName(monitor);
        double ui_scale = 1.0f;
        int value;

        if (g_variant_lookup(dict, monitor_name.c_str(), "i", &value) && value > 0)
          ui_scale = static_cast<double>(value)/8.0f;

        if (app_target_monitor.Str() == monitor_name)
          app_target_scale = ui_scale;

        dpi = DEFAULT_DPI * ui_scale;
        min_scale = std::min(min_scale, ui_scale);
        max_scale = std::max(max_scale, ui_scale);
      }

      if (em_converters_[monitor]->SetDPI(dpi))
        any_changed = true;
    }

    if (app_target_scale == 0)
      app_target_scale = (g_settings_get_boolean(ui_settings_, APP_USE_MAX_SCALE.c_str())) ? max_scale : min_scale;

    UpdateAppsScaling(app_target_scale);

    if (any_changed)
      parent_->dpi_changed.emit();
  }

  void UpdateAppsScaling(double scale)
  {
    changing_gnome_settings_ = true;
    changing_gnome_settings_timeout_.reset();
    unsigned integer_scaling = std::max<unsigned>(1, scale);
    double point_scaling = scale / static_cast<double>(integer_scaling);
    double text_scale_factor = parent_->font_scaling() * point_scaling;
    glib::Variant default_cursor_size(g_settings_get_default_value(gnome_ui_settings_, GNOME_CURSOR_SIZE.c_str()), glib::StealRef());
    int cursor_size = std::round(default_cursor_size.GetInt32() * point_scaling * cursor_scale_);
    g_settings_set_int(gnome_ui_settings_, GNOME_CURSOR_SIZE.c_str(), cursor_size);
    g_settings_set_uint(gnome_ui_settings_, GNOME_SCALE_FACTOR.c_str(), integer_scaling);
    g_settings_set_double(gnome_ui_settings_, GNOME_TEXT_SCALE_FACTOR.c_str(), text_scale_factor);

    changing_gnome_settings_timeout_.reset(new glib::TimeoutSeconds(GNOME_SETTINGS_CHANGED_WAIT_SECONDS, [this] {
      changing_gnome_settings_ = false;
      return false;
    }, glib::Source::Priority::LOW));
  }

  void UpdateRemoteContentSearch()
  {
    glib::String remote_content(g_settings_get_string(remote_content_settings_, REMOTE_CONTENT_KEY.c_str()));
    bool remote_content_enabled = ((remote_content.Str() == "all") ? true : false);

    if (remote_content_enabled != remote_content_enabled_)
    {
      remote_content_enabled_ = remote_content_enabled;
      parent_->remote_content.changed.emit(remote_content_enabled_);
    }
  }

  bool GetRemoteContentEnabled() const
  {
    return remote_content_enabled_;
  }

  Settings* parent_;
  glib::Object<GSettings> usettings_;
  glib::Object<GSettings> launcher_settings_;
  glib::Object<GSettings> lim_settings_;
  glib::Object<GSettings> gestures_settings_;
  glib::Object<GSettings> ui_settings_;
  glib::Object<GSettings> ubuntu_ui_settings_;
  glib::Object<GSettings> gnome_ui_settings_;
  glib::Object<GSettings> remote_content_settings_;
  glib::Source::UniquePtr changing_gnome_settings_timeout_;
  glib::SignalManager signals_;
  std::vector<EMConverter::Ptr> em_converters_;
  std::vector<int> launcher_sizes_;
  LauncherPosition cached_launcher_position_;
  FormFactor cached_form_factor_;
  double cursor_scale_;
  bool cached_double_click_activate_;
  bool changing_gnome_settings_;
  bool lowGfx_;
  bool remote_content_enabled_;
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
    return;
  }

  settings_instance = this;
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
  if (pimpl->lowGfx_ != low_gfx)
  {
    pimpl->lowGfx_ = low_gfx;

    low_gfx_changed.emit();
  }
}

EMConverter::Ptr const& Settings::em(int monitor) const
{
  return pimpl->em(monitor);
}

void Settings::SetLauncherSize(int launcher_size, int monitor)
{
  if (monitor < 0 || monitor >= (int)monitors::MAX)
  {
    LOG_ERROR(logger) << "Invalid monitor index: " << monitor << ". Not updating launcher size.";
  }
  else
  {
    pimpl->launcher_sizes_[monitor] = launcher_size;
  }
}

int Settings::LauncherSize(int monitor) const
{
  if (monitor < 0 || monitor >= (int)monitors::MAX)
  {
    LOG_ERROR(logger) << "Invalid monitor index: " << monitor << ". Returning 0.";
    return 0;
  }

  return pimpl->launcher_sizes_[monitor];
}

} // namespace unity
