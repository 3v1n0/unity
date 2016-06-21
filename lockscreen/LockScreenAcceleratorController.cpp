// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014-2016 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "LockScreenAcceleratorController.h"

#include <NuxCore/Logger.h>
#include <UnityCore/GLibDBusProxy.h>

namespace unity
{
namespace lockscreen
{

namespace
{
DECLARE_LOGGER(logger, "unity.lockscreen.accelerator.controller");

const std::string MEDIA_KEYS_SCHEMA = "org.gnome.settings-daemon.plugins.media-keys";
const std::vector<std::string> ALLOWED_MEDIA_KEYS = {
  "logout",
  "magnifier",
  "on-screen-keyboard",
  "magnifier-zoom-in",
  "screenreader",
  "pause",
  "stop",
  "toggle-contrast",
  "video-out",
  "volume-down",
  "volume-mute",
  "volume-up",
};

const std::string WM_KEYS_SCHEMA = "org.gnome.desktop.wm.keybindings";
const std::vector<std::string> ALLOWED_WM_KEYS = {
  "switch-input-source",
  "switch-input-source-backward",
};

const std::vector<std::string> ALLOWED_XF86_KEYS = {
  "XF86ScreenSaver",
  "XF86Sleep",
  "XF86Standby",
  "XF86Suspend",
  "XF86Hibernate",
  "XF86PowerOff",
  "XF86MonBrightnessUp",
  "XF86MonBrightnessDown",
  "XF86KbdBrightnessUp",
  "XF86KbdBrightnessDown",
  "XF86KbdLightOnOff",
  "XF86AudioMicMute",
  "XF86Touchpad",
};

bool IsKeyBindingAllowed(std::string const& key)
{
  if (std::find(begin(ALLOWED_XF86_KEYS), end(ALLOWED_XF86_KEYS), key) != end(ALLOWED_XF86_KEYS))
    return true;

  glib::Object<GSettings> media_settings(g_settings_new(MEDIA_KEYS_SCHEMA.c_str()));
  Accelerator key_accelerator(key);

  for (auto const& setting : ALLOWED_MEDIA_KEYS)
  {
    Accelerator media_key(glib::String(g_settings_get_string(media_settings, setting.c_str())).Str());
    if (media_key == key_accelerator)
      return true;
  }

  glib::Object<GSettings> wm_settings(g_settings_new(WM_KEYS_SCHEMA.c_str()));

  for (auto const& setting : ALLOWED_WM_KEYS)
  {
    glib::Variant accels(g_settings_get_value(wm_settings, setting.c_str()), glib::StealRef());
    auto children = g_variant_n_children(accels);

    if (children > 0)
    {
      glib::String value;

      for (auto i = 0u; i < children; ++i)
      {
        g_variant_get_child(accels, 0, "s", &value);

        if (Accelerator(value.Str()) == key_accelerator)
          return true;
      }
    }
  }

  return false;
}

} // namespace

AcceleratorController::AcceleratorController(key::Grabber::Ptr const& key_grabber)
  : accelerators_(new Accelerators)
{
  for (auto const& action : key_grabber->GetActions())
    AddAction(action);

  key_grabber->action_added.connect(sigc::mem_fun(this, &AcceleratorController::AddAction));
  key_grabber->action_removed.connect(sigc::mem_fun(this, &AcceleratorController::RemoveAction));
}

void AcceleratorController::AddAction(CompAction const& action)
{
  if (action.type() != CompAction::BindingTypeKey)
    return;

  auto const& key = action.keyToString();

  if (!IsKeyBindingAllowed(key))
  {
    LOG_DEBUG(logger) << "Action not allowed " << key;
    return;
  }

  auto accelerator = std::make_shared<Accelerator>(key);
  accelerator->activated.connect(sigc::bind(sigc::mem_fun(this, &AcceleratorController::OnActionActivated), action));
  accelerators_->Add(accelerator);
  actions_accelerators_.push_back({action, accelerator});

  LOG_DEBUG(logger) << "Action added " << key;
}

void AcceleratorController::RemoveAction(CompAction const& action)
{
  if (action.type() != CompAction::BindingTypeKey)
    return;

  LOG_DEBUG(logger) << "Removing action " << action.keyToString();

  for (auto it = begin(actions_accelerators_); it != end(actions_accelerators_);)
  {
    if (it->first == action)
    {
      accelerators_->Remove(it->second);
      it = actions_accelerators_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void AcceleratorController::OnActionActivated(CompAction& action)
{
  LOG_DEBUG(logger) << "Activating action " << action.keyToString();

  CompOption::Vector options;

  if (action.state() & CompAction::StateInitKey)
    action.initiate()(&action, 0, options);

  if (action.state() & CompAction::StateTermKey)
    action.terminate()(&action, CompAction::StateTermTapped, options);
}

Accelerators::Ptr const& AcceleratorController::GetAccelerators() const
{
  return accelerators_;
}

} // lockscreen namespace
} // unity namespace
