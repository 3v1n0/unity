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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#ifndef UNITY_ACTION_ACTOR_H
#define UNITY_ACTION_ACTOR_H

#include <sigc++/signal.h>
#include <deque>
#include <map>

#include "ActionSubject.h"

namespace unity
{
namespace actions
{

enum class MonitorType : unsigned
{
  MOST_RECENT_EVENTS,
  LEAST_RECENT_EVENTS,
  MOST_RECENT_SUBJECTS,
  LEAST_RECENT_SUBJECTS,
  MOST_POPULAR_SUBJECTS,
  LEAST_POPULAR_SUBJECTS,
  RELEVANCY
};

class Actor : public sigc::trackable, boost::noncopyable
{
public:
  typedef boost::shared_ptr<Actor> Ptr;
  typedef std::deque<Subject::Ptr> Subjects;

  virtual ~Actor() {};

  virtual void AddMonitor(MonitorType type, unsigned int pool_size) = 0;
  virtual void RemoveMonitor(MonitorType type);

  Subjects GetSubjectsForMonitor(MonitorType type) const;

  // Signals
  sigc::signal<void, MonitorType, Subject::Ptr const&> subject_added;
  sigc::signal<void, MonitorType, Subject::Ptr const&> subject_removed;

protected:
  AddSubject(Subject::Ptr const&);
  RemoveSubject(Subject::Ptr const&);

  std::map<MonitorType, Subjects> subjects_;
  std::map<MonitorType, unsigned int> subjects_pools_;
};


}
}

#endif // UNITY_ACTION_ACTOR_H
