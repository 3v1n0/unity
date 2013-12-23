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
namespace grabber
{
DECLARE_LOGGER(logger, "unity.grabber.gnome");

// Private implementation
namespace shell
{
const std::string DBUS_NAME = "org.gnome.Shell";
const std::string DBUS_INTERFACE = "org.gnome.Shell";
const std::string DBUS_OBJECT_PATH = "/org/gnome/Shell";
const std::string INTROSPECTION_XML =
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
const std::string DBUS_NAME = "com.canonical.Unity.Test.GnomeKeyGrabber";
}

bool GnomeKeyGrabber::Impl::BindingLess::operator()
  (const CompAction::KeyBinding& first,
   const CompAction::KeyBinding& second) const
{
  return first.keycode() < second.keycode() ||
        (first.keycode() == second.keycode() &&
         first.modifiers() < second.modifiers());
}

GnomeKeyGrabber::Impl::Info::Info(CompAction* action,
                                  const CompAction* address)
  : action_(action)
  , address_(address)
{
}

GnomeKeyGrabber::Impl::Impl(CompScreen* screen, bool test_mode)
  : test_mode_(test_mode)
  , shell_server_(test_mode_ ? testing::DBUS_NAME : shell::DBUS_NAME)
  , screen_(screen)
  , current_action_id_(0)
{
  shell_server_.AddObjects(shell::INTROSPECTION_XML, shell::DBUS_OBJECT_PATH);
  shell_object_ = shell_server_.GetObject(shell::DBUS_INTERFACE);
  shell_object_->SetMethodsCallsHandler(
    sigc::mem_fun(this, &Impl::onShellMethodCall));
}

unsigned int GnomeKeyGrabber::Impl::addAction(const CompAction& action,
                                              bool addressable)
{
  bool resize(actions_.size() == actions_.capacity());

  current_action_id_++;
  actions_.push_back(action);
  CompAction& added(actions_.back());
  action_ids_.push_back(current_action_id_);
  Info& info(info_by_action_id_[current_action_id_]);

  if (addressable)
  {
    action_ids_by_address_[&action] = current_action_id_;
    info.address_ = &action;
  }

  if (!resize)
  {
    action_ids_by_action_[&added] = current_action_id_;
    info.action_ = &added;
  }
  else
  {
    action_ids_by_action_.clear();

    for (unsigned int i(0); i < actions_.size(); i++)
    {
      action_ids_by_action_[&actions_[i]] = action_ids_[i];
      info_by_action_id_[action_ids_[i]].action_ = &actions_[i];
    }
  }

  std::map<CompAction::KeyBinding, unsigned int>::iterator
  i(grabs_by_binding_.find(added.key()));

  if (i == grabs_by_binding_.end())
  {
    grabs_by_binding_[added.key()] = 0;
    i = grabs_by_binding_.find(added.key());
  }

  if (i->second++ == 0)
    screen_->addAction(&added);

  return current_action_id_;
}

bool GnomeKeyGrabber::Impl::removeAction(const CompAction& action)
{
  std::map<const CompAction*, unsigned int>::const_iterator
  i(action_ids_by_address_.find(&action));

  return i != action_ids_by_address_.end() && removeAction(i->second);
}

bool GnomeKeyGrabber::Impl::removeAction(unsigned int action_id)
{
  std::map<unsigned int, Info>::iterator
  i(info_by_action_id_.find(action_id));

  if (i != info_by_action_id_.end())
  {
    CompAction* action(i->second.action_);
    const CompAction* address(i->second.address_);

    if (--grabs_by_binding_[action->key()] == 0)
      screen->removeAction(action);

    std::vector<unsigned int>::iterator
    j(std::find(action_ids_.begin(), action_ids_.end(), action_id));

    if (j != action_ids_.end())
    {
      actions_.erase(actions_.begin() + (j - action_ids_.begin()));
      action_ids_.erase(j);
    }

    action_ids_by_action_.erase(action);
    action_ids_by_address_.erase(address);
    info_by_action_id_.erase(i);
    return true;
  }

  return false;
}

GVariant* GnomeKeyGrabber::Impl::onShellMethodCall(const std::string& method,
                                                   GVariant* parameters)
{
  LOG_DEBUG(logger) << "Called method '" << method << "'";

  if (method == "GrabAccelerators")
  {
    if (g_variant_is_of_type(parameters, G_VARIANT_TYPE("(a(su))")))
    {
      GVariant* variant;
      GVariantBuilder builder;
      GVariantIter* iterator;
      const gchar* accelerator;
      guint flags;

      g_variant_builder_init(&builder, G_VARIANT_TYPE("au"));
      g_variant_get(parameters, "(a(su))", &iterator);

      while (g_variant_iter_next(iterator, "(&su)", &accelerator, &flags))
        g_variant_builder_add(&builder, "u", grabAccelerator(accelerator,
                                                             flags));

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
      const gchar* accelerator;
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

unsigned int GnomeKeyGrabber::Impl::grabAccelerator(const char* accelerator,
                                                    unsigned int flags)
{
  CompAction action;
  action.keyFromString(accelerator);

  if (!isActionPostponed(action))
  {
    action.setState(CompAction::StateInitKey);
    action.setInitiate(boost::bind(&GnomeKeyGrabber::Impl::actionInitiated,
                                   this, _1, _2, _3));
  }
  else
  {
    action.setState(CompAction::StateInitKey | CompAction::StateTermKey);
    action.setTerminate(boost::bind(&GnomeKeyGrabber::Impl::actionTerminated,
                                    this, _1, _2, _3));
  }

  return addAction(action, false);
}

void GnomeKeyGrabber::Impl::activateAction(const CompAction* action,
                                           unsigned int device) const
{
  std::map<const CompAction*, unsigned int>::const_iterator
  i(action_ids_by_action_.find(action));

  if (i != action_ids_by_action_.end())
    shell_object_->EmitSignal("AcceleratorActivated",
                              g_variant_new("(uu)", i->second, device));
}

bool GnomeKeyGrabber::Impl::actionInitiated(CompAction* action,
                                            CompAction::State state,
                                            CompOption::Vector& options) const
{
  activateAction(action, 0);
  return true;
}

bool GnomeKeyGrabber::Impl::actionTerminated(CompAction* action,
                                             CompAction::State state,
                                             CompOption::Vector& options) const
{
  if (state & CompAction::StateTermTapped)
  {
    activateAction(action, 0);
    return true;
  }

  return false;
}

bool GnomeKeyGrabber::Impl::isActionPostponed(const CompAction& action) const
{
  int keycode(action.key().keycode());
  return keycode == 0 || modHandler->keycodeToModifiers(keycode) != 0;
}

// Public implementation

GnomeKeyGrabber::GnomeKeyGrabber(CompScreen* screen)
  : impl_(new Impl(screen))
{
}

GnomeKeyGrabber::GnomeKeyGrabber(CompScreen* screen, const TestMode& dummy)
  : impl_(new Impl(screen, true))
{
}

GnomeKeyGrabber::~GnomeKeyGrabber()
{
}

CompAction::Vector& GnomeKeyGrabber::getActions()
{
  return impl_->actions_;
}

void GnomeKeyGrabber::addAction(const CompAction& action)
{
  impl_->addAction(action);
}

void GnomeKeyGrabber::removeAction(const CompAction& action)
{
  impl_->removeAction(action);
}

} // namespace grabber
} // namespace unity
