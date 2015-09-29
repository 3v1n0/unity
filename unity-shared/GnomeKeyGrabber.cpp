// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013-2015 Canonical Ltd
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

#include "GnomeKeyGrabberImpl.h"

#include <NuxCore/Logger.h>

namespace unity
{
namespace key
{
DECLARE_LOGGER(logger, "unity.key.gnome.grabber");

// Private implementation
namespace shell
{
std::string const DBUS_NAME = "org.gnome.Shell";
std::string const DBUS_INTERFACE = "org.gnome.Shell";
std::string const DBUS_OBJECT_PATH = "/org/gnome/Shell";
std::string const INTROSPECTION_XML =
R"(<node>
  <interface name='org.gnome.Shell'>
    <method name='GrabAccelerators'>
      <arg type='a(su)' direction='in' name='accelerators'/>
      <arg type='au' direction='out' name='actions'/>
    </method>
    <method name='GrabAccelerator'>
      <arg type='s' direction='in' name='accelerator'/>
      <arg type='u' direction='in' name='flags'/>
      <arg type='u' direction='out' name='action'/>
    </method>
    <method name='UngrabAccelerator'>
      <arg type='u' direction='in' name='action'/>
      <arg type='b' direction='out' name='success'/>
    </method>
    <signal name='AcceleratorActivated'>
      <arg type='u' name='action'/>
      <arg type='u' name='device'/>
      <arg type='u' name='timestamp'/>
    </signal>
  </interface>
</node>)";
}

namespace testing
{
std::string const DBUS_NAME = "com.canonical.Unity.Test.GnomeKeyGrabber";
}

GnomeGrabber::Impl::Impl(bool test_mode)
  : screen_(screen)
  , shell_server_(test_mode ? testing::DBUS_NAME : shell::DBUS_NAME)
  , current_action_id_(0)
{
  shell_server_.AddObjects(shell::INTROSPECTION_XML, shell::DBUS_OBJECT_PATH);
  shell_object_ = shell_server_.GetObject(shell::DBUS_INTERFACE);
  shell_object_->SetMethodsCallsHandler(sigc::mem_fun(this, &Impl::OnShellMethodCall));
}

GnomeGrabber::Impl::~Impl()
{
  for (auto& action : actions_)
    screen_->removeAction(&action);
}

uint32_t GnomeGrabber::Impl::NextActionID()
{
  return ++current_action_id_;
}

bool GnomeGrabber::Impl::AddAction(CompAction const& action, uint32_t action_id)
{
  LOG_DEBUG(logger) << "AddAction (\"" << action.keyToString() << "\") = " << action_id;

  if (screen_->addAction(const_cast<CompAction*>(&action)))
  {
    actions_ids_.push_back(action_id);
    actions_.push_back(action);
    return true;
  }

  LOG_ERROR(logger) << "Impossible to grab action \"" << action.keyToString() << "\"";
  return false;
}

bool GnomeGrabber::Impl::AddAction(CompAction const& action)
{
  return AddAction(action, NextActionID());
}

bool GnomeGrabber::Impl::RemoveAction(CompAction const& action)
{
  for (size_t index = 0; index < actions_.size(); ++index)
  {
    if (actions_[index] == action)
      return RemoveAction(index);
  }

  return false;
}

bool GnomeGrabber::Impl::RemoveAction(uint32_t action_id)
{
  if (!action_id)
    return false;

  size_t index = 0;

  for (auto id : actions_ids_)
  {
    if (id == action_id)
      return RemoveAction(index);

    ++index;
  }

  return false;
}

bool GnomeGrabber::Impl::RemoveAction(size_t index)
{
  if (!index || index >= actions_.size())
    return false;

  CompAction* action = &(actions_[index]);
  LOG_DEBUG(logger) << "RemoveAction (\"" << action->keyToString() << "\")";

  screen_->removeAction(action);
  actions_.erase(actions_.begin() + index);
  actions_ids_.erase(actions_ids_.begin() + index);

  return true;
}

GVariant* GnomeGrabber::Impl::OnShellMethodCall(std::string const& method, GVariant* parameters)
{
  LOG_DEBUG(logger) << "Called method '" << method << "'";

  if (method == "GrabAccelerators")
  {
    if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(a(su))")))
    {
      GVariant* variant;
      GVariantBuilder builder;
      GVariantIter* iterator;
      gchar const* accelerator;
      guint flags;

      g_variant_builder_init(&builder, G_VARIANT_TYPE("au"));
      g_variant_get(parameters, "(a(su))", &iterator);

      while (g_variant_iter_next(iterator, "(&su)", &accelerator, &flags))
        g_variant_builder_add(&builder, "u", GrabAccelerator(accelerator, flags));

      g_variant_iter_free(iterator);
      variant = g_variant_builder_end(&builder);
      return g_variant_new_tuple(&variant, 1);
    }
    else
    {
      LOG_WARN(logger) << "Expected arguments of type (a(su))";
    }
  }
  else if (method == "GrabAccelerator")
  {
    if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(su)")))
    {
      gchar const* accelerator;
      guint flags;
      g_variant_get(parameters, "(&su)", &accelerator, &flags);

      if (uint32_t action_id = GrabAccelerator(accelerator, flags))
        return g_variant_new("(u)", action_id);
    }
    else
    {
      LOG_WARN(logger) << "Expected arguments of type (su)";
    }
  }
  else if (method == "UngrabAccelerator")
  {
    uint32_t action_id;
    g_variant_get(parameters, "(u)", &action_id);
    return g_variant_new("(b)", RemoveAction(action_id));
  }

  return nullptr;
}

uint32_t GnomeGrabber::Impl::GrabAccelerator(char const* accelerator, uint32_t flags)
{
  CompAction action;
  action.keyFromString(accelerator);
  uint32_t action_id = NextActionID();

  if (action.key().toString().empty())
  {
    CompString prefixed = "XF86" + CompString(accelerator);
    LOG_DEBUG(logger) << "Can't grab \"" << accelerator << "\", trying \"" << prefixed << "\"";
    action.keyFromString(prefixed);
  }
  else
  {
    LOG_DEBUG(logger) << "grabAccelerator \"" << accelerator << "\"";
  }

  if (!IsActionPostponed(action))
  {
    action.setState(CompAction::StateInitKey);
    action.setInitiate([this, action_id](CompAction* action, CompAction::State state, CompOption::Vector& options) {
      LOG_DEBUG(logger) << "pressed \"" << action->keyToString() << "\"";
      ActivateAction(*action, action_id, 0, options[7].value().i());
      return true;
    });
  }
  else
  {
    action.setState(CompAction::StateInitKey | CompAction::StateTermKey);
    action.setTerminate([this, action_id](CompAction* action, CompAction::State state, CompOption::Vector& options) {
      auto key = action->keyToString();

      LOG_DEBUG(logger) << "released \"" << key << "\"";

      if (state & CompAction::StateTermTapped)
      {
        LOG_DEBUG(logger) << "tapped \"" << key << "\"";
        ActivateAction(*action, action_id, 0, options[7].value().i());
        return true;
      }

      return false;
    });
  }

  return AddAction(action, action_id) ? action_id : 0;
}

void GnomeGrabber::Impl::ActivateAction(CompAction const& action, uint32_t action_id, uint32_t device, uint32_t timestamp) const
{
  LOG_DEBUG(logger) << "ActivateAction (" << action_id << " \"" << action.keyToString() << "\")";
  shell_object_->EmitSignal("AcceleratorActivated", g_variant_new("(uuu)", action_id, device, timestamp));
}

bool GnomeGrabber::Impl::IsActionPostponed(CompAction const& action) const
{
  int keycode = action.key().keycode();
  return keycode == 0 || modHandler->keycodeToModifiers(keycode) != 0;
}

// Public implementation

GnomeGrabber::GnomeGrabber()
  : impl_(new Impl())
{}

GnomeGrabber::GnomeGrabber(TestMode const& dummy)
  : impl_(new Impl(true))
{}

GnomeGrabber::~GnomeGrabber()
{}

CompAction::Vector& GnomeGrabber::GetActions()
{
  return impl_->actions_;
}

void GnomeGrabber::AddAction(CompAction const& action)
{
  impl_->AddAction(action);
}

void GnomeGrabber::RemoveAction(CompAction const& action)
{
  impl_->RemoveAction(action);
}

} // namespace key
} // namespace unity
