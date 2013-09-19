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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITYSHARED_DESKTOP_APPLICATION_MANAGER_H
#define UNITYSHARED_DESKTOP_APPLICATION_MANAGER_H

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

#include "unity-shared/ApplicationManager.h"
#include "unity-shared/ZeitgeistUtils.h"

namespace unity
{
namespace desktop
{

class Application : public ::unity::Application
{
public:
  std::string desktop_id() const override;
  void LogEvent(ApplicationEventType, ApplicationSubjectPtr const&) const override;
};

class ApplicationSubject : public ::unity::ApplicationSubject
{
public:
  ApplicationSubject();
  ApplicationSubject(desktop::ApplicationSubject const&);
  ApplicationSubject(::unity::ApplicationSubject const&);

  operator ZeitgeistSubject*() const;

private:
  glib::Object<ZeitgeistSubject> subject_;
};

} // namespace desktop
} // namespace unity

#endif // UNITYSHARED_DESKTOP_APPLICATION_MANAGER_H
