// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: William Hua <william.hua@canonical.com>
 */

#include "LockScreenAcceleratorController.h"

#include <UnityCore/GLibDBusProxy.h>

namespace unity
{
namespace lockscreen
{

namespace
{
const char* const MEDIA_KEYS_SCHEMA                = "org.gnome.settings-daemon.plugins.media-keys";
const char* const MEDIA_KEYS_KEY_VOLUME_MUTE       = "volume-mute";
const char* const MEDIA_KEYS_KEY_VOLUME_DOWN       = "volume-down";
const char* const MEDIA_KEYS_KEY_VOLUME_UP         = "volume-up";
const char* const INPUT_SWITCH_SCHEMA              = "org.gnome.desktop.wm.keybindings";
const char* const INPUT_SWITCH_KEY_PREVIOUS_SOURCE = "switch-input-source-backward";
const char* const INPUT_SWITCH_KEY_NEXT_SOURCE     = "switch-input-source";

const char* const INDICATOR_INTERFACE_ACTIONS      = "org.gtk.Actions";
const char* const INDICATOR_METHOD_ACTIVATE        = "Activate";
const char* const INDICATOR_SOUND_BUS_NAME         = "com.canonical.indicator.sound";
const char* const INDICATOR_SOUND_OBJECT_PATH      = "/com/canonical/indicator/sound";
const char* const INDICATOR_SOUND_ACTION_MUTE      = "mute";
const char* const INDICATOR_SOUND_ACTION_SCROLL    = "scroll";
const char* const INDICATOR_KEYBOARD_BUS_NAME      = "com.canonical.indicator.keyboard";
const char* const INDICATOR_KEYBOARD_OBJECT_PATH   = "/com/canonical/indicator/keyboard";
const char* const INDICATOR_KEYBOARD_ACTION_SCROLL = "locked_scroll";

void ActivateIndicator(std::string const& bus_name,
                       std::string const& object_path,
                       std::string const& action_name,
                       glib::Variant const& parameters = glib::Variant())
{
  GVariantBuilder builder;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("(sava{sv})"));
  g_variant_builder_add(&builder, "s", action_name.c_str());

  if (parameters)
    g_variant_builder_add_parsed(&builder, "[%v]", static_cast<GVariant*>(parameters));
  else
    g_variant_builder_add_parsed(&builder, "@av []");

  g_variant_builder_add_parsed(&builder, "@a{sv} []");

  auto proxy = std::make_shared<glib::DBusProxy>(bus_name, object_path, INDICATOR_INTERFACE_ACTIONS);
  proxy->CallBegin(INDICATOR_METHOD_ACTIVATE, g_variant_builder_end(&builder), [proxy] (GVariant*, glib::Error const&) {});
}

void MuteIndicatorSound()
{
  ActivateIndicator(INDICATOR_SOUND_BUS_NAME,
                    INDICATOR_SOUND_OBJECT_PATH,
                    INDICATOR_SOUND_ACTION_MUTE);
}

void ScrollIndicatorSound(int offset)
{
  ActivateIndicator(INDICATOR_SOUND_BUS_NAME,
                    INDICATOR_SOUND_OBJECT_PATH,
                    INDICATOR_SOUND_ACTION_SCROLL,
                    g_variant_new_int32(offset));
}

void ScrollIndicatorKeyboard(int offset)
{
  ActivateIndicator(INDICATOR_KEYBOARD_BUS_NAME,
                    INDICATOR_KEYBOARD_OBJECT_PATH,
                    INDICATOR_KEYBOARD_ACTION_SCROLL,
                    g_variant_new_int32(-offset));
}
} // namespace

AcceleratorController::AcceleratorController()
  : accelerators_(new Accelerators)
{
  ReloadAccelerators();
}

void AcceleratorController::ReloadAccelerators() const
{
  accelerators_->Clear();

  auto settings = glib::Object<GSettings>(g_settings_new(MEDIA_KEYS_SCHEMA));

  auto accelerator = std::make_shared<Accelerator>(glib::String(g_settings_get_string(settings, MEDIA_KEYS_KEY_VOLUME_MUTE)));
  accelerator->activated.connect(std::function<void ()>(MuteIndicatorSound));
  accelerators_->Add(accelerator);

  accelerator = std::make_shared<Accelerator>(glib::String(g_settings_get_string(settings, MEDIA_KEYS_KEY_VOLUME_DOWN)));
  accelerator->activated.connect(std::bind(ScrollIndicatorSound, -1));
  accelerators_->Add(accelerator);

  accelerator = std::make_shared<Accelerator>(glib::String(g_settings_get_string(settings, MEDIA_KEYS_KEY_VOLUME_UP)));
  accelerator->activated.connect(std::bind(ScrollIndicatorSound, +1));
  accelerators_->Add(accelerator);

  settings = glib::Object<GSettings>(g_settings_new(INPUT_SWITCH_SCHEMA));

  auto variant = glib::Variant(g_settings_get_value(settings, INPUT_SWITCH_KEY_PREVIOUS_SOURCE), glib::StealRef());

  if (g_variant_n_children(variant) > 0)
  {
    const gchar* string;

    g_variant_get_child(variant, 0, "&s", &string);

    accelerator = std::make_shared<Accelerator>(string);
    accelerator->activated.connect(std::bind(ScrollIndicatorKeyboard, -1));
    accelerators_->Add(accelerator);
  }

  variant = glib::Variant(g_settings_get_value(settings, INPUT_SWITCH_KEY_NEXT_SOURCE), glib::StealRef());

  if (g_variant_n_children(variant) > 0)
  {
    const gchar* string;

    g_variant_get_child(variant, 0, "&s", &string);

    accelerator = std::make_shared<Accelerator>(string);
    accelerator->activated.connect(std::bind(ScrollIndicatorKeyboard, +1));
    accelerators_->Add(accelerator);
  }
}

Accelerators::Ptr const& AcceleratorController::GetAccelerators() const
{
  return accelerators_;
}

} // lockscreen namespace
} // unity namespace
