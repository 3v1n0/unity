// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
    </signal>
  </interface>
</node>)";
}

namespace testing
{
std::string const DBUS_NAME = "com.canonical.Unity.Test.GnomeKeyGrabber";
}

GnomeGrabber::Impl::Impl(bool test_mode)
  : shell_server_(test_mode ? testing::DBUS_NAME : shell::DBUS_NAME)
  , screen_(screen)
  , current_action_id_(0)
{
  shell_server_.AddObjects(shell::INTROSPECTION_XML, shell::DBUS_OBJECT_PATH);
  shell_object_ = shell_server_.GetObject(shell::DBUS_INTERFACE);
  shell_object_->SetMethodsCallsHandler(sigc::mem_fun(this, &Impl::onShellMethodCall));
}

GnomeGrabber::Impl::~Impl()
{
  if (screen_)
  {
    for (auto& action : actions_)
      screen_->removeAction(&action);
  }
}

unsigned int GnomeGrabber::Impl::addAction(CompAction const& action, bool addressable)
{
  ++current_action_id_;
  actions_.push_back(action);
  action_ids_.push_back(current_action_id_);

  if (addressable)
  {
    action_ids_by_action_[&action] = current_action_id_;
    actions_by_action_id_[current_action_id_] = &action;
  }

  if (screen_)
    screen_->addAction(&actions_.back());

  LOG_DEBUG(logger) << "addAction (\"" << action.keyToString() << "\", " << addressable << ") = " << current_action_id_;

  return current_action_id_;
}

bool GnomeGrabber::Impl::removeAction(CompAction const& action)
{
  auto i = action_ids_by_action_.find(&action);
  return i != action_ids_by_action_.end() && removeAction(i->second);
}

bool GnomeGrabber::Impl::removeAction(unsigned int action_id)
{
  auto i = std::find(action_ids_.begin(), action_ids_.end(), action_id);

  if (i != action_ids_.end())
  {
    auto j = actions_.begin() + (i - action_ids_.begin());
    auto k = actions_by_action_id_.find(action_id);

    LOG_DEBUG(logger) << "removeAction (" << action_id << " \"" << j->keyToString() << "\")";

    if (screen_)
      screen_->removeAction(&(*j));

    if (k != actions_by_action_id_.end())
    {
      action_ids_by_action_.erase(k->second);
      actions_by_action_id_.erase(k);
    }

    action_ids_.erase(i);
    actions_.erase(j);
    return true;
  }

  return false;
}

GVariant* GnomeGrabber::Impl::onShellMethodCall(std::string const& method, GVariant* parameters)
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
        g_variant_builder_add(&builder, "u", grabAccelerator(accelerator, flags));

      g_variant_iter_free(iterator);
      variant = g_variant_builder_end(&builder);
      return g_variant_new_tuple(&variant, 1);
    }
    else
      LOG_WARN(logger) << "Expected arguments of type (a(su))";
  }
  else if (method == "GrabAccelerator")
  {
    if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(su)")))
    {
      GVariant* variant;
      gchar const* accelerator;
      guint flags;

      g_variant_get(parameters, "(&su)", &accelerator, &flags);
      variant = g_variant_new_uint32(grabAccelerator(accelerator, flags));
      return g_variant_new_tuple(&variant, 1);
    }
    else
      LOG_WARN(logger) << "Expected arguments of type (su)";
  }
  else if (method == "UngrabAccelerator")
  {
    if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(u)")))
    {
      GVariant* variant;
      guint action;

      g_variant_get(parameters, "(u)", &action);
      variant = g_variant_new_boolean(removeAction(action));
      return g_variant_new_tuple(&variant, 1);
    }
    else
      LOG_WARN(logger) << "Expected arguments of type (u)";
  }

  return nullptr;
}

unsigned int GnomeGrabber::Impl::grabAccelerator(char const* accelerator, unsigned int flags)
{
  CompAction action;
  action.keyFromString(accelerator);

  if (action.key().toString().empty())
  {
      CompString prefixed = "XF86" + CompString(accelerator);
      LOG_DEBUG(logger) << "Can't grab \"" << accelerator << "\", trying \"" << prefixed << "\"";
      action.keyFromString(prefixed);
  }
  else
      LOG_DEBUG(logger) << "grabAccelerator \"" << accelerator << "\"";

  if (!isActionPostponed(action))
  {
    action.setState(CompAction::StateInitKey);
    action.setInitiate([this](CompAction* action, CompAction::State state, CompOption::Vector& options) {
      LOG_DEBUG(logger) << "pressed \"" << action->keyToString() << "\"";
      activateAction(action, 0);
      return true;
    });
  }
  else
  {
    action.setState(CompAction::StateInitKey | CompAction::StateTermKey);
    action.setTerminate([this](CompAction* action, CompAction::State state, CompOption::Vector& options) {
      auto key = action->keyToString();

      LOG_DEBUG(logger) << "released \"" << key << "\"";

      if (state & CompAction::StateTermTapped)
      {
        LOG_DEBUG(logger) << "tapped \"" << key << "\"";
        activateAction(action, 0);
        return true;
      }

      return false;
    });
  }

  return addAction(action, false);
}

void GnomeGrabber::Impl::activateAction(CompAction const* action, unsigned int device) const
{
  ptrdiff_t i = action - &actions_.front();

  if (0 <= i && i < static_cast<ptrdiff_t>(action_ids_.size()))
  {
    auto action_id = action_ids_[i];

    LOG_DEBUG(logger) << "activateAction (" << action_id << " \"" << action->keyToString() << "\")";
    shell_object_->EmitSignal("AcceleratorActivated", g_variant_new("(uu)", action_id, device));
  }
}

bool GnomeGrabber::Impl::isActionPostponed(CompAction const& action) const
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
  impl_->addAction(action);
}

void GnomeGrabber::RemoveAction(CompAction const& action)
{
  impl_->removeAction(action);
}

} // namespace key
} // namespace unity
