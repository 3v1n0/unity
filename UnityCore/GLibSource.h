// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2012 Canonical Ltd
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
* Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
*/

#ifndef UNITY_GLIB_SOURCE_H
#define UNITY_GLIB_SOURCE_H

#include <boost/utility.hpp>
#include <sigc++/sigc++.h>
#include <memory>
#include <vector>
#include <glib.h>

namespace unity
{
namespace glib
{

class Source : public boost::noncopyable
{
public:
  typedef std::shared_ptr<Source> Ptr;
  typedef sigc::slot<bool> SourceCallback;

  enum Priority
  {
    HIGH = G_PRIORITY_HIGH,
    DEFAULT = G_PRIORITY_DEFAULT,
    HIGH_IDLE = G_PRIORITY_HIGH_IDLE,
    DEFAULT_IDLE = G_PRIORITY_DEFAULT_IDLE,
    LOW = G_PRIORITY_LOW
  };

  virtual ~Source();
  unsigned int Id();

  void Remove();

  bool IsRunning();
  virtual void Run(SourceCallback callback) = 0;

  void SetPriority(Priority prio);
  Priority GetPriority();

  sigc::signal<void, unsigned int> removed;

protected:
  Source();

  static gboolean Callback(gpointer data);
  static void DestroyCallback(gpointer data);

  GSource *source_;
  unsigned int source_id_;
  SourceCallback callback_;

private:
  void EmitRemovedSignal();
};

class Timeout : public Source
{
public:
  inline Timeout(unsigned int milliseconds, Priority prio = Priority::DEFAULT);
  inline Timeout(unsigned int milliseconds, SourceCallback cb, Priority prio = Priority::DEFAULT);

  inline void Run(SourceCallback callback);

private:
  inline void Init(unsigned int milliseconds, Priority prio);
};

class Idle : public Source
{
public:
  inline Idle(Priority prio = Priority::DEFAULT_IDLE);
  inline Idle(SourceCallback cb, Priority prio = Priority::DEFAULT_IDLE);

  inline void Run(SourceCallback callback);

private:
  inline void Init(Priority prio);
};


class SourceManager : public boost::noncopyable
{
public:
  SourceManager();
  ~SourceManager();

  void Add(Source* source);
  void Add(Source::Ptr const& source);
  void Remove(unsigned int id);

  Source::Ptr GetSource(unsigned int id) const;

protected:
  void OnSourceRemoved(unsigned int id);

  std::vector<Source::Ptr> sources_;
};

} // glib namespace
} // unity namespace

#include "GLibSource-inl.h"

#endif
