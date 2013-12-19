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

GnomeKeyGrabber::Impl::Impl(GnomeKeyGrabber* grabber, CompScreen* screen, bool test_mode)
  : grabber_(grabber)
  , shell_server_(test_mode ? testing::DBUS_NAME : shell::DBUS_NAME)
  , screen_(screen)
  , current_action_id_(0)
  , test_mode_(test_mode)
{
  shell_server_.AddObjects(shell::INTROSPECTION_XML, shell::DBUS_OBJECT_PATH);
  shell_object_ = shell_server_.GetObject(shell::DBUS_INTERFACE);
  shell_object_->SetMethodsCallsHandler(sigc::mem_fun(this, &Impl::onShellMethodCall));
}

GnomeKeyGrabber::Impl::~Impl()
{
}

unsigned int GnomeKeyGrabber::Impl::addAction(const CompAction& action)
{
  current_action_id_++;
  actions_.push_back(action);
  action_for_id_[current_action_id_] = &actions_.back();
  screen_->addAction(&actions_.back());
  return current_action_id_;
}

bool GnomeKeyGrabber::Impl::removeAction(const CompAction& action)
{
  bool removed(false);

  std::list<unsigned int> action_ids;

  for (std::map<unsigned int, CompAction*>::const_iterator i(action_for_id_.begin()); i != action_for_id_.end(); ++i)
    if (*i->second == action)
      action_ids.push_front(i->first);

  for (std::list<unsigned int>::const_iterator i(action_ids.begin()); i != action_ids.end(); ++i)
    removed = removeAction(*i) || removed;

  return removed;
}

bool GnomeKeyGrabber::Impl::removeAction(unsigned int action_id)
{
  std::map<unsigned int, CompAction*>::iterator i(action_for_id_.find(action_id));

  if (i != action_for_id_.end())
  {
    CompAction* action = i->second;

    screen_->removeAction(action);
    action_for_id_.erase(i);

    for (CompAction::Vector::iterator j(actions_.begin()); j != actions_.end(); ++j)
    {
      if (&*j == action) {
        actions_.erase(j);
        return true;
      }
    }
  }

  return false;
}

GVariant* GnomeKeyGrabber::Impl::onShellMethodCall(const std::string& method, GVariant* parameters)
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

unsigned int GnomeKeyGrabber::Impl::grabAccelerator(const char *accelerator, unsigned int flags)
{
  CompAction action;
  action.keyFromString(accelerator);

  if (!isActionPostponed(action))
  {
    action.setInitiate(boost::bind(&GnomeKeyGrabber::Impl::actionInitiated, this, _1, _2, _3));
    action.setState(CompAction::StateInitKey);
  }
  else
  {
    action.setTerminate(boost::bind(&GnomeKeyGrabber::Impl::actionTerminated, this, _1, _2, _3));
    action.setState(CompAction::StateTermKey);
  }

  return addAction(action);
}

void GnomeKeyGrabber::Impl::activateAction(const CompAction* action, unsigned int device) const
{
  unsigned int action_id(-1);
  unsigned int closest_action_id(-1);

  for (std::map<unsigned int, CompAction*>::const_iterator i(action_for_id_.begin()); i != action_for_id_.end(); ++i)
  {
    if (i->second == action)
    {
      action_id = i->first;
      break;
    }

    if (*i->second == *action && (closest_action_id < 0 || i->first < closest_action_id))
      closest_action_id = i->first;
  }

  if (action_id >= 0)
    shell_object_->EmitSignal("AcceleratorActivated", g_variant_new("(uu)", action_id, device));
  else if (closest_action_id >= 0)
    shell_object_->EmitSignal("AcceleratorActivated", g_variant_new("(uu)", closest_action_id, device));
}

bool GnomeKeyGrabber::Impl::actionInitiated(CompAction* action, CompAction::State state, CompOption::Vector& options) const
{
  activateAction(action, 0);
  return true;
}

bool GnomeKeyGrabber::Impl::actionTerminated(CompAction* action, CompAction::State state, CompOption::Vector& options) const
{
  if (state & CompAction::StateTermTapped)
  {
    activateAction(action, 0);
    return true;
  }

  return false;
}

bool GnomeKeyGrabber::Impl::isActionPostponed(CompAction& action)
{
  return !action.key().keycode() || modHandler->keycodeToModifiers(action.key().keycode());
}

// Public implementation

GnomeKeyGrabber::GnomeKeyGrabber(CompScreen* screen)
  : impl_(new GnomeKeyGrabber::Impl(this, screen))
{
}

GnomeKeyGrabber::GnomeKeyGrabber(CompScreen* screen, GnomeKeyGrabber::TestMode const& tm)
  : impl_(new GnomeKeyGrabber::Impl(this, screen, true))
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
